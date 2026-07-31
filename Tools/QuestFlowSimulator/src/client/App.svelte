<script lang="ts">
  import { onMount } from "svelte";
  import {
    createWorkspace,
    getCatalog,
    getCatalogs,
    getSession,
    getWorkspaces,
    updateWorkspace,
  } from "./api";
  import type {
    CatalogSummary,
    LocationNode,
    QuestDataset,
    QuestStep,
    Session,
    SimulationResult,
    ViewMode,
    Workspace,
  } from "../shared/types";

  const EMPTY_SESSION: Session = { authenticated: false };
  const FALLBACK_DATASET: QuestDataset = {
    schemaVersion: 1,
    title: "체험판 동선",
    locations: [
      { id: "dock", name: "선착장", xMeters: 10, yMeters: 22 },
      { id: "village", name: "마을 광장", xMeters: 62, yMeters: 32 },
      { id: "warehouse", name: "창고", xMeters: 118, yMeters: 68 },
      { id: "bunker", name: "벙커", xMeters: 164, yMeters: 32 },
    ],
    steps: [
      {
        id: "q1",
        questId: "Q1",
        questTitle: "취수 시설 확인",
        name: "취수 시설 확인",
        fromLocationId: "dock",
        toLocationId: "village",
        actionSeconds: 28,
        actionVariancePercent: 20,
        moveSpeedMps: 3.8,
        enabled: true,
      },
      {
        id: "q2",
        questId: "Q2",
        questTitle: "보급품 획득",
        name: "보급품 획득",
        fromLocationId: "village",
        toLocationId: "warehouse",
        actionSeconds: 42,
        actionVariancePercent: 20,
        moveSpeedMps: 3.8,
        enabled: true,
      },
      {
        id: "q4",
        questId: "Q4",
        questTitle: "벙커 귀환",
        name: "벙커 귀환",
        fromLocationId: "warehouse",
        toLocationId: "bunker",
        actionSeconds: 64,
        actionVariancePercent: 20,
        moveSpeedMps: 3.8,
        enabled: true,
      },
    ],
    settings: { runs: 10_000 },
  };

  let mode: ViewMode = "desktop";
  let session = EMPTY_SESSION;
  let catalogs: CatalogSummary[] = [];
  let selectedCatalog: CatalogSummary | null = null;
  let dataset: QuestDataset = structuredClone(FALLBACK_DATASET);
  let selectedLocationId: string | null = dataset.locations[0]?.id ?? null;
  let selectedStepId: string | null = dataset.steps[0]?.id ?? null;
  let workspaces: Workspace[] = [];
  let currentWorkspace: Workspace | null = null;
  let iterations = 10_000;
  let seed = 240731;
  let result: SimulationResult | null = null;
  let running = false;
  let loading = true;
  let saveState = "브라우저 임시저장";
  let statusMessage = "";
  let activeProTab: "quests" | "properties" | "results" = "properties";
  let dragLocationId: string | null = null;
  let simulator: Worker | null = null;

  $: selectedLocation =
    dataset.locations.find((location) => location.id === selectedLocationId) ??
    null;
  $: selectedStep =
    dataset.steps.find((step) => step.id === selectedStepId) ?? null;
  $: canvasBounds = calculateBounds(dataset.locations);

  onMount(async () => {
    const savedMode = localStorage.getItem("quest-flow:view-mode");
    if (savedMode === "desktop" || savedMode === "pro") mode = savedMode;

    simulator = new Worker(new URL("./simulation.worker.ts", import.meta.url), {
      type: "module",
    });
    simulator.onmessage = (event: MessageEvent<SimulationResult>) => {
      result = event.data;
      running = false;
      activeProTab = "results";
    };
    simulator.onerror = () => {
      running = false;
      statusMessage = "시뮬레이션을 완료하지 못했습니다.";
    };

    try {
      const [sessionValue, catalogValues] = await Promise.all([
        getSession(),
        getCatalogs(),
      ]);
      session = sessionValue;
      catalogs = catalogValues;
      if (session.authenticated) {
        workspaces = await getWorkspaces().catch(() => []);
      }
      if (catalogs.length > 0) {
        await selectCatalog(catalogs[0]);
      } else {
        statusMessage = "공개 catalog가 없어 내장 체험 데이터를 표시합니다.";
      }
    } catch (error) {
      statusMessage =
        error instanceof Error
          ? `서버 연결 실패 · 내장 체험 데이터를 사용합니다. (${error.message})`
          : "서버 연결 실패 · 내장 체험 데이터를 사용합니다.";
    } finally {
      loading = false;
      runSimulation();
    }

    return () => simulator?.terminate();
  });

  function calculateBounds(locations: LocationNode[]) {
    if (locations.length === 0) return { minX: 0, minY: 0, spanX: 100, spanY: 100 };
    const xs = locations.map((location) => location.xMeters);
    const ys = locations.map((location) => location.yMeters);
    const minX = Math.min(...xs);
    const minY = Math.min(...ys);
    return {
      minX,
      minY,
      spanX: Math.max(30, Math.max(...xs) - minX),
      spanY: Math.max(30, Math.max(...ys) - minY),
    };
  }

  function xOnCanvas(value: number) {
    return 80 + ((value - canvasBounds.minX) / canvasBounds.spanX) * 840;
  }

  function yOnCanvas(value: number) {
    return 60 + ((value - canvasBounds.minY) / canvasBounds.spanY) * 500;
  }

  function locationById(id: string) {
    return dataset.locations.find((location) => location.id === id);
  }

  function setMode(value: ViewMode) {
    mode = value;
    localStorage.setItem("quest-flow:view-mode", value);
  }

  function localKey() {
    return `quest-flow:draft:${selectedCatalog?.slug ?? "demo"}`;
  }

  function persistDraft() {
    localStorage.setItem(localKey(), JSON.stringify(dataset));
    saveState = `브라우저 저장 · ${new Date().toLocaleTimeString("ko-KR", {
      hour: "2-digit",
      minute: "2-digit",
    })}`;
  }

  function replaceDataset(next: QuestDataset) {
    dataset = structuredClone(next);
    selectedLocationId = dataset.locations[0]?.id ?? null;
    selectedStepId = dataset.steps[0]?.id ?? null;
    result = null;
  }

  async function selectCatalog(catalog: CatalogSummary) {
    loading = true;
    statusMessage = "";
    try {
      const detail = await getCatalog(catalog.slug);
      selectedCatalog = detail.catalog;
      const saved = localStorage.getItem(
        `quest-flow:draft:${detail.catalog.slug}`,
      );
      replaceDataset(
        saved ? (JSON.parse(saved) as QuestDataset) : detail.dataset,
      );
      const workspace = workspaces.find(
        (value) => value.catalogId === detail.catalog.id,
      );
      currentWorkspace = workspace ?? null;
      saveState = saved ? "브라우저 초안 복원됨" : "catalog 불러옴";
      runSimulation();
    } catch (error) {
      statusMessage =
        error instanceof Error ? error.message : "catalog를 불러오지 못했습니다.";
    } finally {
      loading = false;
    }
  }

  function updateLocation(
    id: string,
    patch: Partial<Pick<LocationNode, "name" | "xMeters" | "yMeters">>,
  ) {
    dataset = {
      ...dataset,
      locations: dataset.locations.map((location) =>
        location.id === id ? { ...location, ...patch } : location,
      ),
    };
    persistDraft();
  }

  function updateStep(id: string, patch: Partial<QuestStep>) {
    dataset = {
      ...dataset,
      steps: dataset.steps.map((step) =>
        step.id === id ? { ...step, ...patch } : step,
      ),
    };
    persistDraft();
  }

  function handlePointerMove(event: PointerEvent) {
    if (!dragLocationId) return;
    const svg = event.currentTarget as SVGSVGElement;
    const rect = svg.getBoundingClientRect();
    const canvasX = ((event.clientX - rect.left) / rect.width) * 1000;
    const canvasY = ((event.clientY - rect.top) / rect.height) * 620;
    const x =
      canvasBounds.minX +
      ((Math.min(920, Math.max(80, canvasX)) - 80) / 840) *
        canvasBounds.spanX;
    const y =
      canvasBounds.minY +
      ((Math.min(560, Math.max(60, canvasY)) - 60) / 500) *
        canvasBounds.spanY;
    updateLocation(dragLocationId, {
      xMeters: Math.round(x * 10) / 10,
      yMeters: Math.round(y * 10) / 10,
    });
  }

  function runSimulation() {
    if (!simulator || running) return;
    running = true;
    statusMessage = "";
    simulator.postMessage({
      dataset,
      options: {
        iterations: Math.min(100_000, Math.max(1, Math.floor(iterations))),
        seed: Math.floor(seed),
      },
    });
  }

  async function saveWorkspace() {
    if (!session.authenticated) {
      statusMessage = "로그인 후 D1 작업공간에 저장할 수 있습니다.";
      return;
    }
    if (!selectedCatalog) {
      statusMessage = "먼저 catalog를 선택하세요.";
      return;
    }
    saveState = "D1 저장 중…";
    try {
      currentWorkspace = currentWorkspace
        ? await updateWorkspace(currentWorkspace, dataset)
        : await createWorkspace(dataset.title, selectedCatalog.id, dataset);
      workspaces = [
        ...workspaces.filter((item) => item.id !== currentWorkspace?.id),
        currentWorkspace,
      ];
      saveState = "D1 저장 완료";
    } catch (error) {
      saveState = "D1 저장 실패";
      statusMessage =
        error instanceof Error ? error.message : "작업공간을 저장하지 못했습니다.";
    }
  }

  function formatTime(seconds: number) {
    if (!Number.isFinite(seconds)) return "—";
    const minutes = Math.floor(seconds / 60);
    const rest = Math.round(seconds % 60);
    return minutes > 0 ? `${minutes}분 ${rest}초` : `${rest}초`;
  }
</script>

<svelte:head>
  <title>{dataset.title} · Quest Flow</title>
</svelte:head>

<main class:pro={mode === "pro"} class="app-shell">
  <header class="topbar">
    <div class="brand">
      <span class="brand-mark">QF</span>
      <div>
        <strong>Quest Flow</strong>
        <small>{selectedCatalog?.datasetVersion ?? "browser simulation"}</small>
      </div>
    </div>

    <div class="project">
      <span class="eyebrow">CATALOG</span>
      <strong>{selectedCatalog?.title ?? dataset.title}</strong>
    </div>

    <div class="top-actions">
      <span class:online={session.authenticated} class="session">
        {session.authenticated ? session.displayName ?? "로그인됨" : "체험 모드"}
      </span>
      <span class="save-state">{saveState}</span>
      <div class="mode-switch" aria-label="화면 모드">
        <button
          class:active={mode === "desktop"}
          onclick={() => setMode("desktop")}
        >Desktop</button>
        <button class:active={mode === "pro"} onclick={() => setMode("pro")}
        >Pro</button>
      </div>
      <button class="primary" disabled={running} onclick={runSimulation}>
        {running ? "계산 중…" : "시뮬레이션"}
      </button>
    </div>
  </header>

  {#if statusMessage}
    <div class="notice" role="status">
      <span>{statusMessage}</span>
      <button aria-label="알림 닫기" onclick={() => (statusMessage = "")}>×</button>
    </div>
  {/if}

  <section class="workspace">
    <aside class="catalog-panel">
      <div class="panel-heading">
        <div>
          <span class="eyebrow">QUEST CATALOG</span>
          <h2>퀘스트</h2>
        </div>
        <span class="count">{catalogs.length || 1}</span>
      </div>

      {#if catalogs.length}
        <nav class="catalog-list" aria-label="퀘스트 catalog">
          {#each catalogs as catalog}
            <button
              class:active={catalog.slug === selectedCatalog?.slug}
              onclick={() => selectCatalog(catalog)}
            >
              <span class="visibility">
                {catalog.visibility === "public" ? "DEMO" : "MEMBER"}
              </span>
              <strong>{catalog.title}</strong>
              <small>{catalog.description ?? catalog.slug}</small>
            </button>
          {/each}
        </nav>
      {:else}
        <div class="catalog-list">
          <button class="active">
            <span class="visibility">DEMO</span>
            <strong>Q1–Q4 체험판</strong>
            <small>내장 예시 데이터</small>
          </button>
        </div>
      {/if}

      <div class="step-list">
        <span class="eyebrow">FLOW STEPS</span>
        {#each dataset.steps as step, index}
          <button
            class:active={step.id === selectedStepId}
            onclick={() => {
              selectedStepId = step.id;
              activeProTab = "properties";
            }}
          >
            <span>{String(index + 1).padStart(2, "0")}</span>
            <div>
              <strong>{step.name}</strong>
              <small>
                {locationById(step.fromLocationId)?.name ?? "?"} →
                {locationById(step.toLocationId)?.name ?? "?"}
              </small>
            </div>
          </button>
        {/each}
      </div>
    </aside>

    <section class="canvas-panel">
      <div class="canvas-toolbar">
        <div>
          <span class="eyebrow">ROUTE MAP · METERS</span>
          <h1>{dataset.title}</h1>
        </div>
        <div class="legend">
          <span><i class="dot location-dot"></i>장소</span>
          <span><i class="line"></i>이동</span>
        </div>
      </div>

      <div class="map-frame">
        {#if loading}
          <div class="loading">catalog 불러오는 중…</div>
        {/if}
        <svg
          viewBox="0 0 1000 620"
          aria-label="장소와 퀘스트 이동 동선 편집기"
          onpointermove={handlePointerMove}
          onpointerup={() => (dragLocationId = null)}
          onpointerleave={() => (dragLocationId = null)}
        >
          <defs>
            <pattern id="smallGrid" width="20" height="20" patternUnits="userSpaceOnUse">
              <path d="M 20 0 L 0 0 0 20" fill="none" stroke="#1c2b3d" stroke-width="1" />
            </pattern>
            <pattern id="grid" width="100" height="100" patternUnits="userSpaceOnUse">
              <rect width="100" height="100" fill="url(#smallGrid)" />
              <path d="M 100 0 L 0 0 0 100" fill="none" stroke="#29405a" stroke-width="1.2" />
            </pattern>
            <marker id="arrow" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse">
              <path d="M 0 0 L 10 5 L 0 10 z" fill="#50d2a0" />
            </marker>
          </defs>
          <rect width="1000" height="620" fill="url(#grid)" />

          {#each dataset.steps.filter((step) => step.enabled) as step}
            {@const from = locationById(step.fromLocationId)}
            {@const to = locationById(step.toLocationId)}
            {#if from && to}
              <g
                class:active={step.id === selectedStepId}
                class="route"
                onclick={() => (selectedStepId = step.id)}
                role="button"
                tabindex="0"
              >
                <line
                  x1={xOnCanvas(from.xMeters)}
                  y1={yOnCanvas(from.yMeters)}
                  x2={xOnCanvas(to.xMeters)}
                  y2={yOnCanvas(to.yMeters)}
                  marker-end="url(#arrow)"
                />
                <text
                  x={(xOnCanvas(from.xMeters) + xOnCanvas(to.xMeters)) / 2}
                  y={(yOnCanvas(from.yMeters) + yOnCanvas(to.yMeters)) / 2 - 10}
                >
                  {Math.hypot(
                    to.xMeters - from.xMeters,
                    to.yMeters - from.yMeters,
                  ).toFixed(1)} m
                </text>
              </g>
            {/if}
          {/each}

          {#each dataset.locations as location}
            <g
              class:active={location.id === selectedLocationId}
              class="location"
              transform={`translate(${xOnCanvas(location.xMeters)} ${yOnCanvas(location.yMeters)})`}
              onpointerdown={(event) => {
                (event.currentTarget as SVGGElement).setPointerCapture(
                  event.pointerId,
                );
                dragLocationId = location.id;
                selectedLocationId = location.id;
                activeProTab = "properties";
              }}
              role="button"
              tabindex="0"
            >
              <circle r="24" />
              <circle class="core" r="7" />
              <text y="44">{location.name}</text>
              <text class="coordinate" y="62">
                {location.xMeters.toFixed(1)}, {location.yMeters.toFixed(1)}
              </text>
            </g>
          {/each}
        </svg>
        <div class="map-hint">드래그로 위치 이동 · 좌표 단위 1m</div>
      </div>
    </section>

    <aside class="properties-panel">
      <div class="tabs">
        <button
          class:active={activeProTab === "quests"}
          onclick={() => (activeProTab = "quests")}
        >퀘스트</button>
        <button
          class:active={activeProTab === "properties"}
          onclick={() => (activeProTab = "properties")}
        >속성</button>
        <button
          class:active={activeProTab === "results"}
          onclick={() => (activeProTab = "results")}
        >결과</button>
      </div>

      <div class:hidden={activeProTab !== "properties"} class="property-content">
        <div class="panel-heading compact">
          <div>
            <span class="eyebrow">INSPECTOR</span>
            <h2>속성 편집</h2>
          </div>
        </div>

        {#if selectedLocation}
          <fieldset>
            <legend>장소</legend>
            <label>
              <span>이름</span>
              <input
                value={selectedLocation.name}
                oninput={(event) =>
                  updateLocation(selectedLocation!.id, {
                    name: event.currentTarget.value,
                  })}
              />
            </label>
            <div class="field-row">
              <label>
                <span>X (m)</span>
                <input
                  type="number"
                  step="0.1"
                  value={selectedLocation.xMeters}
                  oninput={(event) =>
                    updateLocation(selectedLocation!.id, {
                      xMeters: event.currentTarget.valueAsNumber || 0,
                    })}
                />
              </label>
              <label>
                <span>Y (m)</span>
                <input
                  type="number"
                  step="0.1"
                  value={selectedLocation.yMeters}
                  oninput={(event) =>
                    updateLocation(selectedLocation!.id, {
                      yMeters: event.currentTarget.valueAsNumber || 0,
                    })}
                />
              </label>
            </div>
          </fieldset>
        {/if}

        {#if selectedStep}
          <fieldset>
            <legend>행동 단계</legend>
            <label>
              <span>이름</span>
              <input
                value={selectedStep.name}
                oninput={(event) =>
                  updateStep(selectedStep!.id, { name: event.currentTarget.value })}
              />
            </label>
            <div class="field-row">
              <label>
                <span>행동 시간 (초)</span>
                <input
                  type="number"
                  min="0"
                  step="1"
                  value={selectedStep.actionSeconds}
                  oninput={(event) =>
                    updateStep(selectedStep!.id, {
                      actionSeconds: event.currentTarget.valueAsNumber || 0,
                    })}
                />
              </label>
              <label>
                <span>행동 시간 오차 ±%</span>
                <input
                  type="number"
                  min="0"
                  step="1"
                  value={selectedStep.actionVariancePercent}
                  oninput={(event) =>
                    updateStep(selectedStep!.id, {
                      actionVariancePercent:
                        event.currentTarget.valueAsNumber || 0,
                    })}
                />
              </label>
            </div>
            <label>
              <span>이동 속도 (m/s)</span>
              <input
                type="number"
                min="0.1"
                step="0.1"
                value={selectedStep.moveSpeedMps}
                oninput={(event) =>
                  updateStep(selectedStep!.id, {
                    moveSpeedMps: event.currentTarget.valueAsNumber || 0.1,
                  })}
              />
            </label>
            <label class="toggle">
              <input
                type="checkbox"
                checked={selectedStep.enabled}
                onchange={(event) =>
                  updateStep(selectedStep!.id, {
                    enabled: event.currentTarget.checked,
                  })}
              />
              <span>시뮬레이션에 포함</span>
            </label>
          </fieldset>
        {/if}
      </div>

      <div class:hidden={activeProTab !== "quests"} class="property-content quests-mobile">
        <span class="eyebrow">FLOW STEPS</span>
        {#each dataset.steps as step, index}
          <button
            class:active={step.id === selectedStepId}
            onclick={() => {
              selectedStepId = step.id;
              activeProTab = "properties";
            }}
          >
            <span>{index + 1}</span>
            <strong>{step.name}</strong>
          </button>
        {/each}
      </div>

      <div class:hidden={activeProTab !== "results"} class="property-content results-mobile">
        <span class="eyebrow">LATEST RESULT</span>
        {#if result}
          <div class="mobile-metrics">
            <strong>{formatTime(result.averageSeconds)}</strong>
            <span>P10 {formatTime(result.p10Seconds)}</span>
            <span>P90 {formatTime(result.p90Seconds)}</span>
            <span>{result.totalDistanceMeters.toFixed(1)}m</span>
          </div>
        {:else}
          <p>시뮬레이션을 실행하세요.</p>
        {/if}
      </div>

      <div class="save-box">
        <div>
          <span class="eyebrow">PERSISTENCE</span>
          <strong>{session.authenticated ? "D1 작업공간" : "브라우저 초안"}</strong>
        </div>
        <button class="secondary" onclick={persistDraft}>임시저장</button>
        <button
          class="primary"
          disabled={!session.authenticated}
          onclick={saveWorkspace}
        >D1 저장</button>
      </div>
    </aside>
  </section>

  <section class="result-panel">
    <div class="run-settings">
      <div>
        <span class="eyebrow">MONTE CARLO</span>
        <h2>시뮬레이션 결과</h2>
      </div>
      <label>
        <span>반복</span>
        <input type="number" min="1" max="100000" step="1000" bind:value={iterations} />
      </label>
      <label>
        <span>시드</span>
        <input type="number" bind:value={seed} />
      </label>
    </div>

    <div class="metrics">
      <article class="featured">
        <span>평균 플레이 시간</span>
        <strong>{result ? formatTime(result.averageSeconds) : "—"}</strong>
        <small>{result?.iterations.toLocaleString() ?? 0}회 기준</small>
      </article>
      <article>
        <span>P10</span>
        <strong>{result ? formatTime(result.p10Seconds) : "—"}</strong>
        <small>빠른 진행</small>
      </article>
      <article>
        <span>P90</span>
        <strong>{result ? formatTime(result.p90Seconds) : "—"}</strong>
        <small>느린 진행</small>
      </article>
      <article>
        <span>총 이동 거리</span>
        <strong>{result ? `${result.totalDistanceMeters.toFixed(1)} m` : "—"}</strong>
        <small>직선 거리 근사</small>
      </article>
      <article>
        <span>이동 / 행동</span>
        <strong>
          {result
            ? `${formatTime(result.averageTravelSeconds)} / ${formatTime(result.averageActionSeconds)}`
            : "—"}
        </strong>
        <small>평균 시간 구성</small>
      </article>
    </div>
  </section>
</main>
