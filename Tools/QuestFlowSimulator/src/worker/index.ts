import { Hono } from "hono";
import type { Context, Next } from "hono";
import { z } from "zod";
import {
  type AuthenticatedUser,
  type SyncPrincipal,
  LoginRateLimitError,
  assertLoginAllowed,
  authenticateAdminSession,
  authenticateSyncToken,
  clearAdminSessionCookie,
  createAdminSession,
  createSyncTokenValue,
  isAdminAuthConfigured,
  recordLoginFailure,
  revokeAdminSession,
  verifyAdminPassword,
} from "./auth";
import {
  AuthoringValidationError,
  flavorForSlug,
  hashAuthoringPack,
  normalizeAuthoringPack,
  stableStringify,
} from "../shared/authoring";
import type { QuestAuthoringPack, QuestFlavor } from "../shared/types";
import {
  ApiError,
  apiErrorResponse,
  parseStoredJson,
  readValidatedJson,
  requestId,
} from "./http";

type AppBindings = Env;

type AppVariables = {
  requestId: string;
  user: AuthenticatedUser | null;
  syncPrincipal: SyncPrincipal | null;
};

type AppEnvironment = {
  Bindings: AppBindings;
  Variables: AppVariables;
};

type JsonValue =
  | null
  | boolean
  | number
  | string
  | JsonValue[]
  | { [key: string]: JsonValue };

const jsonValueSchema: z.ZodType<JsonValue> = z.lazy(() =>
  z.union([
    z.null(),
    z.boolean(),
    z.number().finite(),
    z.string(),
    z.array(jsonValueSchema),
    z.record(z.string(), jsonValueSchema),
  ]),
);

const workspaceStateSchema = z
  .record(z.string().max(128), jsonValueSchema)
  .refine((state) => Object.keys(state).length <= 2_000, {
    message: "Workspace state has too many top-level keys.",
  });

const loginSchema = z
  .object({
    password: z.string().min(1).max(256),
  })
  .strict();

const workspaceCreateSchema = z
  .object({
    title: z.string().trim().min(1).max(120),
    catalogId: z.string().trim().min(1).max(120),
    state: workspaceStateSchema,
  })
  .strict();

const workspaceUpdateSchema = z
  .object({
    title: z.string().trim().min(1).max(120).optional(),
    state: workspaceStateSchema.optional(),
    revision: z.number().int().positive(),
  })
  .strict()
  .refine((value) => value.title !== undefined || value.state !== undefined, {
    message: "At least one of title or state is required.",
  });

const syncScopeSchema = z.enum([
  "demo:read",
  "demo:write",
  "demo:publish",
  "main:read",
  "main:write",
  "main:publish",
]);

const syncTokenCreateSchema = z
  .object({
    label: z.string().trim().min(1).max(80),
    scopes: z.array(syncScopeSchema).min(1).max(6),
    expiresInDays: z.number().int().min(1).max(365).optional(),
  })
  .strict();

const publishSchema = z
  .object({
    expectedBaseDatasetVersion: z.string().min(1).max(128),
    workspaceId: z.uuid().optional(),
    pack: z.unknown(),
    summary: z.string().trim().max(500).optional(),
  })
  .strict();

interface CatalogListRow {
  id: string;
  slug: string;
  title: string;
  visibility: "public" | "authenticated";
  schema_version: number;
  dataset_version: string;
  updated_at: string;
}

interface CatalogDetailRow extends CatalogListRow {
  source_kind: string;
  source_hash: string;
  data_json: string;
}

interface WorkspaceRow {
  id: string;
  title: string;
  catalog_id: string;
  catalog_slug: string;
  state_json: string;
  revision: number;
  base_dataset_version: string | null;
  created_at: string;
  updated_at: string;
}

interface ReleaseRow {
  id: string;
  catalog_id: string;
  slug: string;
  visibility: "public" | "authenticated";
  parent_dataset_version: string | null;
  dataset_version: string;
  content_hash: string;
  data_json: string;
  created_at: string;
}

interface SyncTokenRow {
  token_id: string;
  label: string;
  scopes_json: string;
  created_at: number;
  expires_at: number | null;
  last_used_at: number | null;
}

const app = new Hono<AppEnvironment>();

app.use("/api/*", async (c: Context<AppEnvironment>, next: Next) => {
  const id = requestId(c.req.raw);
  c.set("requestId", id);
  c.header("x-request-id", id);
  c.header("cache-control", "no-store");
  const [user, syncPrincipal] = await Promise.all([
    authenticateAdminSession(c.req.raw, c.env),
    authenticateSyncToken(c.req.raw, c.env),
  ]);
  c.set("user", user);
  c.set("syncPrincipal", syncPrincipal);
  await next();
});

function requireUser(c: Context<AppEnvironment>): AuthenticatedUser {
  const user = c.get("user");
  if (!user) {
    throw new ApiError(401, "authentication_required", "Authentication required.");
  }
  return user;
}

function requireSameOrigin(c: Context<AppEnvironment>): void {
  const origin = c.req.header("Origin");
  if (origin && origin !== new URL(c.req.url).origin) {
    throw new ApiError(403, "forbidden_origin", "Request origin is not allowed.");
  }
}

function requireSecureLoginTransport(c: Context<AppEnvironment>): void {
  const url = new URL(c.req.url);
  const localDevelopment =
    url.protocol === "http:" &&
    (url.hostname === "localhost" ||
      url.hostname === "127.0.0.1" ||
      url.hostname === "[::1]");
  if (url.protocol !== "https:" && !localDevelopment) {
    throw new ApiError(
      403,
      "secure_transport_required",
      "Administrator login requires HTTPS.",
    );
  }
}

app.get("/api/session", (c) => {
  const user = c.get("user");
  return c.json({
    authenticated: user !== null,
    authConfigured: isAdminAuthConfigured(c.env),
    subject: user?.subject ?? null,
    displayName: user?.name ?? null,
  });
});

app.post("/api/login", async (c) => {
  requireSameOrigin(c);
  requireSecureLoginTransport(c);
  if (!isAdminAuthConfigured(c.env)) {
    throw new ApiError(
      503,
      "authentication_not_configured",
      "Administrator authentication is not configured.",
    );
  }

  try {
    await assertLoginAllowed(c.req.raw, c.env);
  } catch (error) {
    if (!(error instanceof LoginRateLimitError)) {
      throw error;
    }
    c.header(
      "retry-after",
      String(Math.max(1, error.retryAt - Math.floor(Date.now() / 1000))),
    );
    throw new ApiError(
      429,
      "too_many_login_attempts",
      "Too many login attempts. Try again later.",
    );
  }

  const body = await readValidatedJson(c.req.raw, loginSchema);
  if (!(await verifyAdminPassword(body.password, c.env))) {
    await recordLoginFailure(c.req.raw, c.env);
    console.warn(
      JSON.stringify({
        level: "warning",
        event: "admin_login_failed",
        requestId: c.get("requestId"),
      }),
    );
    throw new ApiError(401, "invalid_credentials", "Invalid credentials.");
  }

  c.header("set-cookie", await createAdminSession(c.req.raw, c.env));
  return c.json({
    authenticated: true,
    authConfigured: true,
    subject: "admin",
    displayName: "관리자",
  });
});

app.post("/api/logout", async (c) => {
  requireSameOrigin(c);
  await revokeAdminSession(c.req.raw, c.env);
  c.header("set-cookie", clearAdminSessionCookie(c.req.raw));
  return c.json({
    authenticated: false,
    authConfigured: isAdminAuthConfigured(c.env),
  });
});

app.get("/api/catalogs", async (c) => {
  const authenticated = c.get("user") !== null;
  const statement = authenticated
    ? c.env.DB.prepare(
        `SELECT id, slug, title, visibility, schema_version,
                dataset_version, updated_at
           FROM quest_catalogs
          ORDER BY CASE visibility WHEN 'public' THEN 0 ELSE 1 END,
                   title, slug`,
      )
    : c.env.DB.prepare(
        `SELECT id, slug, title, visibility, schema_version,
                dataset_version, updated_at
           FROM quest_catalogs
          WHERE visibility = 'public'
          ORDER BY title, slug`,
      );
  const { results } = await statement.all<CatalogListRow>();
  return c.json({
    catalogs: results.map((row) => ({
      id: row.id,
      slug: row.slug,
      title: row.title,
      visibility: row.visibility,
      schemaVersion: row.schema_version,
      datasetVersion: row.dataset_version,
      updatedAt: row.updated_at,
    })),
  });
});

app.get("/api/catalogs/:slug", async (c) => {
  const slug = c.req.param("slug");
  if (!/^[a-z0-9]+(?:-[a-z0-9]+)*$/.test(slug) || slug.length > 80) {
    throw new ApiError(404, "not_found", "Catalog not found.");
  }

  const authenticated = c.get("user") !== null;
  const row = await c.env.DB.prepare(
    `SELECT id, slug, title, visibility, schema_version, dataset_version,
            source_kind, source_hash, data_json, updated_at
       FROM quest_catalogs
      WHERE slug = ?
        AND (visibility = 'public' OR ? = 1)
      LIMIT 1`,
  )
    .bind(slug, authenticated ? 1 : 0)
    .first<CatalogDetailRow>();

  // Private slugs deliberately look identical to missing slugs.
  if (!row) {
    throw new ApiError(404, "not_found", "Catalog not found.");
  }

  return c.json({
    catalog: {
      id: row.id,
      slug: row.slug,
      title: row.title,
      visibility: row.visibility,
      schemaVersion: row.schema_version,
      datasetVersion: row.dataset_version,
      sourceKind: row.source_kind,
      sourceHash: row.source_hash,
      updatedAt: row.updated_at,
    },
    data: parseStoredJson(row.data_json),
  });
});

app.get("/api/sync/:slug/status", async (c) => {
  const row = await selectCurrentRelease(c.env.DB, c.req.param("slug"));
  assertReleaseReadable(c, row);
  return c.json({ status: serializeSyncStatus(row!) });
});

app.get("/api/sync/:slug/current", async (c) => {
  const row = await selectCurrentRelease(c.env.DB, c.req.param("slug"));
  assertReleaseReadable(c, row);
  return c.json({ release: serializeRelease(row!) });
});

app.get("/api/sync/:slug/releases/:version", async (c) => {
  const slug = c.req.param("slug");
  if (!flavorForSlug(slug)) {
    throw new ApiError(404, "not_found", "Quest release not found.");
  }
  const row = await c.env.DB.prepare(
    `SELECT r.id, r.catalog_id, q.slug, q.visibility,
            parent.dataset_version AS parent_dataset_version,
            r.dataset_version, r.content_hash, r.data_json, r.created_at
       FROM quest_releases r
       JOIN quest_catalogs q ON q.id = r.catalog_id
       LEFT JOIN quest_releases parent ON parent.id = r.parent_release_id
      WHERE q.slug = ? AND r.dataset_version = ?
      LIMIT 1`,
  )
    .bind(slug, c.req.param("version"))
    .first<ReleaseRow>();
  assertReleaseReadable(c, row);
  return c.json({ release: serializeRelease(row!) });
});

app.post("/api/sync/:slug/publish", async (c) => {
  const slug = c.req.param("slug");
  const flavor = flavorForSlug(slug);
  if (!flavor) {
    throw new ApiError(404, "not_found", "Quest catalog not found.");
  }
  const principal = requireSyncScope(c, `${flavor.toLowerCase()}:publish`);
  if (c.get("user")) requireSameOrigin(c);
  const body = await readValidatedJson(c.req.raw, publishSchema);
  const current = await selectCurrentRelease(c.env.DB, slug);
  if (!current) {
    throw new ApiError(404, "not_found", "Quest catalog not found.");
  }
  if (current.dataset_version !== body.expectedBaseDatasetVersion) {
    throw new ApiError(
      409,
      "dataset_version_conflict",
      "The published quest dataset changed. Pull before publishing again.",
      { currentDatasetVersion: current.dataset_version },
    );
  }
  if (body.workspaceId) {
    const user = c.get("user");
    if (!user) {
      throw new ApiError(403, "workspace_session_required", "Workspace publishing requires an administrator session.");
    }
    const workspace = await selectOwnedWorkspace(c.env.DB, body.workspaceId, user.subject);
    if (!workspace) {
      throw new ApiError(404, "not_found", "Workspace not found.");
    }
    if (workspace.catalog_id !== current.catalog_id) {
      throw new ApiError(422, "workspace_catalog_mismatch", "Workspace belongs to another catalog.");
    }
    if (workspace.base_dataset_version !== body.expectedBaseDatasetVersion) {
      throw new ApiError(
        409,
        "workspace_base_conflict",
        "Workspace is based on an older quest release. Pull the current release before publishing.",
        { currentDatasetVersion: current.dataset_version },
      );
    }
  }

  let pack: QuestAuthoringPack;
  try {
    pack = normalizeAuthoringPack(body.pack);
  } catch (error) {
    if (!(error instanceof AuthoringValidationError)) throw error;
    throw new ApiError(
      422,
      "authoring_pack_invalid",
      "Quest authoring pack failed validation.",
      { issues: error.issues },
    );
  }
  if (pack.flavor !== flavor) {
    throw new ApiError(
      422,
      "flavor_mismatch",
      `Pack flavor must be ${flavor}.`,
    );
  }

  const contentHash = await hashAuthoringPack(pack);
  if (contentHash === current.content_hash) {
    return c.json({ release: serializeRelease(current), unchanged: true });
  }
  const releaseId = crypto.randomUUID();
  const datasetVersion = contentHash;
  const auditId = crypto.randomUUID();
  const packJson = stableStringify(pack);
  const editorJson = JSON.stringify(pack.editor);
  try {
    const statements = [
      c.env.DB.prepare(
        `INSERT INTO quest_releases (
           id, catalog_id, parent_release_id, dataset_version, content_hash,
           schema_version, data_json, source_kind, created_by
         )
         SELECT ?, q.id, current.id, ?, ?, 3, ?, 'sync-publish', ?
           FROM quest_catalogs q
           JOIN quest_channels ch ON ch.catalog_id = q.id
           JOIN quest_releases current ON current.id = ch.current_release_id
          WHERE q.slug = ? AND current.dataset_version = ?`,
      ).bind(
        releaseId,
        datasetVersion,
        contentHash,
        packJson,
        principal.subject,
        slug,
        body.expectedBaseDatasetVersion,
      ),
      c.env.DB.prepare(
        `UPDATE quest_channels
            SET current_release_id = ?, updated_at = CURRENT_TIMESTAMP
          WHERE catalog_id = ? AND current_release_id = ?
            AND EXISTS (SELECT 1 FROM quest_releases WHERE id = ?)`,
      ).bind(releaseId, current.catalog_id, current.id, releaseId),
      c.env.DB.prepare(
        `UPDATE quest_catalogs
            SET schema_version = 3, dataset_version = ?,
                source_kind = 'sync-publish', source_hash = ?, data_json = ?,
                updated_at = CURRENT_TIMESTAMP
          WHERE id = ? AND dataset_version = ?
            AND EXISTS (SELECT 1 FROM quest_releases WHERE id = ?)`,
      ).bind(
        datasetVersion,
        contentHash,
        editorJson,
        current.catalog_id,
        body.expectedBaseDatasetVersion,
        releaseId,
      ),
      c.env.DB.prepare(
        `INSERT INTO quest_sync_audit_log (
           id, actor_subject, action, catalog_id,
           previous_dataset_version, next_dataset_version, release_id,
           request_id, summary
         ) VALUES (?, ?, 'publish', ?, ?, ?, ?, ?, ?)`,
      ).bind(
        auditId,
        principal.subject,
        current.catalog_id,
        current.dataset_version,
        datasetVersion,
        releaseId,
        c.get("requestId"),
        body.summary ?? null,
      ),
      ...(body.workspaceId
        ? [c.env.DB.prepare(
            `UPDATE workspaces
                SET base_dataset_version = ?, updated_at = CURRENT_TIMESTAMP
              WHERE id = ? AND owner_subject = 'admin'
                AND base_dataset_version = ?`,
          ).bind(datasetVersion, body.workspaceId, body.expectedBaseDatasetVersion)]
        : []),
    ];
    await c.env.DB.batch(statements);
  } catch (error) {
    const latest = await selectCurrentRelease(c.env.DB, slug);
    if (latest && latest.dataset_version !== current.dataset_version) {
      throw new ApiError(
        409,
        "dataset_version_conflict",
        "The published quest dataset changed. Pull before publishing again.",
        { currentDatasetVersion: latest.dataset_version },
      );
    }
    throw error;
  }

  const created = await selectCurrentRelease(c.env.DB, slug);
  if (!created || created.id !== releaseId) {
    throw new ApiError(500, "publish_failed", "Quest release was not published.");
  }
  return c.json({ release: serializeRelease(created), unchanged: false }, 201);
});

app.get("/api/sync-tokens", async (c) => {
  requireUser(c);
  const { results } = await c.env.DB.prepare(
    `SELECT token_id, label, scopes_json, created_at, expires_at, last_used_at
       FROM quest_sync_tokens
      WHERE revoked_at IS NULL
      ORDER BY created_at DESC, token_id`,
  ).all<SyncTokenRow>();
  return c.json({ tokens: results.map(serializeSyncToken) });
});

app.post("/api/sync-tokens", async (c) => {
  requireSameOrigin(c);
  const user = requireUser(c);
  const body = await readValidatedJson(c.req.raw, syncTokenCreateSchema);
  const { token, tokenHash } = await createSyncTokenValue();
  const tokenId = crypto.randomUUID();
  const now = Math.floor(Date.now() / 1000);
  const expiresAt = body.expiresInDays
    ? now + body.expiresInDays * 24 * 60 * 60
    : null;
  const scopes = [...new Set(body.scopes)].sort();
  await c.env.DB.prepare(
    `INSERT INTO quest_sync_tokens (
       token_hash, token_id, label, scopes_json, expires_at, created_by
     ) VALUES (?, ?, ?, ?, ?, ?)`,
  )
    .bind(tokenHash, tokenId, body.label, JSON.stringify(scopes), expiresAt, user.subject)
    .run();
  return c.json(
    {
      token,
      item: {
        id: tokenId,
        label: body.label,
        scopes,
        createdAt: new Date(now * 1000).toISOString(),
        expiresAt: expiresAt ? new Date(expiresAt * 1000).toISOString() : null,
        lastUsedAt: null,
      },
    },
    201,
  );
});

app.delete("/api/sync-tokens/:id", async (c) => {
  requireSameOrigin(c);
  requireUser(c);
  const id = c.req.param("id");
  if (!isUuid(id)) {
    throw new ApiError(404, "not_found", "Sync token not found.");
  }
  const result = await c.env.DB.prepare(
    `UPDATE quest_sync_tokens SET revoked_at = unixepoch()
      WHERE token_id = ? AND revoked_at IS NULL`,
  )
    .bind(id)
    .run();
  if (result.meta.changes !== 1) {
    throw new ApiError(404, "not_found", "Sync token not found.");
  }
  return c.body(null, 204);
});

app.get("/api/workspaces", async (c) => {
  const user = requireUser(c);
  const parsedLimit = Number(c.req.query("limit") ?? "50");
  const limit =
    Number.isInteger(parsedLimit) && parsedLimit >= 1 && parsedLimit <= 100
      ? parsedLimit
      : 50;

  const { results } = await c.env.DB.prepare(
    `SELECT w.id, w.title, w.catalog_id, q.slug AS catalog_slug,
            w.state_json, w.revision, w.base_dataset_version,
            w.created_at, w.updated_at
       FROM workspaces w
       JOIN quest_catalogs q ON q.id = w.catalog_id
      WHERE w.owner_subject = ?
      ORDER BY w.updated_at DESC, w.id
      LIMIT ?`,
  )
    .bind(user.subject, limit)
    .all<WorkspaceRow>();

  return c.json({
    workspaces: results.map(serializeWorkspace),
  });
});

app.post("/api/workspaces", async (c) => {
  requireSameOrigin(c);
  const user = requireUser(c);
  const body = await readValidatedJson(c.req.raw, workspaceCreateSchema);
  const catalog = await c.env.DB.prepare(
    `SELECT id, dataset_version FROM quest_catalogs WHERE id = ? LIMIT 1`,
  )
    .bind(body.catalogId)
    .first<{ id: string; dataset_version: string }>();
  if (!catalog) {
    throw new ApiError(404, "catalog_not_found", "Catalog not found.");
  }

  const id = crypto.randomUUID();
  const stateJson = JSON.stringify(body.state);
  await c.env.DB.prepare(
    `INSERT INTO workspaces (
       id, owner_subject, title, catalog_id, state_json, revision,
       base_dataset_version
     ) VALUES (?, ?, ?, ?, ?, 1, ?)`,
  )
    .bind(
      id,
      user.subject,
      body.title,
      catalog.id,
      stateJson,
      catalog.dataset_version,
    )
    .run();

  const created = await selectOwnedWorkspace(c.env.DB, id, user.subject);
  if (!created) {
    throw new ApiError(500, "workspace_create_failed", "Workspace was not created.");
  }
  return c.json({ workspace: serializeWorkspace(created) }, 201);
});

app.put("/api/workspaces/:id", async (c) => {
  requireSameOrigin(c);
  const user = requireUser(c);
  const id = c.req.param("id");
  if (!isUuid(id)) {
    throw new ApiError(404, "not_found", "Workspace not found.");
  }
  const body = await readValidatedJson(c.req.raw, workspaceUpdateSchema);
  const existing = await selectOwnedWorkspace(c.env.DB, id, user.subject);
  if (!existing) {
    throw new ApiError(404, "not_found", "Workspace not found.");
  }
  if (existing.revision !== body.revision) {
    throw new ApiError(
      409,
      "revision_conflict",
      "Workspace has been updated by another request.",
      { currentRevision: existing.revision },
    );
  }

  const nextTitle = body.title ?? existing.title;
  const nextStateJson =
    body.state === undefined ? existing.state_json : JSON.stringify(body.state);
  const result = await c.env.DB.prepare(
    `UPDATE workspaces
        SET title = ?, state_json = ?, revision = revision + 1,
            updated_at = CURRENT_TIMESTAMP
      WHERE id = ? AND owner_subject = ? AND revision = ?`,
  )
    .bind(nextTitle, nextStateJson, id, user.subject, body.revision)
    .run();

  if (result.meta.changes !== 1) {
    const current = await selectOwnedWorkspace(c.env.DB, id, user.subject);
    if (!current) {
      throw new ApiError(404, "not_found", "Workspace not found.");
    }
    throw new ApiError(
      409,
      "revision_conflict",
      "Workspace has been updated by another request.",
      { currentRevision: current.revision },
    );
  }

  const updated = await selectOwnedWorkspace(c.env.DB, id, user.subject);
  if (!updated) {
    throw new ApiError(500, "workspace_update_failed", "Workspace was not updated.");
  }
  return c.json({ workspace: serializeWorkspace(updated) });
});

app.delete("/api/workspaces/:id", async (c) => {
  requireSameOrigin(c);
  const user = requireUser(c);
  const id = c.req.param("id");
  if (!isUuid(id)) {
    throw new ApiError(404, "not_found", "Workspace not found.");
  }

  const result = await c.env.DB.prepare(
    `DELETE FROM workspaces WHERE id = ? AND owner_subject = ?`,
  )
    .bind(id, user.subject)
    .run();
  if (result.meta.changes !== 1) {
    throw new ApiError(404, "not_found", "Workspace not found.");
  }
  return c.body(null, 204);
});

app.notFound((c) => {
  const id = c.get("requestId") ?? requestId(c.req.raw);
  return apiErrorResponse(
    c,
    new ApiError(404, "not_found", "Resource not found."),
    id,
  );
});

app.onError((error, c) => {
  const id = c.get("requestId") ?? requestId(c.req.raw);
  if (error instanceof ApiError) {
    if (error.status >= 500) {
      console.error(
        JSON.stringify({
          level: "error",
          event: "api_error",
          requestId: id,
          code: error.code,
          method: c.req.method,
          path: c.req.path,
        }),
      );
    }
    return apiErrorResponse(c, error, id);
  }

  console.error(
    JSON.stringify({
      level: "error",
      event: "unhandled_error",
      requestId: id,
      method: c.req.method,
      path: c.req.path,
      errorName: error instanceof Error ? error.name : "UnknownError",
      message: error instanceof Error ? error.message : "Unknown error",
    }),
  );
  return apiErrorResponse(
    c,
    new ApiError(500, "internal_error", "An internal error occurred."),
    id,
  );
});

function isUuid(value: string): boolean {
  return /^[0-9a-f]{8}-[0-9a-f]{4}-[1-8][0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/i.test(
    value,
  );
}

async function selectOwnedWorkspace(
  db: D1Database,
  id: string,
  ownerSubject: string,
): Promise<WorkspaceRow | null> {
  return db
    .prepare(
      `SELECT w.id, w.title, w.catalog_id, q.slug AS catalog_slug,
              w.state_json, w.revision, w.base_dataset_version,
              w.created_at, w.updated_at
         FROM workspaces w
         JOIN quest_catalogs q ON q.id = w.catalog_id
        WHERE w.id = ? AND w.owner_subject = ?
        LIMIT 1`,
    )
    .bind(id, ownerSubject)
    .first<WorkspaceRow>();
}

function serializeWorkspace(row: WorkspaceRow) {
  return {
    id: row.id,
    title: row.title,
    catalogId: row.catalog_id,
    catalogSlug: row.catalog_slug,
    state: parseStoredJson(row.state_json),
    revision: row.revision,
    baseDatasetVersion: row.base_dataset_version,
    createdAt: row.created_at,
    updatedAt: row.updated_at,
  };
}

function requireSyncScope(
  c: Context<AppEnvironment>,
  scope: string,
): SyncPrincipal {
  const user = c.get("user");
  if (user) {
    return { subject: user.subject, tokenId: null, scopes: ["*"] };
  }
  const principal = c.get("syncPrincipal");
  if (!principal) {
    throw new ApiError(401, "authentication_required", "Authentication required.");
  }
  if (!principal.scopes.includes(scope)) {
    throw new ApiError(403, "insufficient_scope", "Sync token lacks the required scope.");
  }
  return principal;
}

function assertReleaseReadable(
  c: Context<AppEnvironment>,
  row: ReleaseRow | null,
): void {
  if (!row) {
    throw new ApiError(404, "not_found", "Quest release not found.");
  }
  if (row.visibility === "public") return;
  const flavor = flavorForSlug(row.slug);
  if (!flavor) {
    throw new ApiError(404, "not_found", "Quest release not found.");
  }
  if (c.get("user")) return;
  const principal = c.get("syncPrincipal");
  if (!principal?.scopes.includes(`${flavor.toLowerCase()}:read`)) {
    // Authenticated catalog existence is deliberately not disclosed.
    throw new ApiError(404, "not_found", "Quest release not found.");
  }
}

async function selectCurrentRelease(
  db: D1Database,
  slug: string,
): Promise<ReleaseRow | null> {
  if (!flavorForSlug(slug)) return null;
  return db.prepare(
    `SELECT r.id, r.catalog_id, q.slug, q.visibility,
            parent.dataset_version AS parent_dataset_version,
            r.dataset_version, r.content_hash, r.data_json, r.created_at
       FROM quest_catalogs q
       JOIN quest_channels ch ON ch.catalog_id = q.id
       JOIN quest_releases r ON r.id = ch.current_release_id
       LEFT JOIN quest_releases parent ON parent.id = r.parent_release_id
      WHERE q.slug = ?
      LIMIT 1`,
  )
    .bind(slug)
    .first<ReleaseRow>();
}

function serializeSyncStatus(row: ReleaseRow) {
  const flavor = flavorForSlug(row.slug)!;
  return {
    slug: row.slug,
    flavor,
    datasetVersion: row.dataset_version,
    contentHash: row.content_hash,
    updatedAt: row.created_at,
  };
}

function serializeRelease(row: ReleaseRow) {
  return {
    ...serializeSyncStatus(row),
    parentDatasetVersion: row.parent_dataset_version,
    pack: parseStoredJson(row.data_json),
  };
}

function serializeSyncToken(row: SyncTokenRow) {
  return {
    id: row.token_id,
    label: row.label,
    scopes: parseStoredJson(row.scopes_json),
    createdAt: new Date(row.created_at * 1000).toISOString(),
    expiresAt: row.expires_at
      ? new Date(row.expires_at * 1000).toISOString()
      : null,
    lastUsedAt: row.last_used_at
      ? new Date(row.last_used_at * 1000).toISOString()
      : null,
  };
}

export default app;
