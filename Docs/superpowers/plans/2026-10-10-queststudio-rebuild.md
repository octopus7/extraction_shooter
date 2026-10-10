# QuestStudio Rebuild Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** 챕터별 퀘스트 연결 편집과 수동 전체 스냅샷 교환을 지원하는 QuestStudio 및 한국어 저작 데이터를 구성한다.

**Architecture:** 기존 퀘스트 정의 객체를 보존하는 공유 교환 포맷을 사용한다. Svelte 화면과 Worker API가 동일한 검증 규칙을 사용하고, D1은 불변 전체 스냅샷을 보관한다. 본편 원문과 노드 데이터는 접근 제한 로컬 저장소에만 둔다.

**Tech Stack:** TypeScript, Svelte, Vite, Cloudflare Workers/D1, Wrangler, Vitest; UE 5.7 C++/JSON/CSV.

**Spec:** [승인된 설계](../specs/2026-10-10-queststudio-rebuild-design.md)

## Global Constraints

- 로컬 SSOT, 사용자 제어 임포트·익스포트. 자동 시드·동기화·게시 금지.
- 전체 스냅샷과 동일 snapshot_id의 노드/문자열만 사용.
- UI 문구는 한국어 문자열 키 테이블로 해결. 새 콘텐츠는 한국어만 작성.
- 기존 ADMIN_PASSWORD 관리자 인증. 구글 로그인 및 이메일 관리자 판별 추가 금지.
- 본편 데이터는 접근 제한 저장소에 보관. 공용 fixture는 합성 데이터만 사용.
- 원문에 없는 실행 조건이나 선행관계를 확정하지 않는다.
- 기존 dirty RaidMap.umap와 다른 작업의 변경을 보존한다.
- 검증·일회성 코드 정리·기록을 저장소별 단일 작업 커밋으로 묶는다. 원격 게시/데이터 변경은 별도 사용자 동작이다.

## Review Focus

1. 챕터를 바꾼 뒤 저장해도 보이지 않는 노드/관계/문자열이 남아야 한다.
2. 미기재 선행은 확정된 루트로 오인하지 않으며, 번호 오류/자기 참조/순환을 거부한다.
3. 알 수 없는 JSON 필드와 기존 다른 언어 문자열은 가져오기/내보내기에서 보존한다.
4. 스냅샷 저장 실패 시 부분 데이터가 노출되지 않으며 다른 버전과 섞이지 않는다.
5. 데모 정의 제거 후 자동 수락·종료·세이브 복원 경로에 삭제한 퀘스트가 살아나지 않아야 한다.

## 공통 인터페이스

`src/shared/types.ts`:

```ts
interface QuestDefinition {
  quest_id: string;
  title_string_key: string;
  description_string_key: string;
  required_completed_quest_ids: string[];
  authoring_tags: string[]; // exactly one chapter:N tag
  [key: string]: unknown;
}
interface QuestNode {
  definition: QuestDefinition;
  x: number;
  y: number;
  authoring: {
    prerequisitesStatus: 'confirmed' | 'unspecified';
    sourceReference?: string;
    [key: string]: unknown;
  };
}
interface LocalizationEntry { key: string; locale: string; value: string }
interface QuestPack { schemaVersion: 1; nodes: QuestNode[]; strings: LocalizationEntry[] }
interface SnapshotMetadata {
  id: string; alias: string; memo: string; createdAt: string;
  nodeCount: number; stringCount: number;
}
```

`validation.ts`: `validatePack(value: unknown): QuestPack`, `ValidationError` with `key: string` and optional `params: Record<string, string | number>`. Preserve unknown fields. Validate finite positions, unique case-insensitive quest IDs, localization tuple uniqueness, Korean string references, known prerequisites and acyclic graph.

`graph.ts`: `getChapter(node: QuestNode): string`, `resolveQuestId(pack: QuestPack, input: string, chapter: string): string`, `updatePrerequisites(pack: QuestPack, targetQuestId: string, prerequisiteIds: string[]): QuestPack`, `projectChapter(pack: QuestPack, chapter: string): GraphProjection`. Projection returns visible nodes `{id, questId, x, y, external, titleKey}` and edges `{source, target}`. Only direct external prerequisites appear. Number input accepts current-region numbers or full region-qualified IDs; existing arbitrary IDs remain usable by exact match.

`ui-strings.ts`: `t(key: string, params?: Record<string, string | number>): string`. UI and API error messages use keys. The UI owner owns this file; other owners send required keys to that owner.

Worker routes:
- `GET /api/session` → `{authenticated, authConfigured}`
- `POST /api/login` with `{password}`; `POST /api/logout`
- `GET /api/snapshots` → `{snapshots: SnapshotMetadata[]}`
- `POST /api/snapshots` with `{alias, memo, pack}` → `{snapshot: SnapshotMetadata}`
- `GET /api/snapshots/:id` → `{snapshot: SnapshotMetadata, pack: QuestPack}`
- Errors → `{error: {key, params?}}`. All snapshot routes require authentication.

## Task 1: Shared contract and Worker persistence — agent A

**Files:** `Tools/QuestStudio/src/shared/{types,validation,graph}.ts`, `src/worker/{index,auth,http,snapshots}.ts`, `migrations/0005_queststudio_snapshots.sql`, relevant `test/*.spec.ts`.

- [x] Write and run failing tests for pack validation, known/unknown IDs, cycles, external boundary projection and preserving hidden data.
- [x] Implement the declared shared interfaces. Chapter tags never participate in prerequisite availability.
- [x] Recover only administrator auth behavior from `aa458986^`; remove sync-token/publish/workspace behavior.
- [x] Create three work tables plus compatible auth tables. Store complete node JSON, preserving unknown fields. Snapshot writes use a single transactional D1 batch.
- [x] Test full snapshot round-trip, failed save rollback, authorization, session expiry, login throttling and mutation origin checks with Workers/D1 integration tests.

## Task 2: Chapter graph and manual workflow — agent B

**Files:** `Tools/QuestStudio/src/client/*`, `src/shared/ui-strings.ts`, `index.html`, client behavior tests.

- [x] Implement keyed Korean UI strings and client API error handling.
- [x] Build login, manual file import/export, snapshot save/load, chapter selector and graph zoom/pan.
- [x] Show selected chapter nodes and direct external prerequisite boundary nodes labeled `다른 지역`; no recursion past the boundary.
- [x] Show read-only quest details and prerequisite input/removal. Invalid input leaves the document unchanged. No place or simulation UI.
- [x] Test/verify filtering, remove/add by number, bad input, manual import/export and complete snapshot preservation.

## Task 3: Source-based Korean authoring pack — agent C

**Files:** access-restricted `TunaSweeper/External/MainPayload/Authoring/QuestStudio/` and corresponding private validation/report/log files only.

- [x] Read source files and current private SSOT priorities; retain original regional numbering.
- [x] Produce QuestPack matching the common interface, Korean title/description/detail strings and `chapter:N` tags.
- [x] Enter only explicit prerequisites; preserve unspecified status and source evidence. Avoid invented runtime events and IDs.
- [x] Validate source coverage, numbering, references, Korean-only new content and absence of demo IDs.
- [x] Remove any one-off generators after independent output validation. Do not stage pre-existing private changes.

## Task 4: Scaffold, demo cleanup and integration — root

**Files:** `Tools/QuestStudio/{package.json,package-lock.json,tsconfig.json,vite.config.ts,vitest.config.ts,wrangler.jsonc,.gitignore,README.md}`, public quest/scenario JSON/CSV, affected UE subsystem/tests/docs.

- [x] Restore compatible tooling configuration selectively and install dependencies; no content seed hooks.
- [x] Remove UE demo quest definitions and now-unreferenced quest-specific strings; preserve shared UI strings.
- [x] Remove obsolete demo auto-accept/ending quest coupling and update meaningful regression tests. Keep unrelated map/interaction work untouched.
- [x] Integrate agents, run tool type checking, tests, production build and Wrangler dry run. Use synthetic public fixtures.
- [x] Run required UE build/tests for C++ changes and open the explicit TunaSweeper project after successful build.
- [x] Perform browser checks on local tool, run final diff/private-content checks, and request an independent review.
- [x] Log only public work publicly and private data work privately. Commit completed changes once per affected repository, including the pending authentication correction. Do not deploy or import content remotely automatically.

## Execution

User selected parallel agents. After this plan is reviewed, agents A/B/C implement their exclusive file scopes concurrently; root integrates and handles UE cleanup. No separate worktree is necessary for the file-disjoint work in this checkout; the existing unrelated map change is explicitly excluded from commits.
