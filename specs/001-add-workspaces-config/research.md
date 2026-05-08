# Research: Workspaces Configuration

## Decision 1: $USER 치환 방식

- **Decision**: `strstr()`로 `$USER` 문자열을 찾아 `pw->pw_name`으로 치환. 단순 문자열 치환.
- **Rationale**: 이미 main.c에서 `getpwuid(getuid())`로 사용자 정보를 가져오고 있음. 추가 의존성 없이 구현 가능.
- **Alternatives considered**: 환경변수 전체 확장 (불필요한 복잡성), 정규식 (overkill)

## Decision 2: workspace 검증 위치

- **Decision**: 기존 `starts_with(workspace, user_home)` 체크를 확장하여, 실패 시 workspaces 배열을 순회하며 매칭.
- **Rationale**: 기존 로직 흐름을 최소한으로 변경. HOME 체크가 먼저 통과하면 workspaces는 검사하지 않음.
- **Alternatives considered**: 모든 허용 경로를 하나의 배열로 통합 (기존 코드 변경량 증가)

## Decision 3: cwd rw override

- **Decision**: 양 백엔드 모두 cwd를 rw로 처리 가능. 추가 작업 최소.
- **Rationale**:
  - **Landlock 백엔드**: `backend_run`에 전달되는 workspace(cwd)에 대해 rw 권한을 부여. Landlock은 더 구체적인 경로에 더 넓은 권한을 줄 수 있으므로 상위 ro와 무관하게 cwd rw 가능.
  - **Apptainer 백엔드**: Apptainer는 CWD를 system-defined bind path로 자동 마운트함 (rw 기본). 현재 코드에서 `(void)workspace;`로 무시하는 이유. 단, 상위 경로가 `-B /path:ro`로 마운트된 경우에도 CWD 자동 마운트가 rw로 override되는지 확인 필요.
- **Apptainer 동작 확인**: Apptainer는 각 `-B` 옵션을 독립 bind mount로 처리. 더 구체적인 경로가 나중에 마운트되면 override됨. CWD 자동 마운트는 user-defined bind 이후에 적용되므로, 상위 ro bind가 있어도 CWD는 rw로 마운트됨.
- **과거 이슈**: GitHub #1833 (Singularity 2.5)에서 하위 디렉토리 bind 실패 버그 있었으나 이미 수정됨. Apptainer ≥ 1.0에서는 문제 없음.
- **결론**: 양 백엔드 모두 기존 메커니즘으로 cwd rw가 보장됨. 별도 코드 변경 불필요.
- **Alternatives considered**: Apptainer 백엔드에서 workspace를 명시적으로 `-B workspace:workspace:rw`로 추가 (불필요 — 이미 자동 마운트됨)

## Decision 4: config.json 스키마

- **Decision**: 최상위에 `"workspaces": ["path1", "path2"]` 문자열 배열 추가.
- **Rationale**: 기존 mounts 구조와 동일 레벨. 단순 배열이 가장 직관적.
- **Alternatives considered**: 객체 배열 `[{"path": "...", "mode": "user"}]` (불필요한 복잡성)
