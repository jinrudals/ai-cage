# Tasks: Workspaces Configuration

**Input**: Design documents from `/specs/001-add-workspaces-config/`
**Prerequisites**: plan.md, spec.md, research.md, data-model.md

**Organization**: Tasks are grouped by user story to enable independent implementation and testing.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (e.g., US1, US2)
- Include exact file paths in descriptions

---

## Phase 1: Setup

**Purpose**: 테스트 인프라 구성

- [ ] T001 Create test directory structure: `tests/` and `tests/integration/`
- [ ] T002 [P] Create test config fixtures in `tests/fixtures/config_with_workspaces.json` and `tests/fixtures/config_without_workspaces.json`

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: workspace 검증에 필요한 핵심 함수 추출 및 구현

**⚠️ CRITICAL**: User story 구현 전에 완료 필수

- [ ] T003 Extract `expand_user_variable()` function in `src/main.c` — 경로 문자열에서 `$USER`를 `pw->pw_name`으로 치환하는 함수
- [ ] T004 Extract `is_workspace_allowed()` function in `src/main.c` — cwd가 HOME 또는 치환된 workspaces 중 하나의 하위인지 검증하는 함수
- [ ] T005 [P] Add workspaces JSON 파싱 로직 in `src/main.c` — `cJSON_GetObjectItem(cfg, "workspaces")` 배열 순회 및 치환

**Checkpoint**: 핵심 함수 준비 완료

---

## Phase 3: User Story 1 - 허용된 워크스페이스에서 실행 (Priority: P1) 🎯 MVP

**Goal**: config.json의 workspaces에 등록된 경로/$USER 하위에서 ai-cage 실행 허용

**Independent Test**: `tests/fixtures/config_with_workspaces.json`에 workspace 설정 후 해당 경로에서 ai-cage 실행 → 성공

### Implementation for User Story 1

- [ ] T006 [US1] Integrate `is_workspace_allowed()` into main() replacing existing `starts_with(workspace, user_home)` check in `src/main.c`
- [ ] T007 [US1] Update error message in `src/main.c` — "workspace must be under your home directory or a configured workspace path"
- [ ] T008 [US1] Write integration test: 허용된 workspace에서 실행 성공 in `tests/integration/test_workspaces.sh`

**Checkpoint**: $USER 치환된 workspace 경로에서 ai-cage 실행 가능

---

## Phase 4: User Story 2 - 홈 디렉토리 기본 허용 유지 (Priority: P1)

**Goal**: workspaces 설정 유무와 관계없이 $HOME 하위에서 항상 실행 가능

**Independent Test**: workspaces 키 없는 config로 $HOME 하위에서 실행 → 기존과 동일하게 성공

### Implementation for User Story 2

- [ ] T009 [US2] Verify `is_workspace_allowed()` checks HOME first before workspaces in `src/main.c`
- [ ] T010 [US2] Write integration test: workspaces 미설정 시 HOME 하위 실행 성공 in `tests/integration/test_workspaces.sh`
- [ ] T011 [US2] Write integration test: workspaces 설정 시에도 HOME 하위 실행 성공 in `tests/integration/test_workspaces.sh`

**Checkpoint**: 하위 호환성 100% 보장

---

## Phase 5: User Story 3 - 허용되지 않은 경로에서 실행 차단 (Priority: P2)

**Goal**: workspace에 매칭되지 않고 HOME 하위도 아닌 경로에서 실행 거부 + 명확한 오류 메시지

**Independent Test**: 허용 목록에 없는 경로에서 실행 → exit 1 + 오류 메시지 출력

### Implementation for User Story 3

- [ ] T012 [US3] Write integration test: 미허용 경로에서 실행 거부 + 오류 메시지 확인 in `tests/integration/test_workspaces.sh`
- [ ] T013 [US3] Write integration test: 다른 사용자 workspace 경로에서 실행 거부 in `tests/integration/test_workspaces.sh`
- [ ] T014 [US3] Write integration test: 공용 경로($USER 없음) 실행 성공 in `tests/integration/test_workspaces.sh`

**Checkpoint**: 보안 경계 검증 완료

---

## Phase 6: User Story 4 - 디버그 모드 출력 (Priority: P3)

**Goal**: WRAPPED_AI_DEBUG=1 시 치환된 workspaces 목록을 stderr에 출력

**Independent Test**: 디버그 모드로 실행 → stderr에 "Workspaces (N):" + 경로 목록 출력

### Implementation for User Story 4

- [ ] T015 [US4] Add debug output for resolved workspaces list in `src/main.c` — 기존 debug 블록에 workspaces 출력 추가
- [ ] T016 [US4] Write integration test: 디버그 출력에 치환된 workspace 경로 포함 확인 in `tests/integration/test_workspaces.sh`

**Checkpoint**: 디버그 진단 기능 완료

---

## Phase 7: Polish & Cross-Cutting Concerns

**Purpose**: 문서화 및 예시 업데이트

- [ ] T017 [P] Update `docs/configuration.md` — workspaces 섹션 추가 (형식, $USER 치환, 예시)
- [ ] T018 [P] Update `config.json` — workspaces 예시 추가
- [ ] T019 Build verification: `make clean && make` 성공 확인

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies
- **Foundational (Phase 2)**: Depends on Phase 1
- **User Stories (Phase 3-6)**: All depend on Phase 2 completion
- **Polish (Phase 7)**: Depends on all user stories

### User Story Dependencies

- **US1 (P1)**: Phase 2 완료 후 시작 가능 — 다른 story 의존 없음
- **US2 (P1)**: Phase 2 완료 후 시작 가능 — US1과 병렬 가능 (같은 함수 사용하지만 다른 테스트)
- **US3 (P2)**: US1 완료 후 시작 권장 (동일 검증 로직 활용)
- **US4 (P3)**: Phase 2 완료 후 시작 가능 — 독립적

### Parallel Opportunities

- T001, T002: 병렬 가능
- T003, T004, T005: T005는 T003에 의존하지만 T004는 병렬 가능
- T017, T018: 병렬 가능
- US1과 US4는 서로 독립적으로 병렬 가능

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Phase 1: Setup (T001-T002)
2. Phase 2: Foundational (T003-T005)
3. Phase 3: User Story 1 (T006-T008)
4. **STOP and VALIDATE**: workspace 경로에서 실행 성공 확인
5. `make` 빌드 성공 확인

### Incremental Delivery

1. Setup + Foundational → 핵심 함수 준비
2. US1 → workspace 허용 동작 ✓
3. US2 → 하위 호환성 검증 ✓
4. US3 → 보안 경계 검증 ✓
5. US4 → 디버그 출력 ✓
6. Polish → 문서화 ✓

---

## Notes

- 모든 변경은 `src/main.c` 단일 파일에 집중
- 기존 `starts_with()` 함수 재사용
- 기존 `pw` 변수 (getpwuid 결과) 재사용
- Integration test는 실제 바이너리 빌드 후 실행
