import { timingSafeEqual } from 'node:crypto';

// Administrator-only behavior retained from the former QuestStudio auth module.
const SECURE_COOKIE = '__Host-quest_session';
const LOCAL_COOKIE = 'quest_session';
const SESSION_SECONDS = 12 * 60 * 60;
const WINDOW_SECONDS = 15 * 60;
const MAX_FAILURES = 5;
const encoder = new TextEncoder();
const now = () => Math.floor(Date.now() / 1000);

export function isAdminAuthConfigured(env: Env): boolean {
  return typeof env.ADMIN_PASSWORD === 'string' && env.ADMIN_PASSWORD.length >= 10;
}
export function isLocalRequest(request: Request): boolean {
  const url = new URL(request.url);
  return url.protocol === 'http:' && ['localhost', '127.0.0.1', '[::1]'].includes(url.hostname);
}
const cookieName = (request: Request) => isLocalRequest(request) ? LOCAL_COOKIE : SECURE_COOKIE;
function readToken(request: Request): string | null {
  for (const part of (request.headers.get('cookie') ?? '').split(';')) {
    const index = part.indexOf('=');
    if (index < 0 || part.slice(0, index).trim() !== cookieName(request)) continue;
    const token = part.slice(index + 1).trim();
    return /^[A-Za-z0-9_-]{43}$/.test(token) ? token : null;
  }
  return null;
}
export async function sha256Hex(value: string): Promise<string> {
  const digest = await crypto.subtle.digest('SHA-256', encoder.encode(value));
  return Array.from(new Uint8Array(digest), byte => byte.toString(16).padStart(2, '0')).join('');
}
async function clientKey(request: Request): Promise<string> {
  return sha256Hex(`quest-admin:${request.headers.get('cf-connecting-ip')?.trim() || 'local'}`);
}
export async function authenticateAdminSession(request: Request, env: Env): Promise<boolean> {
  if (!isAdminAuthConfigured(env)) return false;
  const token = readToken(request); if (!token) return false;
  const row = await env.DB.prepare('SELECT token_hash FROM admin_sessions WHERE token_hash = ? AND expires_at > ? LIMIT 1')
    .bind(await sha256Hex(token), now()).first();
  return row !== null;
}
export async function verifyAdminPassword(password: string, env: Env): Promise<boolean> {
  if (!isAdminAuthConfigured(env)) return false;
  const [provided, expected] = await Promise.all([
    crypto.subtle.digest('SHA-256', encoder.encode(password)),
    crypto.subtle.digest('SHA-256', encoder.encode(env.ADMIN_PASSWORD)),
  ]);
  return timingSafeEqual(new Uint8Array(provided), new Uint8Array(expected));
}
export class LoginRateLimitError extends Error {
  constructor(readonly retryAt: number) { super('api.too_many_login_attempts'); }
}
export async function assertLoginAllowed(request: Request, env: Env): Promise<void> {
  const row = await env.DB.prepare('SELECT blocked_until FROM admin_login_attempts WHERE client_key = ? LIMIT 1')
    .bind(await clientKey(request)).first<{ blocked_until: number }>();
  if (row && row.blocked_until > now()) throw new LoginRateLimitError(row.blocked_until);
}
export async function recordLoginFailure(request: Request, env: Env): Promise<void> {
  const timestamp = now();
  await env.DB.prepare(`INSERT INTO admin_login_attempts (client_key, window_started_at, failure_count, blocked_until)
    VALUES (?, ?, 1, 0) ON CONFLICT(client_key) DO UPDATE SET
    blocked_until = CASE WHEN ? - admin_login_attempts.window_started_at < ? AND admin_login_attempts.failure_count + 1 >= ? THEN ? + ? ELSE 0 END,
    failure_count = CASE WHEN ? - admin_login_attempts.window_started_at < ? THEN admin_login_attempts.failure_count + 1 ELSE 1 END,
    window_started_at = CASE WHEN ? - admin_login_attempts.window_started_at < ? THEN admin_login_attempts.window_started_at ELSE ? END`)
    .bind(await clientKey(request), timestamp, timestamp, WINDOW_SECONDS, MAX_FAILURES, timestamp, WINDOW_SECONDS,
      timestamp, WINDOW_SECONDS, timestamp, WINDOW_SECONDS, timestamp).run();
}
function serializeCookie(request: Request, token: string, maxAge: number): string {
  return [`${cookieName(request)}=${token}`, 'Path=/', `Max-Age=${maxAge}`, 'HttpOnly', 'SameSite=Strict',
    ...(isLocalRequest(request) ? [] : ['Secure'])].join('; ');
}
export async function createAdminSession(request: Request, env: Env): Promise<string> {
  const bytes = crypto.getRandomValues(new Uint8Array(32));
  const token = btoa(String.fromCharCode(...bytes)).replaceAll('+', '-').replaceAll('/', '_').replace(/=+$/, '');
  const timestamp = now();
  await env.DB.batch([
    env.DB.prepare('DELETE FROM admin_login_attempts WHERE client_key = ?').bind(await clientKey(request)),
    env.DB.prepare('DELETE FROM admin_sessions WHERE expires_at <= ?').bind(timestamp),
    env.DB.prepare("INSERT INTO admin_sessions (token_hash, owner_subject, expires_at) VALUES (?, 'admin', ?)")
      .bind(await sha256Hex(token), timestamp + SESSION_SECONDS),
  ]);
  return serializeCookie(request, token, SESSION_SECONDS);
}
export async function revokeAdminSession(request: Request, env: Env): Promise<void> {
  const token = readToken(request);
  if (token) await env.DB.prepare('DELETE FROM admin_sessions WHERE token_hash = ?').bind(await sha256Hex(token)).run();
}
export const clearAdminSessionCookie = (request: Request): string => serializeCookie(request, '', 0);
