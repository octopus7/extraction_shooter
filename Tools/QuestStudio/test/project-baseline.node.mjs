import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";
import test from "node:test";

const projectRoot = new URL("../", import.meta.url);

async function read(relativePath) {
  return readFile(new URL(relativePath, projectRoot), "utf8");
}

test("uses the Quest Flow Studio product identity", async () => {
  const [packageRaw, indexHtml, appSource, readme, deployScript] =
    await Promise.all([
      read("package.json"),
      read("index.html"),
      read("src/client/App.svelte"),
      read("README.md"),
      read("deploy.bat"),
    ]);
  const packageJson = JSON.parse(packageRaw);

  assert.equal(packageJson.name, "tunasweeper-quest-studio");
  assert.match(packageJson.description, /quest authoring/i);
  assert.match(indexHtml, /<title>Quest Flow Studio<\/title>/u);
  assert.match(indexHtml, /퀘스트 제작·편집·검증/u);
  assert.match(appSource, /<strong>Quest Flow Studio<\/strong>/u);
  assert.match(appSource, /<title>Quest Flow Studio · \{dataset\.title\}/u);
  assert.match(readme, /^# Quest Flow Studio$/mu);
  assert.match(readme, /Tools[\\/]QuestStudio/u);
  assert.match(deployScript, /Quest Flow Studio - Cloudflare Deploy/u);
  assert.doesNotMatch(deployScript, /Quest Flow Simulator/u);
});

test("generates Worker types before every canonical build", async () => {
  const packageJson = JSON.parse(await read("package.json"));

  assert.equal(
    packageJson.scripts.build,
    "npm run worker:types && npm run seed:generate && tsc --noEmit && vite build",
  );
  assert.equal(
    packageJson.scripts.verify,
    "npm run test:all && npm run build && wrangler deploy --dry-run",
  );
  assert.equal(
    packageJson.scripts["test:all"],
    "npm run test:project && vitest run && npm run test:typecheck",
  );
  assert.equal(
    packageJson.scripts["test:typecheck"],
    "npm run worker:types && tsc -p test/tsconfig.json --noEmit",
  );
  assert.equal(
    packageJson.scripts["test:project"],
    "node --test test/project-baseline.node.mjs",
  );
});
