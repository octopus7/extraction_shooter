import { t } from '../shared/ui-strings';

export class ClientError extends Error {
  constructor(public key: string, public params?: Record<string, string | number>) { super(key); }
}
export function errorMessage(error: unknown): string {
  if (error && typeof error === 'object' && 'key' in error && typeof error.key === 'string') {
    return t(error.key, 'params' in error ? error.params as Record<string, string | number> : undefined);
  }
  return t('client.unexpectedError');
}
export async function apiRequest<T>(url: string, body?: unknown, fetcher: typeof fetch = fetch): Promise<T> {
  let response: Response;
  try {
    response = await fetcher(url, {
      method: body === undefined ? 'GET' : 'POST',
      credentials: 'same-origin',
      ...(body === undefined ? {} : { headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(body) }),
    });
  } catch { throw new ClientError('client.networkError'); }
  let data: { error?: { key?: string; params?: Record<string, string | number> } };
  try { data = await response.json(); } catch { throw new ClientError('client.requestFailed'); }
  if (!response.ok) throw new ClientError(data?.error?.key ?? 'client.requestFailed', data?.error?.params);
  return data as T;
}
