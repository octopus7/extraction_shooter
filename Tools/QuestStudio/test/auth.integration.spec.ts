import { env, exports } from "cloudflare:workers";
import { beforeEach, describe, expect, it } from "vitest";

const origin = "https://quest.test";
const password = "quest-test-password-2026";

function request(
  path: string,
  init: RequestInit = {},
  cookie?: string,
): Promise<Response> {
  const headers = new Headers(init.headers);
  headers.set("Origin", origin);
  if (cookie) {
    headers.set("Cookie", cookie);
  }
  return exports.default.fetch(`${origin}${path}`, {
    ...init,
    headers,
  });
}

function login(passwordAttempt: string, address: string): Promise<Response> {
  return request("/api/login", {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
      "CF-Connecting-IP": address,
    },
    body: JSON.stringify({ password: passwordAttempt }),
  });
}

function cookiePair(response: Response): string {
  const setCookie = response.headers.get("set-cookie");
  expect(setCookie).toContain("__Host-quest_session=");
  return setCookie!.split(";", 1)[0]!;
}

beforeEach(async () => {
  await env.DB.batch([
    env.DB.prepare("DELETE FROM admin_sessions"),
    env.DB.prepare("DELETE FROM admin_login_attempts"),
  ]);
});

describe("administrator authentication", () => {
  it("creates a revocable session and logs out", async () => {
    const anonymousSession = await request("/api/session");
    expect(anonymousSession.status).toBe(200);
    await expect(anonymousSession.json()).resolves.toMatchObject({
      authenticated: false,
      authConfigured: true,
    });

    const denied = await login("incorrect-password", "192.0.2.10");
    expect(denied.status).toBe(401);
    await expect(denied.json()).resolves.toMatchObject({
      error: { code: "invalid_credentials" },
    });

    const loginResponse = await login(password, "192.0.2.10");
    expect(loginResponse.status).toBe(200);
    const sessionCookie = cookiePair(loginResponse);
    const rawToken = sessionCookie.split("=", 2)[1]!;
    expect(loginResponse.headers.get("set-cookie")).toContain("HttpOnly");
    expect(loginResponse.headers.get("set-cookie")).toContain("Secure");
    expect(loginResponse.headers.get("set-cookie")).toContain("SameSite=Strict");
    const storedSession = await env.DB.prepare(
      "SELECT token_hash FROM admin_sessions LIMIT 1",
    ).first<{ token_hash: string }>();
    expect(storedSession?.token_hash).toHaveLength(64);
    expect(storedSession?.token_hash).not.toBe(rawToken);

    const authenticatedSession = await request(
      "/api/session",
      {},
      sessionCookie,
    );
    await expect(authenticatedSession.json()).resolves.toMatchObject({
      authenticated: true,
      subject: "admin",
    });

    const workspaces = await request("/api/workspaces", {}, sessionCookie);
    expect(workspaces.status).toBe(200);
    await expect(workspaces.json()).resolves.toEqual({ workspaces: [] });

    const logout = await request(
      "/api/logout",
      { method: "POST" },
      sessionCookie,
    );
    expect(logout.status).toBe(200);
    expect(logout.headers.get("set-cookie")).toContain("Max-Age=0");

    const reusedSession = await request("/api/session", {}, sessionCookie);
    await expect(reusedSession.json()).resolves.toMatchObject({
      authenticated: false,
    });

    const deniedAfterLogout = await request(
      "/api/workspaces",
      {},
      sessionCookie,
    );
    expect(deniedAfterLogout.status).toBe(401);
  });

  it("rate-limits repeated password failures", async () => {
    const failures = await Promise.all(
      Array.from({ length: 5 }, () =>
        login("incorrect-password", "192.0.2.20"),
      ),
    );
    expect(failures.map((response) => response.status)).toEqual([
      401, 401, 401, 401, 401,
    ]);

    const blocked = await login("incorrect-password", "192.0.2.20");
    expect(blocked.status).toBe(429);
    expect(Number(blocked.headers.get("retry-after"))).toBeGreaterThan(0);
  });

  it("rejects cross-origin login and logout", async () => {
    const foreignHeaders = {
      Origin: "https://example.com",
      "Content-Type": "application/json",
    };
    const loginResponse = await exports.default.fetch(
      `${origin}/api/login`,
      {
        method: "POST",
        headers: foreignHeaders,
        body: JSON.stringify({ password }),
      },
    );
    expect(loginResponse.status).toBe(403);

    const logoutResponse = await exports.default.fetch(
      `${origin}/api/logout`,
      {
        method: "POST",
        headers: { Origin: "https://example.com" },
      },
    );
    expect(logoutResponse.status).toBe(403);
  });

  it("uses a development cookie that browsers accept over local HTTP", async () => {
    const localOrigin = "http://127.0.0.1:5179";
    const loginResponse = await exports.default.fetch(
      `${localOrigin}/api/login`,
      {
        method: "POST",
        headers: {
          Origin: localOrigin,
          "Content-Type": "application/json",
          "CF-Connecting-IP": "192.0.2.30",
        },
        body: JSON.stringify({ password }),
      },
    );

    expect(loginResponse.status).toBe(200);
    const setCookie = loginResponse.headers.get("set-cookie");
    expect(setCookie).toContain("quest_session=");
    expect(setCookie).not.toContain("__Host-quest_session=");
    expect(setCookie).not.toContain("Secure");

    const localCookie = setCookie!.split(";", 1)[0]!;
    const sessionResponse = await exports.default.fetch(
      `${localOrigin}/api/session`,
      {
        headers: { Cookie: localCookie },
      },
    );
    await expect(sessionResponse.json()).resolves.toMatchObject({
      authenticated: true,
    });
  });

  it("rejects administrator login over non-local HTTP", async () => {
    const insecureOrigin = "http://quest.example";
    const response = await exports.default.fetch(
      `${insecureOrigin}/api/login`,
      {
        method: "POST",
        headers: {
          Origin: insecureOrigin,
          "Content-Type": "application/json",
        },
        body: JSON.stringify({ password }),
      },
    );

    expect(response.status).toBe(403);
    await expect(response.json()).resolves.toMatchObject({
      error: { code: "secure_transport_required" },
    });
  });

  it("keeps authenticated catalogs hidden until login", async () => {
    await env.DB.prepare(
      `INSERT OR REPLACE INTO quest_catalogs (
         id, slug, title, visibility, schema_version, dataset_version,
         source_kind, source_hash, data_json
       ) VALUES (?, ?, ?, 'authenticated', 1, 'test-v1', 'test', 'test-hash', ?)`,
    )
      .bind(
        "catalog-private-test",
        "private-test",
        "Private test",
        JSON.stringify({ title: "Private test" }),
      )
      .run();

    const anonymousList = await request("/api/catalogs");
    expect(await anonymousList.text()).not.toContain("private-test");
    expect((await request("/api/catalogs/private-test")).status).toBe(404);

    const loginResponse = await login(password, "192.0.2.40");
    const sessionCookie = cookiePair(loginResponse);
    const authenticatedList = await request(
      "/api/catalogs",
      {},
      sessionCookie,
    );
    expect(await authenticatedList.text()).toContain("private-test");
    expect(
      (await request("/api/catalogs/private-test", {}, sessionCookie)).status,
    ).toBe(200);
  });
});
