export const MAX_REQUEST_BYTES = 8 * 1024 * 1024;
export class ApiError extends Error {
  constructor(readonly status: number, readonly key: string, readonly params?: Record<string, string | number>, readonly headers?: HeadersInit) { super(key); }
}
export function json(value: unknown, status = 200, headers?: HeadersInit): Response {
  const responseHeaders = new Headers(headers);
  responseHeaders.set('cache-control', 'no-store');
  responseHeaders.set('x-content-type-options', 'nosniff');
  return Response.json(value, { status, headers: responseHeaders });
}
export function requireSameOrigin(request: Request): void {
  const origin = request.headers.get('origin');
  if ((origin && origin !== new URL(request.url).origin) || request.headers.get('sec-fetch-site') === 'cross-site') {
    throw new ApiError(403, 'api.forbidden_origin');
  }
}
export async function readJson(request: Request): Promise<unknown> {
  const mediaType = (request.headers.get('content-type') ?? '').split(';')[0]?.trim().toLowerCase();
  if (mediaType !== 'application/json') throw new ApiError(415, 'api.unsupported_media_type');
  if (Number(request.headers.get('content-length')) > MAX_REQUEST_BYTES) throw new ApiError(413, 'api.payload_too_large');
  if (!request.body) throw new ApiError(422, 'api.invalid_json');
  const reader = request.body.getReader(); const chunks: Uint8Array[] = []; let size = 0;
  try {
    while (true) {
      const { done, value } = await reader.read(); if (done) break;
      size += value.byteLength;
      if (size > MAX_REQUEST_BYTES) { await reader.cancel(); throw new ApiError(413, 'api.payload_too_large'); }
      chunks.push(value);
    }
  } finally { reader.releaseLock(); }
  const bytes = new Uint8Array(size); let offset = 0;
  for (const chunk of chunks) { bytes.set(chunk, offset); offset += chunk.length; }
  try { return JSON.parse(new TextDecoder('utf-8', { fatal: true }).decode(bytes)); }
  catch { throw new ApiError(422, 'api.invalid_json'); }
}
