import { createRemoteJWKSet, jwtVerify } from "jose";

export interface AuthenticatedUser {
  subject: string;
  email?: string;
  name?: string;
}

interface AccessBindings {
  ACCESS_TEAM_DOMAIN?: string;
  ACCESS_AUD?: string;
}

export function isAccessConfigured(env: AccessBindings): boolean {
  return Boolean(
    normalizeTeamDomain(env.ACCESS_TEAM_DOMAIN ?? "") &&
      env.ACCESS_AUD?.trim(),
  );
}

function normalizeTeamDomain(value: string): string | null {
  const trimmed = value.trim().toLowerCase();
  if (!trimmed) {
    return null;
  }

  try {
    const url = trimmed.includes("://")
      ? new URL(trimmed)
      : new URL(`https://${trimmed}`);
    if (
      url.protocol !== "https:" ||
      url.username ||
      url.password ||
      url.port ||
      url.pathname !== "/" ||
      url.search ||
      url.hash
    ) {
      return null;
    }
    return url.hostname;
  } catch {
    return null;
  }
}

function readAccessToken(request: Request): string | null {
  const headerToken = request.headers.get("cf-access-jwt-assertion")?.trim();
  if (headerToken) {
    return headerToken;
  }

  const cookie = request.headers.get("cookie");
  if (!cookie) {
    return null;
  }

  for (const part of cookie.split(";")) {
    const separator = part.indexOf("=");
    if (separator < 0) {
      continue;
    }
    const name = part.slice(0, separator).trim();
    if (name !== "CF_Authorization") {
      continue;
    }
    const value = part.slice(separator + 1).trim();
    if (!value) {
      return null;
    }
    try {
      return decodeURIComponent(value);
    } catch {
      return null;
    }
  }

  return null;
}

export async function authenticateAccessRequest(
  request: Request,
  env: AccessBindings,
): Promise<AuthenticatedUser | null> {
  const domain = normalizeTeamDomain(env.ACCESS_TEAM_DOMAIN ?? "");
  const audience = env.ACCESS_AUD?.trim();

  // Private data is fail-closed until both Access settings exist.
  if (!domain || !audience) {
    return null;
  }

  const token = readAccessToken(request);
  if (!token || token.length > 16_384) {
    return null;
  }

  try {
    const issuer = `https://${domain}`;
    const jwks = createRemoteJWKSet(
      new URL(`${issuer}/cdn-cgi/access/certs`),
    );
    const { payload } = await jwtVerify(token, jwks, {
      issuer,
      audience,
    });

    if (typeof payload.sub !== "string" || payload.sub.length === 0) {
      return null;
    }

    return {
      subject: payload.sub,
      email:
        typeof payload.email === "string" ? payload.email : undefined,
      name: typeof payload.name === "string" ? payload.name : undefined,
    };
  } catch {
    return null;
  }
}
