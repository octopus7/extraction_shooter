import { ValidationError, isRecord } from '../shared/validation';
import { ApiError, json, readJson, requireSameOrigin } from './http';
import { assertLoginAllowed, authenticateAdminSession, clearAdminSessionCookie, createAdminSession,
  isAdminAuthConfigured, isLocalRequest, LoginRateLimitError, recordLoginFailure, revokeAdminSession, verifyAdminPassword } from './auth';
import { deleteSnapshot, listSnapshots, loadSnapshot, overwriteSnapshot, saveSnapshot } from './snapshots';

export default {
  async fetch(request: Request, env: Env): Promise<Response> {
    try {
      const path = new URL(request.url).pathname; const method = request.method;
      if (path.startsWith('/api/') && !['GET', 'HEAD', 'OPTIONS'].includes(method)) requireSameOrigin(request);
      if (path === '/api/session' && method === 'GET') {
        return json({ authenticated: await authenticateAdminSession(request, env), authConfigured: isAdminAuthConfigured(env) });
      }
      if (path === '/api/login' && method === 'POST') {
        if (new URL(request.url).protocol !== 'https:' && !isLocalRequest(request)) throw new ApiError(403, 'api.secure_transport_required');
        if (!isAdminAuthConfigured(env)) throw new ApiError(503, 'api.authentication_not_configured');
        try { await assertLoginAllowed(request, env); }
        catch (error) {
          if (!(error instanceof LoginRateLimitError)) throw error;
          const retryAfter = Math.max(1, error.retryAt - Math.floor(Date.now() / 1000));
          throw new ApiError(429, 'api.too_many_login_attempts', { retryAfter }, { 'retry-after': String(retryAfter) });
        }
        const body = await readJson(request);
        if (!isRecord(body) || typeof body.password !== 'string') throw new ApiError(422, 'api.invalid_json');
        if (!await verifyAdminPassword(body.password, env)) {
          await recordLoginFailure(request, env); throw new ApiError(401, 'api.invalid_credentials');
        }
        return json({ authenticated: true, authConfigured: true }, 200, { 'set-cookie': await createAdminSession(request, env) });
      }
      if (path === '/api/logout' && method === 'POST') {
        await revokeAdminSession(request, env);
        return json({ authenticated: false, authConfigured: isAdminAuthConfigured(env) }, 200, { 'set-cookie': clearAdminSessionCookie(request) });
      }
      if (path === '/api/snapshots' || path.startsWith('/api/snapshots/')) {
        if (!await authenticateAdminSession(request, env)) throw new ApiError(401, 'api.unauthorized');
        if (path === '/api/snapshots' && method === 'GET') return json({ snapshots: await listSnapshots(env.DB) });
        if (path === '/api/snapshots' && method === 'POST') return json({ snapshot: await saveSnapshot(env.DB, await readJson(request)) }, 201);
        const mutation = /^\/api\/snapshots\/([^/]+)\/(overwrite|delete)$/.exec(path);
        if (mutation && method === 'POST') {
          const body = await readJson(request);
          if (mutation[2] === 'overwrite') return json({ snapshot: await overwriteSnapshot(env.DB, mutation[1]!, body) });
          await deleteSnapshot(env.DB, mutation[1]!, body);
          return json({ deleted: true });
        }
        const match = /^\/api\/snapshots\/([^/]+)$/.exec(path);
        if (match && method === 'GET') return json(await loadSnapshot(env.DB, match[1]!));
      }
      throw new ApiError(404, 'api.not_found');
    } catch (error) {
      if (error instanceof ApiError) return json({ error: { key: error.key, ...(error.params ? { params: error.params } : {}) } }, error.status, error.headers);
      if (error instanceof ValidationError) return json({ error: { key: error.key, ...(error.params ? { params: error.params } : {}) } }, 422);
      return json({ error: { key: 'api.internal_error' } }, 500);
    }
  },
};
