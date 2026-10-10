import { describe, expect, it, vi } from 'vitest';
import { apiRequest, ClientError, errorMessage } from './api';

describe('keyed API failures', () => {
  it('turns a keyed server failure into a localized message without exposing raw response content', async () => {
    const fetcher = vi.fn(async () => new Response(JSON.stringify({ error: { key: 'api.unauthorized' } }), { status: 401 }));
    await expect(apiRequest('/api/snapshots', undefined, fetcher)).rejects.toMatchObject({ key: 'api.unauthorized' });
    expect(errorMessage(new ClientError('api.unauthorized'))).not.toBe('api.unauthorized');
  });
  it('maps broken JSON, unexpected response and network errors to keyed client errors', async () => {
    await expect(apiRequest('/api/session', undefined, async () => new Response('bad', { status: 502 }))).rejects.toMatchObject({ key: 'client.requestFailed' });
    await expect(apiRequest('/api/session', undefined, async () => { throw new TypeError('network'); })).rejects.toMatchObject({ key: 'client.networkError' });
    expect(errorMessage(new Error('raw failure'))).not.toContain('raw failure');
  });
  it('serializes explicit writes and sends same-origin session credentials', async () => {
    let captured: RequestInit | undefined;
    const result = await apiRequest<{ authenticated: boolean }>('/api/login', { password: 'synthetic-password' }, async (_url, init) => {
      captured = init;
      return new Response(JSON.stringify({ authenticated: true }));
    });
    expect(result.authenticated).toBe(true);
    expect(captured).toMatchObject({ method: 'POST', credentials: 'same-origin', body: '{"password":"synthetic-password"}' });
  });
});
