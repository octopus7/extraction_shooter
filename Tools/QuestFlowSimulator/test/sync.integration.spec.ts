import { env, exports } from "cloudflare:workers";
import { beforeEach, describe, expect, it } from "vitest";

const origin = "https://quest.test";
const password = "quest-test-password-2026";
const initialPack = {
  schemaVersion: 3,
  flavor: "Demo",
  runtime: { questDefinitions: [], questTextStringsCsv: "" },
  editor: {
    schemaVersion: 2,
    title: "Demo v1",
    questNodes: [],
    places: [],
    steps: [],
    settings: { runs: 100 },
  },
};

function request(path: string, init: RequestInit = {}, cookie?: string) {
  const headers = new Headers(init.headers);
  headers.set("Origin", origin);
  if (cookie) headers.set("Cookie", cookie);
  return exports.default.fetch(`${origin}${path}`, { ...init, headers });
}

async function login(): Promise<string> {
  const response = await request("/api/login", {
    method: "POST",
    headers: { "Content-Type": "application/json", "CF-Connecting-IP": "192.0.2.80" },
    body: JSON.stringify({ password }),
  });
  expect(response.status).toBe(200);
  return response.headers.get("set-cookie")!.split(";", 1)[0]!;
}

beforeEach(async () => {
  await env.DB.batch([
    env.DB.prepare("DELETE FROM quest_sync_audit_log"),
    env.DB.prepare("DELETE FROM quest_channels"),
    env.DB.prepare("DELETE FROM quest_releases"),
    env.DB.prepare("DELETE FROM quest_sync_tokens"),
    env.DB.prepare("DELETE FROM workspaces"),
    env.DB.prepare("DELETE FROM quest_catalogs"),
    env.DB.prepare("DELETE FROM admin_sessions"),
    env.DB.prepare("DELETE FROM admin_login_attempts"),
  ]);
  for (const catalog of [
    { id: "catalog-demo", slug: "demo", visibility: "public", flavor: "Demo" },
    { id: "catalog-main", slug: "main-m01-m20", visibility: "authenticated", flavor: "Main" },
  ] as const) {
    const pack = { ...initialPack, flavor: catalog.flavor };
    await env.DB.batch([
      env.DB.prepare(
        `INSERT INTO quest_catalogs (
           id, slug, title, visibility, schema_version, dataset_version,
           source_kind, source_hash, data_json
         ) VALUES (?, ?, ?, ?, 3, ?, 'test', ?, ?)`,
      ).bind(
        catalog.id,
        catalog.slug,
        catalog.flavor,
        catalog.visibility,
        `${catalog.slug}-v1`,
        "a".repeat(64),
        JSON.stringify(pack.editor),
      ),
      env.DB.prepare(
        `INSERT INTO quest_releases (
           id, catalog_id, dataset_version, content_hash, schema_version,
           data_json, source_kind, created_by
         ) VALUES (?, ?, ?, ?, 3, ?, 'test', 'test')`,
      ).bind(
        `release-${catalog.id}`,
        catalog.id,
        `${catalog.slug}-v1`,
        "a".repeat(64),
        JSON.stringify(pack),
      ),
      env.DB.prepare(
        `INSERT INTO quest_channels (catalog_id, current_release_id) VALUES (?, ?)`,
      ).bind(catalog.id, `release-${catalog.id}`),
    ]);
  }
});

describe("quest release sync", () => {
  it("exposes Demo while concealing Main from anonymous clients", async () => {
    expect((await request("/api/sync/demo/status")).status).toBe(200);
    expect((await request("/api/sync/demo/current")).status).toBe(200);
    expect((await request("/api/sync/main-m01-m20/status")).status).toBe(404);
  });

  it("issues a hashed scoped token and accepts it for Main reads", async () => {
    const cookie = await login();
    const created = await request(
      "/api/sync-tokens",
      {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ label: "Codex test", scopes: ["main:read"] }),
      },
      cookie,
    );
    expect(created.status).toBe(201);
    const body = await created.json() as { token: string };
    expect(body.token).toMatch(/^qsync_[A-Za-z0-9_-]{43}$/u);
    const stored = await env.DB.prepare(
      "SELECT token_hash FROM quest_sync_tokens LIMIT 1",
    ).first<{ token_hash: string }>();
    expect(stored?.token_hash).toHaveLength(64);
    expect(stored?.token_hash).not.toContain(body.token);

    const main = await request("/api/sync/main-m01-m20/current", {
      headers: { Authorization: `Bearer ${body.token}` },
    });
    expect(main.status).toBe(200);
  });

  it("publishes atomically and rejects a stale base version", async () => {
    const cookie = await login();
    const nextPack = structuredClone(initialPack);
    nextPack.editor.title = "Demo v2";
    const publish = await request(
      "/api/sync/demo/publish",
      {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          expectedBaseDatasetVersion: "demo-v1",
          pack: nextPack,
          summary: "test publish",
        }),
      },
      cookie,
    );
    expect(publish.status).toBe(201);
    const published = await publish.json() as { release: { datasetVersion: string } };
    expect(published.release.datasetVersion).toMatch(/^[0-9a-f]{64}$/u);

    const [catalog, counts] = await Promise.all([
      env.DB.prepare("SELECT dataset_version, data_json FROM quest_catalogs WHERE slug = 'demo'")
        .first<{ dataset_version: string; data_json: string }>(),
      env.DB.prepare(
        `SELECT
           (SELECT count(*) FROM quest_releases WHERE catalog_id = 'catalog-demo') AS releases,
           (SELECT count(*) FROM quest_sync_audit_log WHERE catalog_id = 'catalog-demo') AS audits`,
      ).first<{ releases: number; audits: number }>(),
    ]);
    expect(catalog?.dataset_version).toBe(published.release.datasetVersion);
    expect(JSON.parse(catalog!.data_json)).toMatchObject({ title: "Demo v2" });
    expect(counts).toMatchObject({ releases: 2, audits: 1 });

    const stale = await request(
      "/api/sync/demo/publish",
      {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ expectedBaseDatasetVersion: "demo-v1", pack: nextPack }),
      },
      cookie,
    );
    expect(stale.status).toBe(409);
    await expect(stale.json()).resolves.toMatchObject({
      error: { code: "dataset_version_conflict" },
    });
  });
});
