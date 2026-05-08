# Implementation Plan: Workspaces Configuration

**Branch**: `001-add-workspaces-config` | **Date**: 2026-05-08 | **Spec**: [spec.md](./spec.md)
**Input**: Feature specification from `/specs/001-add-workspaces-config/spec.md`

## Summary

config.json에 `"workspaces"` 배열을 추가하여 $HOME 외 추가 경로에서 ai-cage 실행을 허용한다. 경로에 `$USER`가 포함되면 현재 사용자명으로 치환하여 사용자별 격리를 보장하고, 없으면 공용 경로로 모든 사용자에게 허용한다. main.c의 workspace 검증 로직을 확장하며, cwd는 항상 rw로 마운트된다.

## Technical Context

**Language/Version**: C99 (GCC)
**Primary Dependencies**: cJSON (vendored), util-linux (findmnt)
**Storage**: JSON config file (`/etc/ai/config.json`)
**Testing**: Shell-based integration tests, make build verification
**Target Platform**: Linux (kernel ≥ 5.13 for Landlock)
**Project Type**: CLI tool (sandbox wrapper)
**Performance Goals**: N/A (startup-time tool)
**Constraints**: main.c 단일 파일 변경으로 완결, 외부 의존성 추가 없음. 양 백엔드(Landlock/Apptainer) 모두 cwd rw override 기존 메커니즘으로 동작 확인됨.
**Scale/Scope**: ~40줄 코드 추가

## Constitution Check

*GATE: Constitution은 템플릿 상태(프로젝트별 미설정). Gate 위반 없음.*

## Project Structure

### Documentation (this feature)

```text
specs/001-add-workspaces-config/
├── plan.md              # This file
├── research.md          # Phase 0 output
├── data-model.md        # Phase 1 output
└── tasks.md             # Phase 2 output (/speckit.tasks)
```

### Source Code (repository root)

```text
src/
├── main.c               # config 파싱 + workspace 검증 (수정 대상)
├── backend.h            # MountList 구조체 (변경 없음)
├── backend_landlock.c   # Landlock 백엔드 (변경 없음)
└── backend_apptainer.c  # Apptainer 백엔드 (변경 없음)

config.json              # workspaces 키 추가 (예시 업데이트)
docs/configuration.md    # 문서 업데이트
```

**Structure Decision**: 기존 단일 프로젝트 구조 유지. main.c만 수정.

## Test Plan

### Unit Tests (C)

`tests/test_workspaces.c` — 순수 로직 검증 (빌드 시 실행):

| Test | 검증 내용 |
|------|-----------|
| `test_expand_user_variable` | `$USER`가 포함된 경로가 올바르게 치환되는지 |
| `test_expand_no_variable` | `$USER` 없는 경로는 그대로 반환되는지 |
| `test_workspace_match_home` | cwd가 $HOME 하위일 때 항상 허용 |
| `test_workspace_match_configured` | cwd가 치환된 workspace 하위일 때 허용 |
| `test_workspace_reject_other_user` | cwd가 다른 사용자의 workspace일 때 거부 |
| `test_workspace_reject_unmatched` | 어떤 workspace에도 매칭 안 될 때 거부 |
| `test_workspace_empty_array` | workspaces가 빈 배열일 때 HOME만 허용 |
| `test_workspace_missing_key` | workspaces 키 자체가 없을 때 기존 동작 유지 |

### Integration Tests (Shell)

`tests/integration/test_workspaces.sh` — 실제 바이너리 실행 검증:

| Test | 방법 |
|------|------|
| 허용된 workspace에서 실행 | 임시 디렉토리를 workspace로 설정, 해당 경로에서 ai-cage 실행 → exit 0 |
| 거부된 경로에서 실행 | workspace에 없는 경로에서 실행 → exit 1 + 오류 메시지 확인 |
| $USER 치환 검증 | `/tmp/ws/$USER` 설정 후 `/tmp/ws/$(whoami)/proj`에서 실행 → 성공 |
| 공용 경로 검증 | `/tmp/ws/common` 설정 후 해당 경로에서 실행 → 성공 |
| 다른 사용자 경로 거부 | `/tmp/ws/$USER` 설정 후 `/tmp/ws/fakeuser/proj`에서 실행 → 실패 |
| 디버그 출력 확인 | `WRAPPED_AI_DEBUG=1`로 실행 → stderr에 치환된 workspace 경로 출력 |
| 하위 호환성 | workspaces 키 없는 config로 $HOME 하위에서 실행 → 기존과 동일 |

### Backend-specific Tests

| 백엔드 | 검증 내용 |
|--------|-----------|
| **Landlock** | workspace 경로가 mounts에서 ro인 상위 하위일 때, cwd에 파일 쓰기 가능한지 확인 |
| **Apptainer** | 동일 시나리오에서 컨테이너 내 cwd에 파일 쓰기 가능한지 확인 (CWD 자동 마운트 rw 동작) |

### 테스트 실행 방법

```sh
# 빌드 + unit tests
make check

# Integration tests (root 불필요, Landlock 백엔드)
tests/integration/test_workspaces.sh

# Apptainer 백엔드 (apptainer 설치 필요)
BACKEND=apptainer tests/integration/test_workspaces.sh
```

