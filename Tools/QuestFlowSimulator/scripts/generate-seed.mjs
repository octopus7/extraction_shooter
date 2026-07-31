import { createHash } from "node:crypto";
import { readFile, mkdir, writeFile } from "node:fs/promises";
import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const scriptDirectory = dirname(fileURLToPath(import.meta.url));
const projectRoot = resolve(scriptDirectory, "..");
const repositoryRoot = resolve(projectRoot, "..", "..");
const outputDirectory = resolve(projectRoot, "data");

const sources = {
  runtimeDefinitions: resolve(
    repositoryRoot,
    "TunaSweeper",
    "Content",
    "Data",
    "QuestDefinitions.json",
  ),
  runtimeStrings: resolve(
    repositoryRoot,
    "TunaSweeper",
    "Content",
    "Data",
    "QuestTextStrings.csv",
  ),
  mainSsot: resolve(
    repositoryRoot,
    "Docs",
    "SSOT",
    "TunaSweeper_SSOT_Quest_Item_v0.6.md",
  ),
  demoTree: resolve(
    repositoryRoot,
    "Docs",
    "DemoDesign",
    "02_퀘스트_진행_트리.md",
  ),
  demoRoute: resolve(
    repositoryRoot,
    "Docs",
    "DemoDesign",
    "05_맵_세션_동선_설계.md",
  ),
};

const sourceText = Object.fromEntries(
  await Promise.all(
    Object.entries(sources).map(async ([name, path]) => [
      name,
      normalizeNewlines(await readFile(path, "utf8")),
    ]),
  ),
);

const runtimeDefinitions = JSON.parse(sourceText.runtimeDefinitions);
if (!Array.isArray(runtimeDefinitions)) {
  throw new Error("QuestDefinitions.json must contain a top-level array.");
}

const translations = parseCsv(sourceText.runtimeStrings);
const runtimeCatalog = buildRuntimeCatalog(runtimeDefinitions, translations);
const demoCatalog = buildDemoCatalog(
  sourceText.demoTree,
  sourceText.demoRoute,
);
const mainCatalog = buildMainCatalog(sourceText.mainSsot);

const catalogs = [
  makeCatalog({
    id: "catalog-demo-q1-q4",
    slug: "demo",
    title: "체험판 Q1~Q4",
    visibility: "public",
    sourceKind: "demo-design-markdown",
    sourceNames: ["demoTree", "demoRoute"],
    data: demoCatalog,
  }),
  makeCatalog({
    id: "catalog-main-m01-m20",
    slug: "main-m01-m20",
    title: "본편 메인 M01~M20",
    visibility: "authenticated",
    sourceKind: "ssot-markdown",
    sourceNames: ["mainSsot"],
    data: mainCatalog,
  }),
  makeCatalog({
    id: "catalog-runtime-snapshot",
    slug: "runtime-snapshot",
    title: "UE5 현재 구현 스냅샷",
    visibility: "authenticated",
    sourceKind: "ue5-json-csv",
    sourceNames: ["runtimeDefinitions", "runtimeStrings"],
    data: runtimeCatalog,
  }),
];

await mkdir(outputDirectory, { recursive: true });
await Promise.all(
  catalogs.map((catalog) =>
    writeStableJson(resolve(outputDirectory, `${catalog.slug}.json`), catalog),
  ),
);
await writeFile(
  resolve(outputDirectory, "seed.sql"),
  renderSeedSql(catalogs),
  "utf8",
);

for (const catalog of catalogs) {
  console.log(
    `${catalog.slug}: ${catalog.datasetVersion} (${catalog.data.steps.length} simulation steps)`,
  );
}

function makeCatalog({
  id,
  slug,
  title,
  visibility,
  sourceKind,
  sourceNames,
  data,
}) {
  const sourceHash = hashSources(sourceNames);
  const catalogData = {
    schemaVersion: 1,
    ...data,
  };
  const datasetHash = hashValue(catalogData);
  return {
    id,
    slug,
    title,
    visibility,
    schemaVersion: 1,
    datasetVersion: `content-${datasetHash.slice(0, 12)}`,
    sourceKind,
    sourceHash: `sha256:${sourceHash}`,
    sources: sourceNames.map((name) => relativeSourcePath(sources[name])),
    data: catalogData,
  };
}

function buildRuntimeCatalog(definitions, stringRows) {
  const stringsByKey = new Map(
    stringRows.map((row) => [row.string_key, row]),
  );
  const referencedKeys = new Set();

  for (const quest of definitions) {
    collectStringKeys(quest, referencedKeys);
  }

  const missingStringKeys = [...referencedKeys]
    .filter((key) => !stringsByKey.has(key))
    .sort();
  if (missingStringKeys.length > 0) {
    throw new Error(
      `QuestDefinitions.json references missing string keys: ${missingStringKeys.join(", ")}`,
    );
  }

  const quests = definitions
    .map((quest) => ({
      ...quest,
      localized: {
        title: localize(stringsByKey, quest.title_string_key),
        description: localize(stringsByKey, quest.description_string_key),
      },
    }))
    .sort(
      (left, right) =>
        Number(left.sort_order ?? 0) - Number(right.sort_order ?? 0) ||
        String(left.quest_id).localeCompare(String(right.quest_id)),
    );

  const sourceNotes = quests.map((quest) => {
    const title = quest.localized.title?.ko || quest.quest_id;
    const objectiveIds = Array.isArray(quest.objectives)
      ? quest.objectives.map((objective) => objective.objective_id).join(", ")
      : "";
    return `${quest.quest_id} ${title}: ${objectiveIds || "목표 없음"}`;
  });

  return buildApproximateSimulationCatalog(
    "UE5 현재 구현 스냅샷",
    quests.map((quest) => ({
      id: String(quest.quest_id),
      title: quest.localized.title?.ko || String(quest.quest_id),
    })),
    sourceNotes,
    120,
  );
}

function buildDemoCatalog(treeMarkdown, routeMarkdown) {
  const sections = parseHeadingSections(treeMarkdown, 2)
    .filter((section) => /^Q(?:[1-4]|3-[12])\./.test(section.heading))
    .map((section) => {
      const match = /^(Q(?:[1-4]|3-[12]))\.\s*(.+)$/.exec(section.heading);
      if (!match) {
        throw new Error(`Invalid demo quest heading: ${section.heading}`);
      }
      const subsections = parseHeadingSections(section.body, 3);
      return {
        id: match[1].toLowerCase(),
        title: match[2].trim(),
        startConditions: listItems(
          subsectionBody(subsections, "시작 조건"),
        ),
        objectives: listItems(subsectionBody(subsections, "목표")),
        completionConditions: listItems(
          subsectionBody(subsections, "완료 조건"),
        ),
        results: listItems(
          subsectionBody(subsections, "결과") ||
            subsectionBody(subsections, "엔딩"),
        ),
      };
    });

  const expectedIds = ["q1", "q2", "q3-1", "q3-2", "q4"];
  if (questsMismatch(sections, expectedIds)) {
    throw new Error(
      `Demo quest IDs must be exactly ${expectedIds.join(", ")}.`,
    );
  }

  const prerequisites = {
    q1: [],
    q2: ["q1"],
    "q3-1": ["q2"],
    "q3-2": ["q2"],
    q4: ["q3-1", "q3-2"],
  };
  const quests = sections.map((quest) => ({
    ...quest,
    prerequisites: prerequisites[quest.id],
  }));

  const routeSections = parseHeadingSections(routeMarkdown, 2);
  const titleById = Object.fromEntries(
    quests.map((quest) => [quest.id, quest.title]),
  );
  const locations = [
    { id: "bunker", name: "벙커", mapId: "demo", xMeters: 0, yMeters: 0 },
    {
      id: "tool-cache",
      name: "공구 보관 장소",
      mapId: "demo",
      xMeters: 10,
      yMeters: 35,
    },
    {
      id: "water-intake",
      name: "취수 시설",
      mapId: "demo",
      xMeters: 0,
      yMeters: 160,
    },
    {
      id: "north-farm",
      name: "북쪽 파밍 지역",
      mapId: "demo",
      xMeters: 320,
      yMeters: 0,
    },
    {
      id: "food-warehouse",
      name: "외부 식량 창고",
      mapId: "demo",
      xMeters: 480,
      yMeters: 20,
    },
  ];
  const step = (
    id,
    questId,
    name,
    fromLocationId,
    toLocationId,
    actionSeconds,
  ) => ({
    id,
    questId,
    questTitle: titleById[questId],
    name,
    fromLocationId,
    toLocationId,
    actionSeconds,
    actionVariancePercent: 15,
    moveSpeedMps: 4,
    enabled: true,
  });
  const steps = [
    step("q1-outbound", "q1", "취수 시설로 이동 및 조사", "bunker", "water-intake", 20),
    step("q1-return", "q1", "벙커로 귀환", "water-intake", "bunker", 10),
    step("q2-tool", "q2", "크로우바 획득", "bunker", "tool-cache", 15),
    step("q2-clean", "q2", "스크린 청소", "tool-cache", "water-intake", 25),
    step("q2-return", "q2", "벙커로 귀환", "water-intake", "bunker", 10),
    step("q3-1-part", "q3-1", "교체용 손잡이 획득", "bunker", "north-farm", 30),
    step("q3-1-repair", "q3-1", "취수 시설 수리", "north-farm", "water-intake", 25),
    step("q3-1-return", "q3-1", "벙커로 귀환", "water-intake", "bunker", 10),
    step("q3-2-tape", "q3-2", "방수 테이프 획득", "bunker", "north-farm", 25),
    step("q3-2-repair", "q3-2", "벙커 배관 수리", "north-farm", "bunker", 25),
    step("q4-outbound", "q4", "식량 창고에서 참치캔 획득", "bunker", "food-warehouse", 30),
    step("q4-return", "q4", "참치캔을 가지고 귀환", "food-warehouse", "bunker", 30),
  ];

  const routeNotes = [
    ...plainLines(subsectionBody(routeSections, "기본 맵 형태")),
    ...plainLines(subsectionBody(routeSections, "거리 기준")),
    "좌표는 문서의 벙커→취수 시설 단순 이동 약 40초를 4m/s로 환산한 근사치이며 UE5 실제 좌표가 아니다.",
  ];
  for (const quest of quests) {
    routeNotes.push(
      `${quest.id.toUpperCase()} ${quest.title}: ${quest.objectives.join(" / ")}`,
    );
  }

  return {
    title: "체험판 Q1~Q4",
    locations,
    steps,
    settings: {
      runs: 1_000,
      mapTransitionSeconds: 0,
    },
    sourceNotes: routeNotes,
  };
}

function buildMainCatalog(markdown) {
  const tableRows = [...markdown.matchAll(/^\|\s*(M\d{2})\s*\|(.+)$/gm)]
    .map((match) => {
      const columns = match[2].split("|").map((value) => value.trim());
      if (columns.length < 5) {
        throw new Error(`Malformed main quest row: ${match[0]}`);
      }
      return {
        id: match[1],
        title: columns[0],
        category: columns[1],
        prerequisiteText: columns[2],
        requiredItemsText: columns[3],
        rewardText: columns[4],
      };
    })
    .filter((row, index, rows) => rows.findIndex((item) => item.id === row.id) === index);

  const expectedIds = Array.from(
    { length: 20 },
    (_, index) => `M${String(index + 1).padStart(2, "0")}`,
  );
  if (questsMismatch(tableRows, expectedIds)) {
    throw new Error("SSOT main quest table must contain M01 through M20 once.");
  }

  const detailSections = new Map(
    parseHeadingSections(markdown, 3)
      .filter((section) => /^M\d{2}\./.test(section.heading))
      .map((section) => [
        section.heading.slice(0, 3),
        parseLabelledBullets(section.body),
      ]),
  );

  const quests = tableRows.map((row) => {
      const details = detailSections.get(row.id);
      if (!details) {
        throw new Error(`Missing SSOT detail section for ${row.id}.`);
      }
      return {
        ...row,
        description: details["내용"] ?? "",
        detailCategory: details["카테고리"] ?? row.category,
        detailRequiredItems:
          details["요구 아이템"] ?? row.requiredItemsText,
        detailReward: details["보상"] ?? row.rewardText,
        prerequisiteRule:
          details["선행조건"] ?? row.prerequisiteText,
      };
    });

  return buildApproximateSimulationCatalog(
    "본편 메인 M01~M20",
    quests,
    quests.map(
      (quest) =>
        `${quest.id} ${quest.title} | 선행: ${quest.prerequisiteRule} | 내용: ${quest.description} | 요구: ${quest.detailRequiredItems} | 보상: ${quest.detailReward}`,
    ),
    540,
  );
}

function buildApproximateSimulationCatalog(
  title,
  quests,
  sourceNotes,
  actionSeconds,
) {
  const origin = {
    id: "route-origin",
    name: "기준 시작점",
    mapId: "approximation",
    xMeters: 0,
    yMeters: 0,
  };
  const questLocations = quests.map((quest, index) => ({
    id: `location-${String(quest.id).toLowerCase()}`,
    name: `${quest.id} ${quest.title}`,
    mapId: "approximation",
    xMeters: 80 + Math.floor(index / 4) * 120,
    yMeters: (index % 4) * 90,
  }));
  const locations = [origin, ...questLocations];
  const steps = quests.map((quest, index) => ({
    id: `step-${String(quest.id).toLowerCase()}`,
    questId: String(quest.id),
    questTitle: String(quest.title),
    name: String(quest.title),
    fromLocationId: index === 0 ? origin.id : questLocations[index - 1].id,
    toLocationId: questLocations[index].id,
    actionSeconds,
    actionVariancePercent: 20,
    moveSpeedMps: 4,
    enabled: true,
  }));

  return {
    title,
    locations,
    steps,
    settings: {
      runs: 1_000,
      mapTransitionSeconds: 0,
    },
    sourceNotes: [
      "위치는 실제 UE5 좌표가 아닌 퀘스트 순서 시각화를 위한 근사 배치입니다.",
      ...sourceNotes,
    ],
  };
}

function parseCsv(text) {
  const rows = [];
  let row = [];
  let field = "";
  let quoted = false;

  for (let index = 0; index < text.length; index += 1) {
    const character = text[index];
    if (quoted) {
      if (character === '"' && text[index + 1] === '"') {
        field += '"';
        index += 1;
      } else if (character === '"') {
        quoted = false;
      } else {
        field += character;
      }
    } else if (character === '"') {
      quoted = true;
    } else if (character === ",") {
      row.push(field);
      field = "";
    } else if (character === "\n") {
      row.push(field);
      if (row.some((value) => value.length > 0)) {
        rows.push(row);
      }
      row = [];
      field = "";
    } else {
      field += character;
    }
  }
  row.push(field);
  if (row.some((value) => value.length > 0)) {
    rows.push(row);
  }
  if (quoted || rows.length === 0) {
    throw new Error("QuestTextStrings.csv is malformed.");
  }

  const headers = rows[0];
  return rows.slice(1).map((values) =>
    Object.fromEntries(headers.map((header, index) => [header, values[index] ?? ""])),
  );
}

function collectStringKeys(value, keys) {
  if (Array.isArray(value)) {
    for (const item of value) {
      collectStringKeys(item, keys);
    }
    return;
  }
  if (!value || typeof value !== "object") {
    return;
  }
  for (const [key, child] of Object.entries(value)) {
    if (key.endsWith("_string_key") && typeof child === "string") {
      keys.add(child);
    } else {
      collectStringKeys(child, keys);
    }
  }
}

function localize(stringsByKey, key) {
  if (typeof key !== "string") {
    return null;
  }
  const row = stringsByKey.get(key);
  return row
    ? { ko: row.ko ?? "", en: row.en ?? "", ja: row.ja ?? "" }
    : null;
}

function parseHeadingSections(markdown, level) {
  const marker = "#".repeat(level);
  const expression = new RegExp(
    `^${marker} (.+)\\n([\\s\\S]*?)(?=^${marker} |\\s*$)`,
    "gm",
  );
  return [...markdown.matchAll(expression)].map((match) => ({
    heading: match[1].trim(),
    body: match[2].trim(),
  }));
}

function subsectionBody(sections, heading) {
  return sections.find((section) => section.heading === heading)?.body ?? "";
}

function listItems(text) {
  return text
    .split("\n")
    .map((line) => line.trim())
    .filter((line) => /^(?:[-*]|\d+\.)\s+/.test(line))
    .map((line) => line.replace(/^(?:[-*]|\d+\.)\s+/, "").trim());
}

function plainLines(text) {
  return text
    .split("\n")
    .map((line) => line.trim())
    .filter((line) => line && !line.startsWith("```"))
    .map((line) => line.replace(/^[-*]\s+/, "").trim());
}

function parseLabelledBullets(text) {
  const values = {};
  for (const match of text.matchAll(
    /^-\s+\*\*([^*]+):\*\*\s*(.+)$/gm,
  )) {
    values[match[1].trim()] = match[2].trim();
  }
  return values;
}

function questsMismatch(quests, expectedIds) {
  return (
    quests.length !== expectedIds.length ||
    quests.some((quest, index) => quest.id !== expectedIds[index])
  );
}

function hashSources(sourceNames) {
  const hash = createHash("sha256");
  for (const name of sourceNames) {
    hash.update(`${name}\0`);
    hash.update(sourceText[name]);
    hash.update("\0");
  }
  return hash.digest("hex");
}

function hashValue(value) {
  return createHash("sha256").update(JSON.stringify(value)).digest("hex");
}

function relativeSourcePath(path) {
  return path
    .slice(repositoryRoot.length + 1)
    .replaceAll("\\", "/");
}

function normalizeNewlines(text) {
  return text.replace(/\r\n?/g, "\n").replace(/^\uFEFF/, "");
}

async function writeStableJson(path, value) {
  await writeFile(path, `${JSON.stringify(value, null, 2)}\n`, "utf8");
}

function sqlString(value) {
  return `'${String(value).replaceAll("'", "''")}'`;
}

function renderSeedSql(catalogValues) {
  const statements = catalogValues.map((catalog) => {
    const dataJson = JSON.stringify(catalog.data);
    return `INSERT INTO quest_catalogs (
  id, slug, title, visibility, schema_version, dataset_version,
  source_kind, source_hash, data_json
) VALUES (
  ${sqlString(catalog.id)},
  ${sqlString(catalog.slug)},
  ${sqlString(catalog.title)},
  ${sqlString(catalog.visibility)},
  ${catalog.schemaVersion},
  ${sqlString(catalog.datasetVersion)},
  ${sqlString(catalog.sourceKind)},
  ${sqlString(catalog.sourceHash)},
  ${sqlString(dataJson)}
)
ON CONFLICT(slug) DO UPDATE SET
  title = excluded.title,
  visibility = excluded.visibility,
  schema_version = excluded.schema_version,
  dataset_version = excluded.dataset_version,
  source_kind = excluded.source_kind,
  source_hash = excluded.source_hash,
  data_json = excluded.data_json,
  updated_at = CURRENT_TIMESTAMP;`;
  });

  return [
    "-- Generated by scripts/generate-seed.mjs. Do not edit by hand.",
    "PRAGMA foreign_keys = ON;",
    ...statements,
    "",
  ].join("\n\n");
}
