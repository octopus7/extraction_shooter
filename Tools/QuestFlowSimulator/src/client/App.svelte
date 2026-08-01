<script lang="ts">
  import { onMount } from "svelte";
  import demoCatalogSource from "../../data/demo.json";
  import {
    createWorkspace,
    getCatalog,
    getCatalogs,
    getSession,
    getWorkspaces,
    loginAdmin,
    logoutAdmin,
    updateWorkspace,
  } from "./api";
  import { readCatalogDraft, serializeCatalogDraft } from "./draft";
  import { normalizeQuestDataset } from "../shared/dataset";
  import type {
    CanvasMode,
    CatalogSummary,
    MapPlace,
    PlaceShape,
    QuestDataset,
    QuestNode,
    QuestStep,
    Session,
    SimulationResult,
    ViewMode,
    Workspace,
  } from "../shared/types";

  const EMPTY_SESSION: Session = {
    authenticated: false,
    authConfigured: false,
  };
  // Generated from Docs/DemoDesign by scripts/generate-seed.mjs.
  // Keeping the public fallback on the same source prevents invented map data.
  const FALLBACK_DATASET: QuestDataset = demoCatalogSource.data;

  let mode: ViewMode = "desktop";
  let session = EMPTY_SESSION;
  let catalogs: CatalogSummary[] = [];
  let selectedCatalog: CatalogSummary | null = null;
  let dataset: QuestDataset = structuredClone(FALLBACK_DATASET);
  let canvasMode: CanvasMode = "quest-chain";
  let selectedQuestId: string | null = dataset.questNodes[0]?.questId ?? null;
  let selectedPlaceId: string | null = dataset.places[0]?.id ?? null;
  let selectedStepId: string | null = dataset.steps[0]?.id ?? null;
  let canvasBounds = calculateCanvasBounds(canvasMode, dataset);
  let canvasWidth = 1000;
  let canvasHeight = 620;
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
  let dragQuestNodeId: string | null = null;
  let dragPlaceId: string | null = null;
  let dragPan = false;
  let panX = 0;
  let panY = 0;
  let panStartClientX = 0;
  let panStartClientY = 0;
  let panOriginX = 0;
  let panOriginY = 0;
  let dragOffsetX = 0;
  let dragOffsetY = 0;
  let zoom = 1;
  let simulator: Worker | null = null;
  let loginOpen = false;
  let loginBusy = false;
  let loginError = "";
  let adminPassword = "";

  $: selectedQuestNode =
    dataset.questNodes.find((node) => node.questId === selectedQuestId) ?? null;
  $: selectedPlace =
    dataset.places.find((place) => place.id === selectedPlaceId) ?? null;
  $: selectedStep =
    dataset.steps.find((step) => step.id === selectedStepId) ?? null;
  $: selectedQuestSteps = dataset.steps.filter(
    (step) => step.questId === selectedQuestId && step.enabled,
  );
  $: selectedRouteSteps = selectedQuestSteps.filter(
    (step) => step.fromPlaceId !== step.toPlaceId,
  );
  $: graphEdges = dataset.questNodes.flatMap((node) =>
    node.prerequisiteQuestIds.map((prerequisiteQuestId) => ({
      id: `${prerequisiteQuestId}-${node.questId}`,
      from: dataset.questNodes.find(
        (candidate) => candidate.questId === prerequisiteQuestId,
      ),
      to: node,
    })),
  ).filter((edge) => edge.from);
  $: canvasWidth = Math.min(6000, Math.max(1000, Math.ceil(canvasBounds.spanX + 160)));
  $: canvasHeight = Math.min(3000, Math.max(620, Math.ceil(canvasBounds.spanY + 120)));
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

  function calculateBounds(points: Array<{ x: number; y: number }>) {
    if (points.length === 0) return { minX: 0, minY: 0, spanX: 100, spanY: 100 };
    const xs = points.map((point) => point.x);
    const ys = points.map((point) => point.y);
    const minX = Math.min(...xs);
    const minY = Math.min(...ys);
    const rawSpanX = Math.max(30, Math.max(...xs) - minX);
    const rawSpanY = Math.max(30, Math.max(...ys) - minY);
    const paddingX = Math.max(30, rawSpanX * 0.12);
    const paddingY = Math.max(30, rawSpanY * 0.12);
    return {
      minX: minX - paddingX,
      minY: minY - paddingY,
      spanX: rawSpanX + paddingX * 2,
      spanY: rawSpanY + paddingY * 2,
    };
  }

  function calculateCanvasBounds(value: CanvasMode, source: QuestDataset) {
    const points =
      value === "quest-chain"
        ? source.questNodes.map((node) => ({ x: node.x, y: node.y }))
        : source.places.map((place) => ({ x: place.xMeters, y: place.yMeters }));
    return calculateBounds(points);
  }

  function autoLayoutQuestNodes(nodes: QuestNode[]) {
    const columns = Math.min(20, Math.max(1, nodes.length));
    const xStart = 100;
    const yStart = 100;
    const columnGap = 170;
    const rowGap = 110;
    const branchGap = 88;
    const nodeIds = nodes.map((node) => node.questId);
    const nodeIdSet = new Set(nodeIds);
    const prerequisitesById = new Map(
      nodes.map((node, index) => {
        const prerequisites = node.prerequisiteQuestIds.filter((id) =>
          nodeIdSet.has(id),
        );
        return [
          node.questId,
          [...new Set(prerequisites.length > 0 || index === 0 ? prerequisites : [nodeIds[index - 1]])],
        ];
      }),
    );
    const rankById = new Map<string, number>();
    const visiting = new Set<string>();

    const rankFor = (questId: string): number => {
      if (rankById.has(questId)) return rankById.get(questId)!;
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

    const nodesByRank = new Map<number, string[]>();
    for (const questId of nodeIds) {
      const rank = rankFor(questId);
      const rankNodes = nodesByRank.get(rank) ?? [];
      rankNodes.push(questId);
      nodesByRank.set(rank, rankNodes);
    }

    const maxRank = Math.max(0, ...rankById.values());
    const rowCount = Math.floor(maxRank / columns) + 1;
    const rowHeights = Array.from({ length: rowCount }, (_, row) => {
      const rankStart = row * columns;
      const rankEnd = Math.min(maxRank, rankStart + columns - 1);
      const largestBranch = Math.max(
        1,
        ...Array.from(
          { length: rankEnd - rankStart + 1 },
          (_, offset) => nodesByRank.get(rankStart + offset)?.length ?? 1,
        ),
      );
      return rowGap + ((largestBranch - 1) * branchGap) / 2;
    });
    const rowStarts = rowHeights.reduce<number[]>((starts, _height, index) => {
      starts[index] =
        (starts[index - 1] ?? yStart) +
        (index === 0 ? 0 : rowHeights[index - 1]);
      return starts;
    }, []);

    return nodes.map((node) => {
      const rank = rankById.get(node.questId) ?? 0;
      const rankNodes = nodesByRank.get(rank) ?? [node.questId];
      const branchIndex = rankNodes.indexOf(node.questId);
      const row = Math.floor(rank / columns);
      return {
        ...node,
        x: xStart + (rank % columns) * columnGap,
        y:
          rowStarts[row] +
          (branchIndex - (rankNodes.length - 1) / 2) * branchGap,
        coordinatesAutoGenerated: undefined,
      };
    });
  }

  function xOnCanvas(value: number) {
    return 80 + ((value - canvasBounds.minX) / canvasBounds.spanX) * (canvasWidth - 160);
  }

  function yOnCanvas(value: number) {
    return 60 + ((value - canvasBounds.minY) / canvasBounds.spanY) * (canvasHeight - 120);
  }

  function widthOnCanvas(value: number) {
    return Math.max(8, (value / canvasBounds.spanX) * (canvasWidth - 160));
  }

  function heightOnCanvas(value: number) {
    return Math.max(8, (value / canvasBounds.spanY) * (canvasHeight - 120));
  }

  function questNodeById(id: string) {
    return dataset.questNodes.find((node) => node.questId === id);
  }

  function hasIncomingQuestEdge(node: QuestNode) {
    return graphEdges.some((edge) => edge.to.id === node.id);
  }

  function hasOutgoingQuestEdge(node: QuestNode) {
    return graphEdges.some((edge) => edge.from?.id === node.id);
  }

  function questEdgePath(from: QuestNode, to: QuestNode) {
    const nodePort = 64;
    const startX = xOnCanvas(from.x) + nodePort;
    const startY = yOnCanvas(from.y);
    const endX = xOnCanvas(to.x) - nodePort;
    const endY = yOnCanvas(to.y);
    const horizontalDistance = endX - startX;

    if (horizontalDistance >= 0) {
      const curve = Math.min(
        120,
        Math.max(32, horizontalDistance * 0.35),
      );
      return `M ${startX} ${startY} C ${startX + curve} ${startY}, ${endX - curve} ${endY}, ${endX} ${endY}`;
    }

    // A wrapped edge goes around the right side of the source/target pair so
    // the return trip cannot cut through a card in the next row.
    const outerX =
      Math.max(startX, endX) + Math.max(70, Math.abs(endY - startY) * 0.35);
    return `M ${startX} ${startY} C ${outerX} ${startY}, ${outerX} ${endY}, ${endX} ${endY}`;
  }

  function placeById(id: string) {
    return dataset.places.find((place) => place.id === id);
  }

  function setCanvasMode(value: CanvasMode) {
    canvasMode = value;
    // Fit once when changing views. Do not derive this from live drag coordinates:
    // recalculating during a drag makes the graph continuously rescale and shrink.
    canvasBounds = calculateCanvasBounds(value, dataset);
    zoom = 1;
    panX = 0;
    panY = 0;
    dragQuestNodeId = null;
    dragPlaceId = null;
    dragPan = false;
    activeProTab = "properties";
  }

  function handleWheel(event: WheelEvent) {
    event.preventDefault();
    const nextZoom = zoom * (event.deltaY < 0 ? 1.12 : 0.89);
    zoom = Math.min(3, Math.max(0.5, Math.round(nextZoom * 100) / 100));
  }

  function resetZoom() {
    zoom = 1;
  }

  function handleCanvasPointerDown(event: PointerEvent) {
    const target = event.currentTarget as SVGRectElement;
    target.setPointerCapture(event.pointerId);
    dragPan = true;
    panStartClientX = event.clientX;
    panStartClientY = event.clientY;
    panOriginX = panX;
    panOriginY = panY;
  }

  function stopDragging() {
    dragQuestNodeId = null;
    dragPlaceId = null;
    dragPan = false;
    dragOffsetX = 0;
    dragOffsetY = 0;
  }

  function svgPointFromClient(svg: SVGSVGElement, clientX: number, clientY: number) {
    const screenMatrix = svg.getScreenCTM();
    if (screenMatrix) {
      const point = new DOMPoint(clientX, clientY).matrixTransform(screenMatrix.inverse());
      return { x: point.x, y: point.y };
    }
    const rect = svg.getBoundingClientRect();
    return {
      x: ((clientX - rect.left) / rect.width) * canvasWidth,
      y: ((clientY - rect.top) / rect.height) * canvasHeight,
    };
  }

  function canvasPointFromEvent(event: PointerEvent, svg: SVGSVGElement) {
    const rawPoint = svgPointFromClient(svg, event.clientX, event.clientY);
    const canvasX = canvasWidth / 2 + (rawPoint.x - panX - canvasWidth / 2) / zoom;
    const canvasY = canvasHeight / 2 + (rawPoint.y - panY - canvasHeight / 2) / zoom;
    return {
      x:
        canvasBounds.minX +
        ((canvasX - 80) / (canvasWidth - 160)) *
          canvasBounds.spanX,
      y:
        canvasBounds.minY +
        ((canvasY - 60) / (canvasHeight - 120)) *
          canvasBounds.spanY,
    };
  }

  function setMode(value: ViewMode) {
    mode = value;
    localStorage.setItem("quest-flow:view-mode", value);
  }

  function localKey() {
    return `quest-flow:draft:${selectedCatalog?.slug ?? "demo"}`;
  }

  function persistDraft() {
    if (selectedCatalog?.visibility === "authenticated") {
      saveState = "로그인 전용 데이터 · D1 저장 필요";
      return;
    }
    if (!selectedCatalog?.datasetVersion) {
      saveState = "데이터 버전 없음 · 저장 보류";
      return;
    }
    localStorage.setItem(
      localKey(),
      serializeCatalogDraft(selectedCatalog.datasetVersion, dataset),
    );
    saveState = `브라우저 저장 · ${new Date().toLocaleTimeString("ko-KR", {
      hour: "2-digit",
      minute: "2-digit",
    })}`;
  }

  function replaceDataset(next: QuestDataset) {
    const normalized = normalizeQuestDataset(next);
    if (!normalized) {
      statusMessage = "catalog 데이터 형식을 읽을 수 없습니다.";
      return;
    }
    const nextDataset = structuredClone(normalized);
    dataset = nextDataset.questNodes.some(
      (node) => node.coordinatesAutoGenerated,
    )
      ? { ...nextDataset, questNodes: autoLayoutQuestNodes(nextDataset.questNodes) }
      : nextDataset;
    canvasBounds = calculateCanvasBounds(canvasMode, dataset);
    panX = 0;
    panY = 0;
    selectedQuestId = dataset.questNodes[0]?.questId ?? null;
    selectedPlaceId = dataset.places[0]?.id ?? null;
    selectedStepId = dataset.steps[0]?.id ?? null;
    result = null;
  }

  function autoLayoutQuestGraph() {
    dataset = {
      ...dataset,
      questNodes: autoLayoutQuestNodes(dataset.questNodes),
    };
    canvasBounds = calculateCanvasBounds(canvasMode, dataset);
    zoom = 1;
    panX = 0;
    panY = 0;
    persistDraft();
    statusMessage = "선행 관계 기준으로 퀘스트 노드를 자동 배치했습니다.";
  }

  async function selectCatalog(catalog: CatalogSummary) {
    loading = true;
    statusMessage = "";
    try {
      const detail = await getCatalog(catalog.slug);
      selectedCatalog = detail.catalog;
      const workspace = workspaces.find(
        (value) => value.catalogId === detail.catalog.id,
      );
      currentWorkspace = workspace ?? null;
      const draftKey = `quest-flow:draft:${detail.catalog.slug}`;
      if (detail.catalog.visibility === "authenticated") {
        // Remove drafts created by older versions that persisted private data.
        localStorage.removeItem(draftKey);
      }
      const savedRaw =
        detail.catalog.visibility === "public" && !workspace
          ? localStorage.getItem(draftKey)
          : null;
      const draft = readCatalogDraft(
        savedRaw,
        detail.catalog.datasetVersion,
      );
      if (draft.discarded) {
        localStorage.removeItem(draftKey);
        statusMessage =
          "데이터가 갱신되어 구버전 브라우저 초안을 제거했습니다.";
      }
      replaceDataset(
        workspace?.state ??
          draft.dataset ??
          detail.dataset,
      );
      saveState = workspace
        ? "D1 작업공간 복원됨"
        : draft.dataset
          ? "브라우저 초안 복원됨"
          : draft.discarded
            ? "구버전 초안 제거 · catalog 불러옴"
            : "catalog 불러옴";
      runSimulation();
    } catch (error) {
      statusMessage =
        error instanceof Error ? error.message : "catalog를 불러오지 못했습니다.";
    } finally {
      loading = false;
    }
  }

  function updatePlace(
    id: string,
    patch: Partial<MapPlace>,
  ) {
    dataset = {
      ...dataset,
      places: dataset.places.map((place) =>
        place.id === id ? { ...place, ...patch } : place,
      ),
    };
    persistDraft();
  }

  function setPlaceShape(id: string, value: string) {
    const shape: PlaceShape =
      value === "circle" || value === "rectangle" ? value : "point";
    updatePlace(id, { shape });
  }

  function updateQuestNode(id: string, patch: Partial<Pick<QuestNode, "x" | "y">>) {
    dataset = {
      ...dataset,
      questNodes: dataset.questNodes.map((node) =>
        node.id === id ? { ...node, ...patch } : node,
      ),
    };
    persistDraft();
  }

  function addPlace() {
    const id = `place-${Date.now()}`;
    const place: MapPlace = {
      id,
      name: "새 장소",
      mapId: "user-edit",
      shape: "point",
      xMeters: canvasBounds.minX + canvasBounds.spanX / 2,
      yMeters: canvasBounds.minY + canvasBounds.spanY / 2,
      actorId: "",
    };
    dataset = { ...dataset, places: [...dataset.places, place] };
    selectedPlaceId = id;
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
    if (dragPan) {
      const svg = event.currentTarget as SVGSVGElement;
      const startPoint = svgPointFromClient(svg, panStartClientX, panStartClientY);
      const currentPoint = svgPointFromClient(svg, event.clientX, event.clientY);
      panX = panOriginX + currentPoint.x - startPoint.x;
      panY = panOriginY + currentPoint.y - startPoint.y;
      return;
    }
    const point = canvasPointFromEvent(
      event,
      event.currentTarget as SVGSVGElement,
    );
    if (dragQuestNodeId) {
      updateQuestNode(dragQuestNodeId, {
        x: Math.round(point.x - dragOffsetX),
        y: Math.round(point.y - dragOffsetY),
      });
    }
    if (dragPlaceId) {
      updatePlace(dragPlaceId, {
        xMeters: Math.round((point.x - dragOffsetX) * 10) / 10,
        yMeters: Math.round((point.y - dragOffsetY) * 10) / 10,
      });
    }
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

  function openLogin() {
    if (!session.authConfigured) {
      statusMessage = "관리자 비밀번호가 아직 설정되지 않았습니다.";
      return;
    }
    loginError = "";
    adminPassword = "";
    loginOpen = true;
  }

  async function submitLogin(event: SubmitEvent) {
    event.preventDefault();
    if (loginBusy || !adminPassword) return;

    loginBusy = true;
    loginError = "";
    try {
      session = await loginAdmin(adminPassword);
      adminPassword = "";
      catalogs = await getCatalogs();
      workspaces = await getWorkspaces();
      loginOpen = false;
      statusMessage = "관리자로 로그인했습니다.";

      const current =
        catalogs.find((catalog) => catalog.slug === selectedCatalog?.slug) ??
        catalogs[0];
      if (current) await selectCatalog(current);
    } catch (error) {
      loginError =
        error instanceof Error ? error.message : "로그인하지 못했습니다.";
    } finally {
      loginBusy = false;
    }
  }

  async function signOut() {
    if (loginBusy) return;

    loginBusy = true;
    try {
      for (const catalog of catalogs) {
        if (catalog.visibility === "authenticated") {
          localStorage.removeItem(`quest-flow:draft:${catalog.slug}`);
        }
      }
      session = await logoutAdmin();
      workspaces = [];
      currentWorkspace = null;
      catalogs = await getCatalogs();
      const publicCatalog =
        catalogs.find((catalog) => catalog.visibility === "public") ??
        catalogs[0];
      if (publicCatalog) await selectCatalog(publicCatalog);
      statusMessage = "로그아웃했습니다.";
    } catch (error) {
      statusMessage =
        error instanceof Error ? error.message : "로그아웃하지 못했습니다.";
    } finally {
      loginBusy = false;
    }
  }

  function formatTime(seconds: number) {
    if (!Number.isFinite(seconds)) return "—";
    const totalSeconds = Math.max(0, Math.round(seconds));
    const hours = Math.floor(totalSeconds / 3600);
    const minutes = Math.floor((totalSeconds % 3600) / 60);
    const rest = totalSeconds % 60;
    if (hours > 0) return `${hours}시간 ${minutes}분 ${rest}초`;
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
        {session.authenticated ? session.displayName ?? "관리자" : "체험 모드"}
      </span>
      {#if session.authenticated}
        <button class="auth-action" disabled={loginBusy} onclick={signOut}>
          로그아웃
        </button>
      {:else}
        <button class="auth-action" disabled={loginBusy} onclick={openLogin}>
          관리자 로그인
        </button>
      {/if}
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

  {#if loginOpen}
    <div
      class="login-backdrop"
      role="presentation"
      onclick={(event) => {
        if (event.target === event.currentTarget && !loginBusy) {
          loginOpen = false;
        }
      }}
    >
      <div
        class="login-dialog"
        role="dialog"
        aria-modal="true"
        aria-labelledby="login-title"
      >
        <div class="login-heading">
          <div>
            <span class="eyebrow">ADMIN SESSION</span>
            <h2 id="login-title">관리자 로그인</h2>
          </div>
          <button
            class="dialog-close"
            type="button"
            aria-label="로그인 창 닫기"
            disabled={loginBusy}
            onclick={() => (loginOpen = false)}
          >×</button>
        </div>

        <form onsubmit={submitLogin}>
          <label>
            <span>관리자 비밀번호</span>
            <input
              type="password"
              name="password"
              autocomplete="current-password"
              maxlength="256"
              required
              bind:value={adminPassword}
            />
          </label>
          {#if loginError}
            <p class="login-error" role="alert">{loginError}</p>
          {/if}
          <div class="login-actions">
            <button
              class="secondary"
              type="button"
              disabled={loginBusy}
              onclick={() => (loginOpen = false)}
            >취소</button>
            <button
              class="primary"
              type="submit"
              disabled={loginBusy || !adminPassword}
            >{loginBusy ? "확인 중…" : "로그인"}</button>
          </div>
        </form>
      </div>
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
        <span class="eyebrow">QUEST NODES</span>
        {#each dataset.questNodes as node, index}
          <button
            class:active={node.questId === selectedQuestId}
            onclick={() => {
              selectedQuestId = node.questId;
              selectedStepId = dataset.steps.find(
                (step) => step.questId === node.questId,
              )?.id ?? null;
              activeProTab = "properties";
            }}
          >
            <span>{String(index + 1).padStart(2, "0")}</span>
            <div>
              <strong>{node.questId} {node.title}</strong>
              <small>
                {dataset.steps.filter((step) => step.questId === node.questId).length}
                개 동선 단계
              </small>
            </div>
          </button>
        {/each}
      </div>
    </aside>

    <section class="canvas-panel">
      <div class="canvas-toolbar">
        <div>
          <span class="eyebrow">
            {canvasMode === "quest-chain"
              ? "QUEST CHAIN · GRAPH"
              : canvasMode === "quest-route"
                ? "QUEST ROUTE · READ ONLY"
                : "PLACE EDITOR · METERS"}
          </span>
          <h1>{dataset.title}</h1>
        </div>
        <div class="canvas-controls">
          <div class="canvas-mode-switch" aria-label="중앙 그래프 표시 모드">
            <button
              class:active={canvasMode === "quest-chain"}
              onclick={() => setCanvasMode("quest-chain")}
            >퀘스트 체인</button>
            <button
              class:active={canvasMode === "quest-route"}
              onclick={() => setCanvasMode("quest-route")}
            >선택 퀘스트 동선</button>
            <button
              class:active={canvasMode === "place-edit"}
              onclick={() => setCanvasMode("place-edit")}
            >장소 편집</button>
          </div>
          {#if canvasMode === "quest-chain"}
            <button class="secondary auto-layout" onclick={autoLayoutQuestGraph}>
              노드 자동 배치
            </button>
          {/if}
          <div class="canvas-zoom">
            <span>{Math.round(zoom * 100)}%</span>
            <button class="secondary" onclick={resetZoom}>줌 초기화</button>
          </div>
        </div>
      </div>

      <div class="map-frame">
        {#if loading}
          <div class="loading">catalog 불러오는 중…</div>
        {/if}
        <svg
          viewBox={`0 0 ${canvasWidth} ${canvasHeight}`}
          preserveAspectRatio="xMidYMid meet"
          aria-label="퀘스트 그래프와 맵 장소 편집기"
          role="application"
          onwheel={handleWheel}
          onpointermove={handlePointerMove}
          onpointerup={stopDragging}
          onpointercancel={stopDragging}
          onpointerleave={stopDragging}
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
            <marker id="quest-arrow" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="1.75" markerHeight="1.75" orient="auto-start-reverse">
              <path d="M 0 0 L 10 5 L 0 10 z" fill="#50d2a0" />
            </marker>
          </defs>
          <rect
            class="canvas-hit-area"
            width={canvasWidth}
            height={canvasHeight}
            fill="url(#grid)"
            onpointerdown={handleCanvasPointerDown}
            role="application"
            aria-label="그래프 영역 이동"
          />

          <g transform={`translate(${canvasWidth / 2 + panX} ${canvasHeight / 2 + panY}) scale(${zoom}) translate(${-canvasWidth / 2} ${-canvasHeight / 2})`}>
            {#if canvasMode === "quest-chain"}
              {#each graphEdges as edge}
                {@const from = edge.from}
                {@const to = edge.to}
                {#if from && to}
                  <path
                    class="quest-edge"
                    d={questEdgePath(from, to)}
                    marker-end="url(#quest-arrow)"
                  />
                {/if}
              {/each}

              {#each dataset.questNodes as node}
                <g
                  class:active={node.questId === selectedQuestId}
                  class="quest-node"
                  transform={`translate(${xOnCanvas(node.x)} ${yOnCanvas(node.y)})`}
                  onpointerdown={(event) => {
                    const svg = (event.currentTarget as SVGGElement).ownerSVGElement;
                    if (svg) {
                      const point = canvasPointFromEvent(event, svg);
                      dragOffsetX = point.x - node.x;
                      dragOffsetY = point.y - node.y;
                    }
                    dragPan = false;
                    (event.currentTarget as SVGGElement).setPointerCapture(
                      event.pointerId,
                    );
                    dragQuestNodeId = node.id;
                    selectedQuestId = node.questId;
                    selectedStepId = dataset.steps.find(
                      (step) => step.questId === node.questId,
                    )?.id ?? null;
                    activeProTab = "properties";
                  }}
                  role="button"
                  tabindex="0"
                >
                  <rect class="node-shell" x="-72" y="-29" width="144" height="58" rx="8" />
                  <rect class="node-header" x="-72" y="-29" width="144" height="16" rx="8" />
                  <rect class="node-body" x="-72" y="-13" width="144" height="29" />
                  <rect class="node-footer" x="-72" y="16" width="144" height="13" rx="5" />
                  <rect class="node-frame" x="-72" y="-29" width="144" height="58" rx="8" />
                  <circle
                    class="node-socket node-in-socket"
                    class:connected={hasIncomingQuestEdge(node)}
                    cx="-64"
                    cy="0"
                    r="4"
                  />
                  <circle
                    class="node-socket node-out-socket"
                    class:connected={hasOutgoingQuestEdge(node)}
                    cx="64"
                    cy="0"
                    r="4"
                  />
                  <text class="node-id" x="-61" y="-17">{node.questId}</text>
                  <text class="node-title" x="-28" y="-17">{node.title}</text>
                </g>
              {/each}
            {:else}
              {#each dataset.steps.filter((step) =>
                step.enabled &&
                (canvasMode === "place-edit" || step.questId === selectedQuestId) &&
                step.fromPlaceId !== step.toPlaceId
              ) as step}
                {@const from = placeById(step.fromPlaceId)}
                {@const to = placeById(step.toPlaceId)}
                {#if from && to}
                  <g
                    class:active={step.id === selectedStepId}
                    class="route"
                    onclick={() => {
                      selectedStepId = step.id;
                      selectedQuestId = step.questId;
                    }}
                    onkeydown={(event) => {
                      if (event.key === "Enter" || event.key === " ") {
                        event.preventDefault();
                        selectedStepId = step.id;
                        selectedQuestId = step.questId;
                      }
                    }}
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

              {#if canvasMode === "quest-route" && selectedRouteSteps.length === 0}
                <text class="empty-canvas" x="500" y="310">
                  선택한 퀘스트에는 등록된 맵 동선이 없습니다.
                </text>
              {/if}

              {#each dataset.places.filter((place) =>
                canvasMode === "place-edit" ||
                selectedRouteSteps.some(
                  (step) =>
                    step.fromPlaceId === place.id || step.toPlaceId === place.id,
                )
              ) as place}
                <g
                  class:active={place.id === selectedPlaceId}
                  class:readonly={canvasMode === "quest-route"}
                  class="place"
                  transform={`translate(${xOnCanvas(place.xMeters)} ${yOnCanvas(place.yMeters)})`}
                  onpointerdown={(event) => {
                    selectedPlaceId = place.id;
                    activeProTab = "properties";
                    if (canvasMode === "place-edit") {
                      const svg = (event.currentTarget as SVGGElement).ownerSVGElement;
                      if (svg) {
                        const point = canvasPointFromEvent(event, svg);
                        dragOffsetX = point.x - place.xMeters;
                        dragOffsetY = point.y - place.yMeters;
                      }
                      dragPan = false;
                      (event.currentTarget as SVGGElement).setPointerCapture(
                        event.pointerId,
                      );
                      dragPlaceId = place.id;
                    }
                  }}
                  role="button"
                  tabindex="0"
                >
                  {#if place.shape === "circle"}
                    <circle
                      class="place-area"
                      r={widthOnCanvas(place.radiusMeters ?? 20)}
                    />
                  {:else if place.shape === "rectangle"}
                    <rect
                      class="place-area"
                      x={-widthOnCanvas(place.widthMeters ?? 50) / 2}
                      y={-heightOnCanvas(place.heightMeters ?? 40) / 2}
                      width={widthOnCanvas(place.widthMeters ?? 50)}
                      height={heightOnCanvas(place.heightMeters ?? 40)}
                      rx="8"
                    />
                  {:else}
                    <circle class="place-point" r="18" />
                    <circle class="core" r="6" />
                  {/if}
                  <text y="44">{place.name}</text>
                  <text class="coordinate" y="62">
                    {place.xMeters.toFixed(1)}, {place.yMeters.toFixed(1)}
                  </text>
                </g>
              {/each}
            {/if}
          </g>
        </svg>
        <div class="map-hint">
          {canvasMode === "quest-chain"
            ? "빈 곳 드래그로 이동 · 노드 드래그 · 휠로 줌"
            : canvasMode === "quest-route"
              ? "빈 곳 드래그로 이동 · 선택 퀘스트 동선 읽기 전용 · 휠로 줌"
              : "빈 곳 드래그로 이동 · 장소 드래그·추가·형태 편집 · 휠로 줌"}
        </div>
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

        {#if canvasMode === "quest-chain" && selectedQuestNode}
          <fieldset>
            <legend>퀘스트 노드</legend>
            <label>
              <span>퀘스트</span>
              <input value={`${selectedQuestNode.questId} ${selectedQuestNode.title}`} readonly />
            </label>
            <div class="field-row">
              <label>
                <span>그래프 X</span>
                <input
                  type="number"
                  step="1"
                  value={selectedQuestNode.x}
                  oninput={(event) =>
                    updateQuestNode(selectedQuestNode!.id, {
                      x: event.currentTarget.valueAsNumber || 0,
                    })}
                />
              </label>
              <label>
                <span>그래프 Y</span>
                <input
                  type="number"
                  step="1"
                  value={selectedQuestNode.y}
                  oninput={(event) =>
                    updateQuestNode(selectedQuestNode!.id, {
                      y: event.currentTarget.valueAsNumber || 0,
                    })}
                />
              </label>
            </div>
            <small class="field-note">그래프 배치 좌표이며 맵 좌표가 아닙니다.</small>
          </fieldset>
        {:else if selectedPlace}
          <fieldset>
            <legend>장소</legend>
            <label>
              <span>이름</span>
              <input
                value={selectedPlace.name}
                disabled={canvasMode !== "place-edit"}
                oninput={(event) =>
                  updatePlace(selectedPlace!.id, { name: event.currentTarget.value })}
              />
            </label>
            <label>
              <span>형태</span>
              <select
                value={selectedPlace.shape}
                disabled={canvasMode !== "place-edit"}
                onchange={(event) =>
                  setPlaceShape(selectedPlace!.id, event.currentTarget.value)}
              >
                <option value="point">액터 지점</option>
                <option value="circle">원형 영역</option>
                <option value="rectangle">직사각형 영역</option>
              </select>
            </label>
            <div class="field-row">
              <label>
                <span>X (m)</span>
                <input
                  type="number"
                  step="0.1"
                  value={selectedPlace.xMeters}
                  disabled={canvasMode !== "place-edit"}
                  oninput={(event) =>
                    updatePlace(selectedPlace!.id, {
                      xMeters: event.currentTarget.valueAsNumber || 0,
                    })}
                />
              </label>
              <label>
                <span>Y (m)</span>
                <input
                  type="number"
                  step="0.1"
                  value={selectedPlace.yMeters}
                  disabled={canvasMode !== "place-edit"}
                  oninput={(event) =>
                    updatePlace(selectedPlace!.id, {
                      yMeters: event.currentTarget.valueAsNumber || 0,
                    })}
                />
              </label>
            </div>
            {#if selectedPlace.shape === "circle"}
              <label>
                <span>반지름 (m)</span>
                <input
                  type="number"
                  min="1"
                  step="1"
                  value={selectedPlace.radiusMeters ?? 20}
                  disabled={canvasMode !== "place-edit"}
                  oninput={(event) =>
                    updatePlace(selectedPlace!.id, {
                      radiusMeters: Math.max(1, event.currentTarget.valueAsNumber || 1),
                    })}
                />
              </label>
            {:else if selectedPlace.shape === "rectangle"}
              <div class="field-row">
                <label>
                  <span>너비 (m)</span>
                  <input
                    type="number"
                    min="1"
                    step="1"
                    value={selectedPlace.widthMeters ?? 50}
                    disabled={canvasMode !== "place-edit"}
                    oninput={(event) =>
                      updatePlace(selectedPlace!.id, {
                        widthMeters: Math.max(1, event.currentTarget.valueAsNumber || 1),
                      })}
                  />
                </label>
                <label>
                  <span>높이 (m)</span>
                  <input
                    type="number"
                    min="1"
                    step="1"
                    value={selectedPlace.heightMeters ?? 40}
                    disabled={canvasMode !== "place-edit"}
                    oninput={(event) =>
                      updatePlace(selectedPlace!.id, {
                        heightMeters: Math.max(1, event.currentTarget.valueAsNumber || 1),
                      })}
                  />
                </label>
              </div>
            {:else}
              <label>
                <span>액터 ID</span>
                <input
                  value={selectedPlace.actorId ?? ""}
                  disabled={canvasMode !== "place-edit"}
                  oninput={(event) =>
                    updatePlace(selectedPlace!.id, {
                      actorId: event.currentTarget.value,
                    })}
                />
              </label>
            {/if}
            {#if canvasMode === "quest-route"}
              <small class="field-note">동선 확인 모드에서는 장소를 변경할 수 없습니다.</small>
            {/if}
          </fieldset>
        {/if}

        {#if canvasMode === "place-edit"}
          <button class="secondary add-place" onclick={addPlace}>+ 장소 추가</button>
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
        <span class="eyebrow">QUEST NODES</span>
        {#each dataset.questNodes as node, index}
          <button
            class:active={node.questId === selectedQuestId}
            onclick={() => {
              selectedQuestId = node.questId;
              selectedStepId = dataset.steps.find(
                (step) => step.questId === node.questId,
              )?.id ?? null;
              activeProTab = "properties";
            }}
          >
            <span>{index + 1}</span>
            <strong>{node.questId} {node.title}</strong>
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
        <button
          class="secondary"
          disabled={selectedCatalog?.visibility === "authenticated"}
          onclick={persistDraft}
        >임시저장</button>
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
