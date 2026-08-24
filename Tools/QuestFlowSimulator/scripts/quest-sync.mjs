import { createHash } from "node:crypto";
import { spawnSync } from "node:child_process";
import {
  mkdir,
  readFile,
  rename,
  rm,
  stat,
  writeFile,
} from "node:fs/promises";
import { homedir } from "node:os";
import { dirname, join, resolve } from "node:path";
import process from "node:process";
import { fileURLToPath } from "node:url";

const scriptDir = dirname(fileURLToPath(import.meta.url));
const toolRoot = resolve(scriptDir, "..");
const repoRoot = resolve(toolRoot, "..", "..");
const baseUrl = (process.env.QUEST_SYNC_URL || "https://quest.oc7.workers.dev").replace(/\/$/u, "");
const jsonOutput = process.argv.includes("--json");
const command = process.argv[2] || "help";
const flavor = parseFlavor(option("--flavor") || "Demo");

const config = flavor === "Demo"
  ? {
      flavor,
      slug: "demo",
      questDefinitions: join(repoRoot, "TunaSweeper", "Content", "Data", "QuestDefinitions.json"),
      questTextStrings: join(repoRoot, "TunaSweeper", "Content", "Data", "QuestTextStrings.csv"),
      editor: join(toolRoot, "data", "demo-editor.json"),
      editorFallback: join(toolRoot, "data", "demo.json"),
      manifest: join(toolRoot, ".quest-sync", "demo.json"),
    }
  : {
      flavor,
      slug: "main-m01-m20",
      questDefinitions: join(repoRoot, "TunaSweeper", "External", "MainPayload", "Data", "QuestDefinitions.json"),
      questTextStrings: join(repoRoot, "TunaSweeper", "External", "MainPayload", "Data", "QuestTextStrings.csv"),
      editor: join(repoRoot, "TunaSweeper", "External", "MainPayload", "Editor", "QuestFlow.json"),
      editorFallback: null,
      manifest: join(repoRoot, "TunaSweeper", "External", "MainPayload", "Editor", ".quest-sync.json"),
    };

try {
  if (command === "token:set") await setProtectedToken();
  else if (command === "status") await statusCommand();
  else if (command === "pull") await pullCommand();
  else if (command === "validate") await validateCommand();
  else if (command === "push") await pushCommand();
  else printHelp();
} catch (error) {
  const message = error instanceof Error ? error.message : String(error);
  if (jsonOutput) console.error(JSON.stringify({ ok: false, error: message }));
  else console.error(`[quest-sync] ${message}`);
  process.exitCode = 1;
}

async function statusCommand() {
  const remote = await api(`/api/sync/${config.slug}/status`, { token: await loadToken(false) });
  const local = await loadLocalPack(false);
  const manifest = await readJsonIfExists(config.manifest);
  const localHash = local ? hashPack(local) : null;
  const state = !local
    ? "missing-local-copy"
    : manifest?.baseDatasetVersion !== remote.status.datasetVersion
      ? "pull-required"
      : localHash === manifest?.localContentHash
        ? "up-to-date"
        : "local-changes";
  output({
    ok: true,
    flavor,
    state,
    remoteDatasetVersion: remote.status.datasetVersion,
    baseDatasetVersion: manifest?.baseDatasetVersion ?? null,
    localContentHash: localHash,
  });
}

async function pullCommand() {
  const token = await loadToken(flavor === "Main");
  const value = await api(`/api/sync/${config.slug}/current`, { token });
  const pack = normalizePack(value.release.pack);
  if (pack.flavor !== flavor) throw new Error(`Remote flavor is ${pack.flavor}, expected ${flavor}.`);
  validatePack(pack);
  await Promise.all([
    atomicWrite(config.questDefinitions, `${JSON.stringify(pack.runtime.questDefinitions, null, 2)}\n`),
    atomicWrite(config.questTextStrings, pack.runtime.questTextStringsCsv),
    atomicWrite(config.editor, `${JSON.stringify(pack.editor, null, 2)}\n`),
  ]);
  await atomicWrite(
    config.manifest,
    `${JSON.stringify({
      schemaVersion: 1,
      flavor,
      slug: config.slug,
      baseDatasetVersion: value.release.datasetVersion,
      pulledContentHash: value.release.contentHash,
      localContentHash: hashPack(pack),
      pulledAt: new Date().toISOString(),
    }, null, 2)}\n`,
  );
  output({ ok: true, flavor, datasetVersion: value.release.datasetVersion, contentHash: value.release.contentHash });
}

async function validateCommand() {
  const pack = await loadLocalPack(true);
  validatePack(pack);
  output({ ok: true, flavor, contentHash: hashPack(pack), questCount: pack.runtime.questDefinitions.length });
}

async function pushCommand() {
  const token = await loadToken(true);
  const manifest = await readJsonIfExists(config.manifest);
  if (!manifest?.baseDatasetVersion) {
    throw new Error(`No sync base for ${flavor}. Run quest:pull before quest:push.`);
  }
  const pack = await loadLocalPack(true);
  validatePack(pack);
  const value = await api(`/api/sync/${config.slug}/publish`, {
    token,
    method: "POST",
    body: {
      expectedBaseDatasetVersion: manifest.baseDatasetVersion,
      pack,
      summary: option("--summary") || "Published by Codex quest-sync CLI",
    },
  });
  await atomicWrite(
    config.manifest,
    `${JSON.stringify({
      ...manifest,
      baseDatasetVersion: value.release.datasetVersion,
      pulledContentHash: value.release.contentHash,
      localContentHash: hashPack(pack),
      pulledAt: new Date().toISOString(),
    }, null, 2)}\n`,
  );
  output({ ok: true, flavor, datasetVersion: value.release.datasetVersion, contentHash: value.release.contentHash });
}

async function loadLocalPack(required) {
  const [definitions, csv, editorSource] = await Promise.all([
    readJsonIfExists(config.questDefinitions),
    readTextIfExists(config.questTextStrings),
    readJsonIfExists(config.editor),
  ]);
  let editor = editorSource;
  if (!editor && config.editorFallback) {
    const fallback = await readJsonIfExists(config.editorFallback);
    editor = fallback?.data ?? fallback;
  }
  if (!Array.isArray(definitions) || csv === null || !editor) {
    if (required) throw new Error(`Local ${flavor} authoring copy is incomplete. Run quest:pull first.`);
    return null;
  }
  return normalizePack({
    schemaVersion: 3,
    flavor,
    runtime: { questDefinitions: definitions, questTextStringsCsv: csv },
    editor,
  });
}

function normalizePack(value) {
  if (!value || value.schemaVersion !== 3 || !["Demo", "Main"].includes(value.flavor)) {
    throw new Error("Unsupported quest authoring pack.");
  }
  return {
    schemaVersion: 3,
    flavor: value.flavor,
    runtime: {
      questDefinitions: value.runtime?.questDefinitions,
      questTextStringsCsv: String(value.runtime?.questTextStringsCsv ?? "")
        .replace(/^\uFEFF/u, "")
        .replaceAll("\r\n", "\n")
        .replaceAll("\r", "\n"),
    },
    editor: value.editor,
  };
}

function validatePack(pack) {
  const issues = [];
  if (pack.flavor !== flavor) issues.push(`flavor must be ${flavor}`);
  if (!Array.isArray(pack.runtime.questDefinitions)) issues.push("runtime.questDefinitions must be an array");
  if (!pack.editor || !Array.isArray(pack.editor.questNodes) || !Array.isArray(pack.editor.places) || !Array.isArray(pack.editor.steps)) {
    issues.push("editor dataset is invalid");
  }
  const runtimeIds = new Set();
  for (const [index, quest] of (pack.runtime.questDefinitions || []).entries()) {
    const id = quest?.quest_id ?? quest?.questId;
    if (typeof id !== "string" || !id.trim()) issues.push(`runtime.questDefinitions.${index}: quest_id is required`);
    else if (runtimeIds.has(id)) issues.push(`duplicate runtime quest_id ${id}`);
    else runtimeIds.add(id);
  }
  for (const quest of pack.runtime.questDefinitions || []) {
    const id = quest?.quest_id ?? quest?.questId;
    const prerequisites = quest?.required_completed_quest_ids ?? quest?.requiredCompletedQuestIds ?? [];
    for (const prerequisite of prerequisites) {
      if (!runtimeIds.has(prerequisite)) issues.push(`${id}: unknown runtime prerequisite ${prerequisite}`);
    }
  }
  if (issues.length) throw new Error(`Quest pack validation failed:\n- ${issues.join("\n- ")}`);
}

async function api(path, { token, method = "GET", body } = {}) {
  const response = await fetch(`${baseUrl}${path}`, {
    method,
    headers: {
      ...(token ? { authorization: `Bearer ${token}` } : {}),
      ...(body ? { "content-type": "application/json" } : {}),
    },
    ...(body ? { body: JSON.stringify(body) } : {}),
  });
  const value = await response.json().catch(() => null);
  if (!response.ok) {
    const message = value?.error?.message || `${response.status} ${response.statusText}`;
    const current = value?.error?.details?.currentDatasetVersion;
    throw new Error(current ? `${message} Current version: ${current}` : message);
  }
  return value;
}

function hashPack(pack) {
  return createHash("sha256").update(stableStringify(pack), "utf8").digest("hex");
}

function stableStringify(value) {
  return JSON.stringify(sortValue(value));
}

function sortValue(value) {
  if (Array.isArray(value)) return value.map(sortValue);
  if (!value || typeof value !== "object") return value;
  return Object.fromEntries(Object.entries(value).sort(([a], [b]) => a.localeCompare(b)).map(([key, child]) => [key, sortValue(child)]));
}

async function setProtectedToken() {
  if (process.platform !== "win32") {
    throw new Error("token:set currently supports Windows DPAPI. Use QUEST_SYNC_TOKEN on other platforms.");
  }
  const tokenFile = protectedTokenPath();
  await mkdir(dirname(tokenFile), { recursive: true });
  const commandText = [
    "$secret = Read-Host 'Codex sync token' -AsSecureString",
    "$secret | ConvertFrom-SecureString | Set-Content -LiteralPath $env:QUEST_SYNC_TOKEN_FILE -Encoding utf8",
  ].join("; ");
  const result = spawnSync("powershell.exe", ["-NoProfile", "-Command", commandText], {
    stdio: "inherit",
    env: { ...process.env, QUEST_SYNC_TOKEN_FILE: tokenFile },
  });
  if (result.status !== 0) throw new Error("Failed to store the token with Windows DPAPI.");
  output({ ok: true, stored: true });
}

async function loadToken(required) {
  if (process.env.QUEST_SYNC_TOKEN) return process.env.QUEST_SYNC_TOKEN.trim();
  if (process.platform !== "win32") {
    if (required) throw new Error("Set QUEST_SYNC_TOKEN before using this command.");
    return null;
  }
  const tokenFile = protectedTokenPath();
  if (!(await exists(tokenFile))) {
    if (required) throw new Error("No Codex sync token. Run npm run quest:token:set first.");
    return null;
  }
  const commandText = [
    "$secure = Get-Content -LiteralPath $env:QUEST_SYNC_TOKEN_FILE -Raw | ConvertTo-SecureString",
    "$ptr = [Runtime.InteropServices.Marshal]::SecureStringToBSTR($secure)",
    "try { [Runtime.InteropServices.Marshal]::PtrToStringBSTR($ptr) } finally { [Runtime.InteropServices.Marshal]::ZeroFreeBSTR($ptr) }",
  ].join("; ");
  const result = spawnSync("powershell.exe", ["-NoProfile", "-Command", commandText], {
    encoding: "utf8",
    env: { ...process.env, QUEST_SYNC_TOKEN_FILE: tokenFile },
  });
  if (result.status !== 0) throw new Error("Failed to read the Windows-protected sync token.");
  const token = result.stdout.trim();
  return token || null;
}

function protectedTokenPath() {
  const appData = process.env.APPDATA || join(homedir(), "AppData", "Roaming");
  return join(appData, "TunaSweeper", "QuestSync", "token.txt");
}

async function atomicWrite(path, content) {
  await mkdir(dirname(path), { recursive: true });
  const temporary = `${path}.${process.pid}.tmp`;
  await writeFile(temporary, content, "utf8");
  await rm(path, { force: true });
  await rename(temporary, path);
}

async function readJsonIfExists(path) {
  const text = await readTextIfExists(path);
  return text === null ? null : JSON.parse(text);
}

async function readTextIfExists(path) {
  try { return await readFile(path, "utf8"); }
  catch (error) { if (error?.code === "ENOENT") return null; throw error; }
}

async function exists(path) {
  try { await stat(path); return true; }
  catch (error) { if (error?.code === "ENOENT") return false; throw error; }
}

function option(name) {
  const index = process.argv.indexOf(name);
  return index >= 0 ? process.argv[index + 1] : null;
}

function parseFlavor(value) {
  const normalized = value.toLowerCase();
  if (normalized === "demo") return "Demo";
  if (normalized === "main") return "Main";
  throw new Error("--flavor must be Demo or Main.");
}

function output(value) {
  if (jsonOutput) console.log(JSON.stringify(value));
  else console.log(Object.entries(value).map(([key, child]) => `${key}: ${child}`).join("\n"));
}

function printHelp() {
  console.log(`Quest authoring sync\n\nCommands:\n  status --flavor Demo|Main [--json]\n  pull --flavor Demo|Main\n  validate --flavor Demo|Main\n  push --flavor Demo|Main [--summary text]\n  token:set\n`);
}
