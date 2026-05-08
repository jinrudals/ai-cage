# Feature Specification: Workspaces Configuration

**Feature Branch**: `001-add-workspaces-config`  
**Created**: 2026-05-08  
**Status**: Complete  
**Input**: User description: "config.json에 workspaces 설정 기능 추가 — ai-cage를 실행할 수 있는 경로를 설정. HOME 밑은 default 허용. 경로에 $USER를 리터럴로 포함하여 사용자별 격리."

## User Scenarios & Testing

### User Story 1 - 허용된 워크스페이스에서 ai-cage 실행 (Priority: P1)

시스템 관리자가 config.json에 workspaces 목록을 설정하면, 사용자는 해당 경로에서 ai-cage를 실행할 수 있다. 경로에 `$USER`를 포함하면 실행 시 실제 사용자명으로 치환된다. `$USER`가 없는 경로는 모든 사용자에게 그대로 허용된다. 예: "/workspace/project/$USER"는 사용자별 격리, "/workspace/project/common"은 모든 사용자 공용.

**Why this priority**: 핵심 기능. 공유 서버에서 사용자별 격리를 자동 보장하면서 홈 디렉토리 밖 작업을 허용한다.

**Independent Test**: config.json에 workspaces를 추가한 뒤, $USER가 치환된 경로 하위에서 ai-cage를 실행하여 정상 동작을 확인한다.

**Acceptance Scenarios**:

1. **Given** config.json에 workspaces로 "/workspace/project/$USER"가 설정되고 현재 사용자가 benjamin, **When** /workspace/project/benjamin/my-app에서 ai-cage를 실행, **Then** 정상적으로 샌드박스가 구성되어 실행된다
2. **Given** 동일 설정, **When** /workspace/project/benjamin/team/repo 같은 깊은 경로에서 실행, **Then** 정상적으로 실행된다
3. **Given** 동일 설정, **When** /workspace/project/other_user/repo에서 실행, **Then** 실행이 거부된다
4. **Given** config.json에 workspaces로 "/workspace/project/common"이 설정됨 ($USER 없음), **When** 아무 사용자가 /workspace/project/common/repo에서 실행, **Then** 정상적으로 실행된다

---

### User Story 2 - 홈 디렉토리 기본 허용 유지 (Priority: P1)

workspaces 설정 유무와 관계없이, 사용자의 홈 디렉토리 하위 경로에서는 항상 ai-cage를 실행할 수 있다.

**Why this priority**: 기존 동작의 하위 호환성 보장.

**Independent Test**: workspaces 설정이 없는 config.json으로 홈 디렉토리 하위에서 ai-cage를 실행하여 기존과 동일하게 동작함을 확인한다.

**Acceptance Scenarios**:

1. **Given** config.json에 workspaces 항목이 없음, **When** 사용자가 $HOME/project에서 ai-cage를 실행, **Then** 기존과 동일하게 정상 실행된다
2. **Given** config.json에 workspaces가 설정됨, **When** 사용자가 $HOME/project에서 ai-cage를 실행, **Then** 여전히 정상 실행된다

---

### User Story 3 - 허용되지 않은 경로에서 실행 차단 (Priority: P2)

workspaces/$USER에 해당하지 않고 홈 디렉토리 하위도 아닌 경로에서 ai-cage를 실행하면 명확한 오류 메시지와 함께 거부된다.

**Why this priority**: 보안 경계를 명확히 하여 다른 사용자의 workspace 침범을 방지한다.

**Independent Test**: 허용 목록에 없는 경로에서 ai-cage를 실행하여 오류 메시지를 확인한다.

**Acceptance Scenarios**:

1. **Given** config.json에 workspaces로 "/workspace/project/$USER"만 설정됨, **When** 사용자가 /workspace/project (사용자 하위 아님)에서 실행, **Then** 오류 메시지가 출력되고 실행이 거부된다
2. **Given** 동일 설정, **When** 사용자가 /opt/other에서 실행, **Then** 오류 메시지가 출력되고 실행이 거부된다

---

### User Story 4 - 디버그 모드에서 워크스페이스 정보 출력 (Priority: P3)

디버그 모드(WRAPPED_AI_DEBUG=1)에서 ai-cage를 실행하면 현재 적용된 workspaces 목록과 resolve된 경로($USER 포함)가 stderr에 출력된다.

**Why this priority**: 관리자가 설정 문제를 진단할 때 유용하지만 핵심 기능은 아니다.

**Independent Test**: 디버그 모드로 실행하여 stderr 출력에 resolve된 workspace 경로가 포함되는지 확인한다.

**Acceptance Scenarios**:

1. **Given** WRAPPED_AI_DEBUG=1이고 workspaces가 ["/workspace/project/$USER"]로 설정됨, **When** 사용자 benjamin이 ai-cage를 실행, **Then** stderr에 "Workspace: /workspace/project/benjamin" 형태로 출력된다

---

### Edge Cases

- workspaces 배열이 비어있을 때? → 홈 디렉토리만 허용 (기존 동작과 동일)
- workspace 경로/$USER 디렉토리가 존재하지 않을 때? → 실행 거부 (디렉토리가 없으면 작업 불가)
- workspaces 경로에 심볼릭 링크가 포함된 경우? → 실제 경로(realpath) 기준으로 비교
- workspaces 경로가 중복 등록된 경우? → 중복 무시, 정상 동작
- cwd가 mounts의 ro 경로 하위일 때? → cwd는 rw로 override, 상위 ro 정책은 cwd 외 영역에서 유지

## Requirements

### Functional Requirements

- **FR-001**: config.json에 "workspaces" 키로 허용 경로 목록을 설정할 수 있어야 한다
- **FR-002**: workspaces의 각 항목은 절대 경로 문자열이며, `$USER` 변수를 포함할 수 있어야 한다
- **FR-003**: 경로에 `$USER`가 포함되어 있으면 실행 시 현재 사용자명으로 치환하고, 포함되어 있지 않으면 경로를 그대로 사용해야 한다
- **FR-004**: 현재 작업 디렉토리가 치환된 workspace 경로의 하위이면 실행을 허용해야 한다
- **FR-005**: 홈 디렉토리 하위 경로는 workspaces 설정과 무관하게 항상 허용되어야 한다
- **FR-006**: workspaces 항목이 없거나 빈 배열이면 기존 동작(홈 디렉토리만 허용)을 유지해야 한다
- **FR-007**: 허용되지 않은 경로에서 실행 시 사용자에게 이해 가능한 오류 메시지를 출력해야 한다
- **FR-008**: 디버그 모드에서 치환된 workspaces 목록을 출력해야 한다
- **FR-009**: workspace 경로에서 실행 시, cwd만 rw로 마운트하고 $HOME은 기존처럼 전체 rw를 유지해야 한다
- **FR-010**: cwd가 기존 mounts 정책에서 ro로 설정된 경로의 하위이더라도, cwd는 rw로 override되어야 한다

### Key Entities

- **Workspace**: config.json에 등록된 경로 문자열. `$USER`를 포함하면 사용자별 격리, 포함하지 않으면 모든 사용자 공용. 치환 후 해당 경로 하위에서만 실행 허용
- **Config (config.json)**: 시스템 전역 설정 파일. mounts와 workspaces를 포함

## Success Criteria

### Measurable Outcomes

- **SC-001**: 관리자가 config.json에 workspaces를 추가한 후 $USER가 치환된 경로 하위에서 ai-cage를 즉시 사용할 수 있다
- **SC-002**: 기존 config.json(workspaces 미설정)으로 운영 중인 환경에서 업데이트 후에도 기존 동작이 100% 유지된다
- **SC-003**: 다른 사용자의 workspace 경로에서 실행 시도 시 즉시 거부된다
- **SC-004**: 설정 변경 후 재컴파일/재시작 없이 즉시 반영된다

## Clarifications

### Session 2026-05-08

- Q: workspace 경로에서 실행 시 샌드박스 내 접근 권한은? → A: cwd만 rw로 마운트. $HOME은 기존처럼 전체 rw. workspace 루트의 기존 마운트 권한(예: ro)은 유지되며, cwd만 rw로 override된다.
- Q: workspace 경로 형식은? → A: config.json에 `$USER`를 리터럴로 포함하여 작성 (예: "/workspace/project/$USER"). 실행 시 실제 사용자명으로 치환. `$USER`가 없는 경로(예: "/workspace/project/common")는 모든 사용자에게 그대로 허용.

## Assumptions

- config.json은 시스템 관리자가 관리하며, 일반 사용자는 수정 권한이 없다
- workspaces 경로는 절대 경로만 지원하며, `$USER` 변수 치환을 지원한다 (그 외 환경변수는 미지원)
- 사용자명은 getpwuid(getuid())로 얻은 pw_name을 사용한다
- 경로 비교는 문자열 prefix 매칭으로 수행한다 (기존 starts_with 로직과 동일)
- 홈 디렉토리 허용은 하드코딩된 기본 동작으로 유지하며, workspaces로 비활성화할 수 없다
- workspace/$USER 디렉토리는 관리자가 사전에 생성해둔다고 가정한다
