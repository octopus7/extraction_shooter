import type {
  CatalogSummary,
  QuestDataset,
  QuestSyncRelease,
  Session,
  Workspace,
} from "../shared/types";
import { normalizeQuestDataset } from "../shared/dataset";

async function request<T>(path: string, init?: RequestInit): Promise<T> {
  const response = await fetch(path, {
    ...init,
    headers: {
      "content-type": "application/json",
      ...init?.headers,
    },
  });

  if (!response.ok) {
    const body = await response.json().catch(() => null) as {
      error?: string | { message?: string };
    } | null;
    const message =
      typeof body?.error === "string" ? body.error : body?.error?.message;
    throw new Error(message ?? `${response.status} ${response.statusText}`);
  }

  return response.json() as Promise<T>;
}

function unwrapList<T>(value: unknown, keys: string[]): T[] {
  if (Array.isArray(value)) return value as T[];
  if (value && typeof value === "object") {
    const record = value as Record<string, unknown>;
    for (const key of keys) {
      if (Array.isArray(record[key])) return record[key] as T[];
    }
  }
  return [];
}

function parseJson<T>(value: unknown): T | undefined {
  if (typeof value === "string") {
    try {
      return JSON.parse(value) as T;
    } catch {
      return undefined;
    }
  }
  if (value && typeof value === "object") return value as T;
  return undefined;
}

export async function getSession(): Promise<Session> {
  const value = await request<Record<string, unknown>>("/api/session");
  const source =
    value.session && typeof value.session === "object"
      ? value.session as Record<string, unknown>
      : value;
  return {
    authenticated: Boolean(
      source.authenticated ?? source.isAuthenticated ?? source.loggedIn,
    ),
    authConfigured: Boolean(source.authConfigured),
    subject: typeof source.subject === "string" ? source.subject : undefined,
    displayName:
      typeof source.displayName === "string"
        ? source.displayName
        : typeof source.name === "string"
          ? source.name
          : undefined,
  };
}

export async function loginAdmin(password: string): Promise<Session> {
  return request<Session>("/api/login", {
    method: "POST",
    body: JSON.stringify({ password }),
  });
}

export async function logoutAdmin(): Promise<Session> {
  return request<Session>("/api/logout", {
    method: "POST",
    body: JSON.stringify({}),
  });
}

export async function getCatalogs(): Promise<CatalogSummary[]> {
  return unwrapList<CatalogSummary>(
    await request<unknown>("/api/catalogs"),
    ["catalogs", "items", "data"],
  );
}

export async function getCatalog(
  slug: string,
): Promise<{ catalog: CatalogSummary; dataset: QuestDataset }> {
  const response = await request<Record<string, unknown>>(
    `/api/catalogs/${encodeURIComponent(slug)}`,
  );
  const rawCatalog =
    response.catalog && typeof response.catalog === "object"
      ? response.catalog as Record<string, unknown>
      : response;
  const dataset = normalizeQuestDataset(
    parseJson<unknown>(
      response.dataset ??
        response.data ??
        response.dataJson ??
        response.data_json ??
        rawCatalog.data_json ??
        rawCatalog.dataJson,
    ),
  );
  if (!dataset) throw new Error("catalog 데이터 형식을 읽을 수 없습니다.");

  return {
    catalog: {
      id: String(rawCatalog.id ?? response.id ?? ""),
      slug: String(rawCatalog.slug ?? response.slug ?? slug),
      title: String(rawCatalog.title ?? response.title ?? dataset.title),
      visibility:
        (rawCatalog.visibility ?? response.visibility) === "authenticated"
          ? "authenticated"
          : "public",
      datasetVersion: String(
        rawCatalog.datasetVersion ??
          rawCatalog.dataset_version ??
          response.datasetVersion ??
          response.dataset_version ??
          "",
      ) || undefined,
      description:
        typeof rawCatalog.description === "string"
          ? rawCatalog.description
          : undefined,
    },
    dataset,
  };
}

export async function getWorkspaces(): Promise<Workspace[]> {
  const raw = unwrapList<Record<string, unknown>>(
    await request<unknown>("/api/workspaces"),
    ["workspaces", "items", "data"],
  );
  return raw.flatMap((item) => {
    const state = normalizeQuestDataset(
      parseJson<unknown>(
        item.state ?? item.stateJson ?? item.state_json,
      ),
    );
    if (!state) return [];
    return [{
      id: String(item.id),
      title: String(item.title ?? state.title),
      catalogId: String(item.catalogId ?? item.catalog_id ?? ""),
      state,
      revision: Number(item.revision ?? 0),
      baseDatasetVersion:
        String(item.baseDatasetVersion ?? item.base_dataset_version ?? "") ||
        undefined,
      updatedAt: String(item.updatedAt ?? item.updated_at ?? "") || undefined,
    }];
  });
}

export async function createWorkspace(
  title: string,
  catalogId: string,
  state: QuestDataset,
): Promise<Workspace> {
  const value = await request<Workspace | { workspace: Workspace }>(
    "/api/workspaces",
    {
      method: "POST",
      body: JSON.stringify({
        title,
        catalogId,
        state,
      }),
    },
  );
  return "workspace" in value ? value.workspace : value;
}

export async function updateWorkspace(
  workspace: Workspace,
  state: QuestDataset,
): Promise<Workspace> {
  const value = await request<Workspace | { workspace: Workspace }>(
    `/api/workspaces/${encodeURIComponent(workspace.id)}`,
    {
      method: "PUT",
      body: JSON.stringify({
        title: workspace.title,
        state,
        revision: workspace.revision,
      }),
    },
  );
  return "workspace" in value ? value.workspace : value;
}

export async function getCurrentQuestRelease(
  slug: string,
): Promise<QuestSyncRelease> {
  const value = await request<{ release: QuestSyncRelease }>(
    `/api/sync/${encodeURIComponent(slug)}/current`,
  );
  return value.release;
}

export async function publishEditorDataset(
  slug: string,
  expectedBaseDatasetVersion: string,
  editor: QuestDataset,
  workspaceId?: string,
): Promise<QuestSyncRelease> {
  const current = await getCurrentQuestRelease(slug);
  const value = await request<{ release: QuestSyncRelease }>(
    `/api/sync/${encodeURIComponent(slug)}/publish`,
    {
      method: "POST",
      body: JSON.stringify({
        expectedBaseDatasetVersion,
        workspaceId,
        pack: { ...current.pack, editor },
        summary: "Published from the Quest Flow web editor",
      }),
    },
  );
  return value.release;
}

export async function createCodexSyncToken(): Promise<string> {
  const value = await request<{ token: string }>("/api/sync-tokens", {
    method: "POST",
    body: JSON.stringify({
      label: `Codex ${new Date().toISOString().slice(0, 10)}`,
      scopes: [
        "demo:read",
        "demo:write",
        "demo:publish",
        "main:read",
        "main:write",
        "main:publish",
      ],
      expiresInDays: 90,
    }),
  });
  return value.token;
}
