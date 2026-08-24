import { timingSafeEqual } from "node:crypto";

export interface AuthenticatedUser {
  subject: "admin";
  name: "관리자";
}

export interface SyncPrincipal {
  subject: string;
  tokenId: string | null;
  scopes: string[];
}

interface AdminAuthBindings {
  DB: D1Database;
  ADMIN_PASSWORD?: string;
}

interface LoginAttemptRow {
  window_started_at: number;
  failure_count: number;
  blocked_until: number;
}

const SECURE_SESSION_COOKIE = "__Host-quest_session";
const LOCAL_SESSION_COOKIE = "quest_session";
const SESSION_TTL_SECONDS = 12 * 60 * 60;
const LOGIN_WINDOW_SECONDS = 15 * 60;
const LOGIN_BLOCK_SECONDS = 15 * 60;
const MAX_LOGIN_FAILURES = 5;
const TOKEN_PATTERN = /^[A-Za-z0-9_-]{43}$/;
const SYNC_TOKEN_PATTERN = /^qsync_[A-Za-z0-9_-]{43}$/;
const textEncoder = new TextEncoder();

export function isAdminAuthConfigured(env: AdminAuthBindings): boolean {
  return typeof env.ADMIN_PASSWORD === "string" && env.ADMIN_PASSWORD.length >= 16;
}

export async function authenticateAdminSession(
  request: Request,
  env: AdminAuthBindings,
): Promise<AuthenticatedUser | null> {
  if (!isAdminAuthConfigured(env)) {
    return null;
  }

  const token = readSessionToken(request);
  if (!token) {
    return null;
  }

  const now = unixTime();
  const tokenHash = await sha256Hex(token);
  const row = await env.DB.prepare(
    `SELECT token_hash
       FROM admin_sessions
      WHERE token_hash = ? AND expires_at > ?
      LIMIT 1`,
  )
    .bind(tokenHash, now)
    .first<{ token_hash: string }>();

  return row ? { subject: "admin", name: "관리자" } : null;
}

export async function authenticateSyncToken(
  request: Request,
  env: AdminAuthBindings,
): Promise<SyncPrincipal | null> {
  const authorization = request.headers.get("authorization");
  if (!authorization?.startsWith("Bearer ")) return null;
  const token = authorization.slice("Bearer ".length).trim();
  if (!SYNC_TOKEN_PATTERN.test(token)) return null;

  const now = unixTime();
  const row = await env.DB.prepare(
    `SELECT token_id, scopes_json
       FROM quest_sync_tokens
      WHERE token_hash = ?
        AND revoked_at IS NULL
        AND (expires_at IS NULL OR expires_at > ?)
      LIMIT 1`,
  )
    .bind(await sha256Hex(token), now)
    .first<{ token_id: string; scopes_json: string }>();
  if (!row) return null;

  const scopes = parseScopes(row.scopes_json);
  if (!scopes) return null;
  await env.DB.prepare(
    `UPDATE quest_sync_tokens SET last_used_at = ? WHERE token_id = ?`,
  )
    .bind(now, row.token_id)
    .run();
  return {
    subject: `codex:${row.token_id}`,
    tokenId: row.token_id,
    scopes,
  };
}

export async function createSyncTokenValue(): Promise<{
  token: string;
  tokenHash: string;
}> {
  const token = `qsync_${randomToken()}`;
  return { token, tokenHash: await sha256Hex(token) };
}

export async function verifyAdminPassword(
  providedPassword: string,
  env: AdminAuthBindings,
): Promise<boolean> {
  const expectedPassword = env.ADMIN_PASSWORD;
  if (!isAdminAuthConfigured(env) || !expectedPassword) {
    return false;
  }

  const [providedHash, expectedHash] = await Promise.all([
    crypto.subtle.digest("SHA-256", textEncoder.encode(providedPassword)),
    crypto.subtle.digest("SHA-256", textEncoder.encode(expectedPassword)),
  ]);
  return timingSafeEqual(
    new Uint8Array(providedHash),
    new Uint8Array(expectedHash),
  );
}

export async function assertLoginAllowed(
  request: Request,
  env: AdminAuthBindings,
): Promise<void> {
  const key = await clientKey(request);
  const row = await env.DB.prepare(
    `SELECT window_started_at, failure_count, blocked_until
       FROM admin_login_attempts
      WHERE client_key = ?
      LIMIT 1`,
  )
    .bind(key)
    .first<LoginAttemptRow>();

  if (row && row.blocked_until > unixTime()) {
    throw new LoginRateLimitError(row.blocked_until);
  }
}

export async function recordLoginFailure(
  request: Request,
  env: AdminAuthBindings,
): Promise<void> {
  const now = unixTime();
  const key = await clientKey(request);
  await env.DB.prepare(
    `INSERT INTO admin_login_attempts (
       client_key, window_started_at, failure_count, blocked_until
     ) VALUES (?, ?, 1, 0)
     ON CONFLICT(client_key) DO UPDATE SET
       blocked_until = CASE
         WHEN ? - admin_login_attempts.window_started_at < ?
          AND admin_login_attempts.failure_count + 1 >= ?
         THEN ? + ?
         ELSE 0
       END,
       failure_count = CASE
         WHEN ? - admin_login_attempts.window_started_at < ?
         THEN admin_login_attempts.failure_count + 1
         ELSE 1
       END,
       window_started_at = CASE
         WHEN ? - admin_login_attempts.window_started_at < ?
         THEN admin_login_attempts.window_started_at
         ELSE ?
       END`,
  )
    .bind(
      key,
      now,
      now,
      LOGIN_WINDOW_SECONDS,
      MAX_LOGIN_FAILURES,
      now,
      LOGIN_BLOCK_SECONDS,
      now,
      LOGIN_WINDOW_SECONDS,
      now,
      LOGIN_WINDOW_SECONDS,
      now,
    )
    .run();
}

export async function createAdminSession(
  request: Request,
  env: AdminAuthBindings,
): Promise<string> {
  const token = randomToken();
  const tokenHash = await sha256Hex(token);
  const now = unixTime();
  const expiresAt = now + SESSION_TTL_SECONDS;
  const key = await clientKey(request);

  await env.DB.batch([
    env.DB.prepare(
      `DELETE FROM admin_login_attempts WHERE client_key = ?`,
    ).bind(key),
    env.DB.prepare(
      `DELETE FROM admin_sessions WHERE expires_at <= ?`,
    ).bind(now),
    env.DB.prepare(
      `INSERT INTO admin_sessions (token_hash, owner_subject, expires_at)
       VALUES (?, 'admin', ?)`,
    ).bind(tokenHash, expiresAt),
  ]);

  return serializeSessionCookie(
    token,
    SESSION_TTL_SECONDS,
    sessionCookieName(request) === SECURE_SESSION_COOKIE,
  );
}

export async function revokeAdminSession(
  request: Request,
  env: AdminAuthBindings,
): Promise<void> {
  const token = readSessionToken(request);
  if (!token) {
    return;
  }

  await env.DB.prepare(
    `DELETE FROM admin_sessions WHERE token_hash = ?`,
  )
    .bind(await sha256Hex(token))
    .run();
}

export function clearAdminSessionCookie(request: Request): string {
  return serializeSessionCookie(
    "",
    0,
    sessionCookieName(request) === SECURE_SESSION_COOKIE,
  );
}

export class LoginRateLimitError extends Error {
  constructor(readonly retryAt: number) {
    super("Too many login attempts.");
  }
}

function readSessionToken(request: Request): string | null {
  const cookie = request.headers.get("cookie");
  if (!cookie) {
    return null;
  }
  const expectedName = sessionCookieName(request);

  for (const part of cookie.split(";")) {
    const separator = part.indexOf("=");
    if (separator < 0) {
      continue;
    }
    if (part.slice(0, separator).trim() !== expectedName) {
      continue;
    }
    const token = part.slice(separator + 1).trim();
    return TOKEN_PATTERN.test(token) ? token : null;
  }
  return null;
}

function serializeSessionCookie(
  token: string,
  maxAge: number,
  secure: boolean,
): string {
  const attributes = [
    `${secure ? SECURE_SESSION_COOKIE : LOCAL_SESSION_COOKIE}=${token}`,
    "Path=/",
    `Max-Age=${maxAge}`,
    "HttpOnly",
    "SameSite=Strict",
  ];
  if (secure) {
    attributes.push("Secure");
  }
  return attributes.join("; ");
}

function sessionCookieName(request: Request): string {
  const url = new URL(request.url);
  const localDevelopment =
    url.protocol === "http:" &&
    (url.hostname === "localhost" ||
      url.hostname === "127.0.0.1" ||
      url.hostname === "[::1]");
  return localDevelopment ? LOCAL_SESSION_COOKIE : SECURE_SESSION_COOKIE;
}

function randomToken(): string {
  const bytes = new Uint8Array(32);
  crypto.getRandomValues(bytes);
  return base64Url(bytes);
}

async function clientKey(request: Request): Promise<string> {
  const address = request.headers.get("cf-connecting-ip")?.trim() || "local";
  return sha256Hex(`quest-admin:${address}`);
}

export async function sha256Hex(value: string): Promise<string> {
  const digest = await crypto.subtle.digest("SHA-256", textEncoder.encode(value));
  return Array.from(new Uint8Array(digest), (byte) =>
    byte.toString(16).padStart(2, "0"),
  ).join("");
}

function parseScopes(value: string): string[] | null {
  try {
    const parsed: unknown = JSON.parse(value);
    return Array.isArray(parsed) && parsed.every((scope) => typeof scope === "string")
      ? parsed
      : null;
  } catch {
    return null;
  }
}

function base64Url(bytes: Uint8Array): string {
  let binary = "";
  for (const byte of bytes) {
    binary += String.fromCharCode(byte);
  }
  return btoa(binary)
    .replaceAll("+", "-")
    .replaceAll("/", "_")
    .replace(/=+$/u, "");
}

function unixTime(): number {
  return Math.floor(Date.now() / 1000);
}
