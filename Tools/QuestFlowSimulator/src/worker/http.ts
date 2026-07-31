import type { Context } from "hono";
import { z } from "zod";

export const MAX_REQUEST_BYTES = 262_144;

export class ApiError extends Error {
  constructor(
    public readonly status: 400 | 401 | 403 | 404 | 409 | 413 | 415 | 422 | 500,
    public readonly code: string,
    message: string,
    public readonly details?: unknown,
  ) {
    super(message);
  }
}

export function requestId(request: Request): string {
  return request.headers.get("cf-ray") ?? crypto.randomUUID();
}

export function apiErrorResponse(
  c: Context,
  error: ApiError,
  id: string,
): Response {
  return c.json(
    {
      error: {
        code: error.code,
        message: error.message,
        requestId: id,
        ...(error.details === undefined ? {} : { details: error.details }),
      },
    },
    error.status,
  );
}

export async function readValidatedJson<T>(
  request: Request,
  schema: z.ZodType<T>,
): Promise<T> {
  const contentType = request.headers.get("content-type")?.toLowerCase() ?? "";
  if (!contentType.startsWith("application/json")) {
    throw new ApiError(
      415,
      "unsupported_media_type",
      "Content-Type must be application/json.",
    );
  }

  const declaredLength = Number(request.headers.get("content-length"));
  if (
    Number.isFinite(declaredLength) &&
    declaredLength > MAX_REQUEST_BYTES
  ) {
    throw new ApiError(413, "payload_too_large", "Request body is too large.");
  }

  if (!request.body) {
    throw new ApiError(422, "invalid_json", "A JSON request body is required.");
  }

  const reader = request.body.getReader();
  const chunks: Uint8Array[] = [];
  let total = 0;

  try {
    while (true) {
      const { done, value } = await reader.read();
      if (done) {
        break;
      }
      total += value.byteLength;
      if (total > MAX_REQUEST_BYTES) {
        await reader.cancel();
        throw new ApiError(
          413,
          "payload_too_large",
          "Request body is too large.",
        );
      }
      chunks.push(value);
    }
  } finally {
    reader.releaseLock();
  }

  const bytes = new Uint8Array(total);
  let offset = 0;
  for (const chunk of chunks) {
    bytes.set(chunk, offset);
    offset += chunk.byteLength;
  }

  let value: unknown;
  try {
    value = JSON.parse(new TextDecoder("utf-8", { fatal: true }).decode(bytes));
  } catch {
    throw new ApiError(422, "invalid_json", "Request body is not valid JSON.");
  }

  const result = schema.safeParse(value);
  if (!result.success) {
    throw new ApiError(
      422,
      "validation_failed",
      "Request body failed validation.",
      result.error.issues.map((issue) => ({
        path: issue.path.join("."),
        message: issue.message,
      })),
    );
  }
  return result.data;
}

export function parseStoredJson(value: string): unknown {
  try {
    return JSON.parse(value);
  } catch {
    throw new ApiError(
      500,
      "invalid_stored_json",
      "Stored data could not be decoded.",
    );
  }
}
