import { createHash } from "node:crypto";
import { readFile, mkdir, writeFile } from "node:fs/promises";
import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const scriptDirectory = dirname(fileURLToPath(import.meta.url));
const projectRoot = resolve(scriptDirectory, "..");
const repositoryRoot = resolve(projectRoot, "..", "..");
const outputDirectory = resolve(projectRoot, "data");

const sources = {
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
    schemaVersion: 2,
    ...data,
  };
  const datasetHash = hashValue(catalogData);
  return {
    id,
    slug,
    title,
    visibility,
    schemaVersion: 2,
    datasetVersion: `content-${datasetHash.slice(0, 12)}`,
    sourceKind,
    sourceHash: `sha256:${sourceHash}`,
    sources: sourceNames.map((name) => relativeSourcePath(sources[name])),
    data: catalogData,
  };
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
  const places = [
    { id: "bunker", name: "벙커", mapId: "demo", shape: "circle", xMeters: 0, yMeters: 0, radiusMeters: 25 },
    {
      id: "tool-cache",
      name: "공구 보관 장소",
      mapId: "demo",
      shape: "point",
      xMeters: 10,
      yMeters: 35,
    },
    {
      id: "water-intake",
      name: "취수 시설",
      mapId: "demo",
      shape: "circle",
      radiusMeters: 18,
      xMeters: 0,
      yMeters: 160,
    },
    {
      id: "north-farm",
      name: "북쪽 파밍 지역",
      mapId: "demo",
      shape: "rectangle",
      widthMeters: 80,
      heightMeters: 60,
      xMeters: 320,
      yMeters: 0,
    },
    {
      id: "food-warehouse",
      name: "외부 식량 창고",
      mapId: "demo",
      shape: "rectangle",
      widthMeters: 100,
      heightMeters: 60,
      xMeters: 480,
      yMeters: 20,
    },
  ];
  const step = (
    id,
    questId,
    name,
    fromPlaceId,
    toPlaceId,
    actionSeconds,
  ) => ({
    id,
    questId,
    questTitle: titleById[questId],
    name,
    fromPlaceId,
    toPlaceId,
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
    questNodes: buildQuestNodes(quests, {
      xStart: 100,
      yStart: 100,
      columnGap: 190,
      rowGap: 130,
      columns: 20,
      existingColumns: 3,
      preserveExistingLayout: true,
    }),
    places,
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
        prerequisites: parsePrerequisiteQuestIds(row.prerequisiteText),
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
  const places = createInferredPlaces(quests);
  const mainQuestIds = new Set(
    quests.filter((quest) => /^M\d{2}$/.test(String(quest.id))).map((quest) => String(quest.id)),
  );
  const steps = quests.map((quest, index) => ({
    id: `step-${String(quest.id).toLowerCase()}`,
    questId: String(quest.id),
    questTitle: String(quest.title),
    name: String(quest.title),
    ...inferredRouteForQuest(
      quest,
      index,
      mainQuestIds.size > 0,
    ),
    actionSeconds,
    actionVariancePercent: 20,
    moveSpeedMps: 4,
    enabled: true,
  }));

  return {
    title,
    questNodes: buildQuestNodes(quests, {
      xStart: 100,
      yStart: 100,
      columnGap: 170,
      rowGap: 110,
      columns: 20,
      existingColumns: 5,
      preserveExistingLayout: true,
    }),
    places,
    steps,
    settings: {
      runs: 1_000,
      mapTransitionSeconds: 0,
    },
    sourceNotes: [
      "퀘스트 노드 좌표는 그래프 시각화용이며 맵 장소 좌표가 아닙니다.",
      "장소와 액터 데이터가 없는 항목은 퀘스트 문맥으로 추정한 임시 좌표입니다.",
      ...sourceNotes,
    ],
  };
}

function buildQuestNodes(quests, layout) {
  const questIds = quests.map((quest) => String(quest.id));
  const questIdSet = new Set(questIds);
  const prerequisitesById = new Map(
    quests.map((quest, index) => {
      const parsed =
        Array.isArray(quest.prerequisites) && quest.prerequisites.length > 0
          ? quest.prerequisites.map(String).filter((id) => questIdSet.has(id))
          : index === 0
            ? []
            : [questIds[index - 1]];
      return [String(quest.id), [...new Set(parsed)]];
    }),
  );
  const rankById = new Map();
  const visiting = new Set();

  const rankFor = (questId) => {
    if (rankById.has(questId)) return rankById.get(questId);
    if (visiting.has(questId)) return 0;
    visiting.add(questId);
    const rank = Math.max(
      0,
      ...(prerequisitesById.get(questId) ?? []).map(
        (prerequisiteId) => rankFor(prerequisiteId) + 1,
      ),
    );
    visiting.delete(questId);
    rankById.set(questId, rank);
    return rank;
  };

  const questsByRank = new Map();
  for (const questId of questIds) {
    const rank = rankFor(questId);
    const rankQuests = questsByRank.get(rank) ?? [];
    rankQuests.push(questId);
    questsByRank.set(rank, rankQuests);
  }

  const branchGap = layout.branchGap ?? Math.round(layout.rowGap * 0.8);
  const maxRank = Math.max(0, ...rankById.values());
  const rowCount = Math.floor(maxRank / layout.columns) + 1;
  const rowHeights = Array.from({ length: rowCount }, (_, row) => {
    const rankStart = row * layout.columns;
    const rankEnd = Math.min(maxRank, rankStart + layout.columns - 1);
    const largestBranch = Math.max(
      1,
      ...Array.from(
        { length: rankEnd - rankStart + 1 },
        (_, offset) => questsByRank.get(rankStart + offset)?.length ?? 1,
      ),
    );
    return layout.rowGap + ((largestBranch - 1) * branchGap) / 2;
  });
  const rowStarts = rowHeights.reduce((starts, _height, index) => {
    starts[index] =
      (starts[index - 1] ?? layout.yStart) +
      (index === 0 ? 0 : rowHeights[index - 1]);
    return starts;
  }, []);

  return quests.map((quest, index) => {
    const questId = String(quest.id);
    const rank = rankById.get(questId) ?? 0;
    const rankQuests = questsByRank.get(rank) ?? [questId];
    const branchIndex = rankQuests.indexOf(questId);
    const row = Math.floor(rank / layout.columns);
    const branchOffset = (branchIndex - (rankQuests.length - 1) / 2) * branchGap;
    const existingColumns = layout.existingColumns ?? layout.columns;
    const existingX =
      layout.xStart + (index % existingColumns) * layout.columnGap;
    const existingY =
      layout.yStart + Math.floor(index / existingColumns) * layout.rowGap;
    return {
      id: `quest-node-${String(quest.id).toLowerCase()}`,
      questId,
      title: String(quest.title),
      x: layout.preserveExistingLayout
        ? existingX
        : layout.xStart + (rank % layout.columns) * layout.columnGap,
      y: layout.preserveExistingLayout
        ? existingY
        : rowStarts[row] + branchOffset,
      prerequisiteQuestIds: prerequisitesById.get(questId) ?? [],
    };
  });
}

function parsePrerequisiteQuestIds(value) {
  const text = String(value ?? "");
  const rangeMatch = /M(\d{2})\s*[~〜-]\s*M(\d{2})/.exec(text);
  if (rangeMatch) {
    const start = Number(rangeMatch[1]);
    const end = Number(rangeMatch[2]);
    return Array.from(
      { length: Math.max(0, end - start + 1) },
      (_, index) => `M${String(start + index).padStart(2, "0")}`,
    );
  }
  return [...text.matchAll(/M\d{2}/g)].map((match) => match[0]);
}

function createInferredPlaces(quests) {
  const hasMainQuest = quests.some((quest) => /^M\d{2}$/.test(String(quest.id)));
  if (!hasMainQuest) {
    return [
      {
        id: "inferred-base",
        name: "추정 거점",
        mapId: "inferred",
        shape: "circle",
        xMeters: 0,
        yMeters: 0,
        radiusMeters: 25,
      },
      {
        id: "inferred-outdoor",
        name: "추정 외부 구역",
        mapId: "inferred",
        shape: "rectangle",
        xMeters: 240,
        yMeters: 30,
        widthMeters: 100,
        heightMeters: 70,
      },
      {
        id: "inferred-interaction",
        name: "추정 상호작용 지점",
        mapId: "inferred",
        shape: "point",
        xMeters: 420,
        yMeters: 60,
        actorId: "inferred.interaction",
      },
    ];
  }

  return [
    {
      id: "main-bunker",
      name: "벙커",
      mapId: "inferred-main",
      shape: "circle",
      xMeters: 0,
      yMeters: 0,
      radiusMeters: 28,
    },
    {
      id: "main-forest-trail",
      name: "서쪽 숲길",
      mapId: "inferred-main",
      shape: "rectangle",
      xMeters: 220,
      yMeters: 40,
      widthMeters: 130,
      heightMeters: 80,
    },
    {
      id: "main-concrete-blockage",
      name: "콘크리트 더미",
      mapId: "inferred-main",
      shape: "circle",
      xMeters: 300,
      yMeters: 180,
      radiusMeters: 20,
    },
    {
      id: "main-vending-machine",
      name: "수상한 자판기",
      mapId: "inferred-main",
      shape: "point",
      xMeters: 430,
      yMeters: 70,
      actorId: "inferred.vending_machine",
    },
    {
      id: "main-security-zone",
      name: "고급보안구역",
      mapId: "inferred-main",
      shape: "rectangle",
      xMeters: 570,
      yMeters: 50,
      widthMeters: 160,
      heightMeters: 120,
    },
    {
      id: "main-boss-zone",
      name: "보스구역",
      mapId: "inferred-main",
      shape: "circle",
      xMeters: 820,
      yMeters: 100,
      radiusMeters: 45,
    },
  ];
}

function inferredRouteForQuest(quest, index, isMainQuest) {
  if (isMainQuest) {
    const routeByQuestId = {
      M01: ["main-bunker", "main-bunker"],
      M02: ["main-bunker", "main-forest-trail"],
      M03: ["main-bunker", "main-bunker"],
      M04: ["main-bunker", "main-forest-trail"],
      M05: ["main-bunker", "main-forest-trail"],
      M06: ["main-forest-trail", "main-concrete-blockage"],
      M07: ["main-forest-trail", "main-concrete-blockage"],
      M08: ["main-forest-trail", "main-concrete-blockage"],
      M09: ["main-forest-trail", "main-vending-machine"],
      M10: ["main-vending-machine", "main-vending-machine"],
      M11: ["main-vending-machine", "main-vending-machine"],
      M12: ["main-vending-machine", "main-concrete-blockage"],
      M13: ["main-concrete-blockage", "main-security-zone"],
      M14: ["main-security-zone", "main-security-zone"],
      M15: ["main-security-zone", "main-security-zone"],
      M16: ["main-security-zone", "main-security-zone"],
      M17: ["main-bunker", "main-bunker"],
      M18: ["main-boss-zone", "main-boss-zone"],
      M19: ["main-bunker", "main-bunker"],
      M20: ["main-bunker", "main-bunker"],
    };
    const [fromPlaceId, toPlaceId] = routeByQuestId[String(quest.id)] ?? [
      "main-bunker",
      "main-bunker",
    ];
    return { fromPlaceId, toPlaceId };
  }

  const title = String(quest.title);
  if (/자판기|상호작용|interaction/i.test(title)) {
    return { fromPlaceId: "inferred-outdoor", toPlaceId: "inferred-interaction" };
  }
  if (/숲|외부|이동|취수|파밍|보스/i.test(title)) {
    return { fromPlaceId: "inferred-base", toPlaceId: "inferred-outdoor" };
  }
  return {
    fromPlaceId: index === 0 ? "inferred-base" : "inferred-outdoor",
    toPlaceId: index === 0 ? "inferred-base" : "inferred-outdoor",
  };
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
