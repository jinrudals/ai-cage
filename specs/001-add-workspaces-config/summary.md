# Summary: 001-add-workspaces-config

**Branch**: `001-add-workspaces-config`
**Date**: 2026-05-08
**변경 파일**: 2개 (+56줄, -6줄)

## 개요

config.json에 `"workspaces"` 배열을 추가하여 `$HOME` 외 경로에서 ai-cage 실행을 허용한다. 경로에 `$USER`를 포함하면 실행 시 현재 사용자명으로 치환되어 사용자별 격리를 보장한다.

```json
{
  "workspaces": ["/workspace/project/$USER", "/workspace/common"]
}
```

## 공통 변경 (`src/main.c`)

| 변경 | 내용 |
|------|------|
| `expand_user_variable()` 추가 | 경로 문자열에서 `$USER`를 `pw->pw_name`으로 치환 |
| `is_workspace_allowed()` 추가 | HOME 먼저 체크 → workspaces 배열 순회하며 prefix 매칭 |
| workspace 검증 이동 | config 파싱 **후**로 이동 (workspaces 읽은 뒤 검증) |
| 기존 `starts_with(workspace, user_home)` 제거 | `is_workspace_allowed()`로 대체 |
| 오류 메시지 변경 | `"workspace must be under your home directory or a configured workspace path"` |
| 디버그 출력 추가 | 원본 경로 → 치환된 경로 출력 (`/tools/$USER -> /tools/benjamin`) |

**하위 호환성**: `workspaces` 키가 없으면 `ws_array = NULL` → `is_workspace_allowed()`가 HOME만 체크 → 기존 동작 100% 유지.

## Landlock 백엔드 (`src/backend_landlock.c`)

**변경 없음.**

기존 코드에서 이미 workspace(cwd)에 full access 규칙을 추가하고 있음:
```c
/* Workspace: full access */
add_path_rule(ruleset_fd, workspace, fs_access);
```

Landlock은 동일 경로에 여러 규칙이 매칭되면 합집합(OR)으로 적용하므로, 상위가 ro여도 workspace cwd는 rw로 동작.

## Apptainer 백엔드 (`src/backend_apptainer.c`)

| 변경 | 내용 |
|------|------|
| `(void)workspace;` 제거 | workspace를 실제로 사용 |
| workspace rw bind 추가 | mounts bind 뒤에 `-B workspace:workspace:rw` 추가 |
| `max_args` 조정 | workspace bind 2개 슬롯 추가 |

Apptainer는 나중에 추가된 bind가 이전 bind를 override하므로, mounts에서 상위가 ro로 bind되어도 workspace를 rw로 다시 bind하면 cwd에서 쓰기 가능.

## 테스트 결과

12개 전체 PASS (Landlock E2E) + Apptainer ro override 검증 완료.

자세한 결과: [test-results.md](./test-results.md)
