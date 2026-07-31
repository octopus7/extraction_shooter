import { Hono } from "hono";
import type { Context, Next } from "hono";
import { z } from "zod";
import {
  type AuthenticatedUser,
  authenticateAccessRequest,
  isAccessConfigured,
} from "./auth";
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
  created_at: string;
  updated_at: string;
}

const app = new Hono<AppEnvironment>();

app.use("/api/*", async (c: Context<AppEnvironment>, next: Next) => {
  const id = requestId(c.req.raw);
  c.set("requestId", id);
  c.header("x-request-id", id);
  c.header("cache-control", "no-store");
  c.set("user", await authenticateAccessRequest(c.req.raw, c.env));
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

app.get("/api/session", (c) => {
  const user = c.get("user");
  return c.json({
    authenticated: user !== null,
    authConfigured: isAccessConfigured(c.env),
    email: user?.email ?? null,
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

app.get("/api/workspaces", async (c) => {
  const user = requireUser(c);
  const parsedLimit = Number(c.req.query("limit") ?? "50");
  const limit =
    Number.isInteger(parsedLimit) && parsedLimit >= 1 && parsedLimit <= 100
      ? parsedLimit
      : 50;

  const { results } = await c.env.DB.prepare(
    `SELECT w.id, w.title, w.catalog_id, q.slug AS catalog_slug,
            w.state_json, w.revision, w.created_at, w.updated_at
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
    `SELECT id FROM quest_catalogs WHERE id = ? LIMIT 1`,
  )
    .bind(body.catalogId)
    .first<{ id: string }>();
  if (!catalog) {
    throw new ApiError(404, "catalog_not_found", "Catalog not found.");
  }

  const id = crypto.randomUUID();
  const stateJson = JSON.stringify(body.state);
  await c.env.DB.prepare(
    `INSERT INTO workspaces (
       id, owner_subject, title, catalog_id, state_json, revision
     ) VALUES (?, ?, ?, ?, ?, 1)`,
  )
    .bind(id, user.subject, body.title, catalog.id, stateJson)
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
              w.state_json, w.revision, w.created_at, w.updated_at
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
    createdAt: row.created_at,
    updatedAt: row.updated_at,
  };
}

export default app;
