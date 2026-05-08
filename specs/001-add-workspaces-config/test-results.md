# Test Results: 001-add-workspaces-config

**Date**: 2026-05-08
**Backend**: Landlock
**Build**: `gcc -Wall -Wextra -O2` — 경고/에러 0개

## 기능 테스트

| # | 시나리오 | Config | cwd | 기대 | 결과 |
|---|----------|--------|-----|------|------|
| 1 | HOME 하위 실행 (기존 동작) | workspaces 있음 | `$HOME/workspace/...` | 성공 | ✅ PASS |
| 2 | `$USER` workspace 경로에서 실행 | `["/tmp/ws/$USER"]` | `/tmp/ws/benjamin/project` | 성공 | ✅ PASS |
| 3 | 공용 경로 (`$USER` 없음) | `["/tmp/ws/common"]` | `/tmp/ws/common/shared` | 성공 | ✅ PASS |
| 4 | 다른 사용자 workspace 거부 | `["/tmp/ws/$USER"]` | `/tmp/ws/otheruser/project` | 거부 | ✅ PASS |
| 5 | 미허용 경로 거부 | `["/tmp/ws/$USER"]` | `/tmp/random-dir` | 거부 | ✅ PASS |
| 6 | HOME 하위 호환성 | workspaces 있음 | `$HOME` | 성공 | ✅ PASS |
| 7 | workspaces 미설정 + HOME 하위 | workspaces 키 없음 | `$HOME` | 성공 | ✅ PASS |
| 8 | workspaces 미설정 + HOME 밖 | workspaces 키 없음 | `/tmp/ws/common/shared` | 거부 | ✅ PASS |

## Landlock 권한 override 테스트

| # | 시나리오 | 기대 | 결과 |
|---|----------|------|------|
| 9 | `/tools` = ro mount, cwd = `/tools/benjamin/project` (workspace) → 파일 쓰기 | 쓰기 성공 | ✅ PASS |
| 10 | 동일 상태에서 `/tools/ai-cage/` (workspace 밖)에 쓰기 | 차단 | ✅ PASS |

## 디버그 출력 테스트

| # | 시나리오 | 기대 출력 | 결과 |
|---|----------|-----------|------|
| 11 | `WRAPPED_AI_DEBUG=1` + workspaces 설정 | stderr에 치환된 경로 출력 | ✅ PASS |

**디버그 출력 예시**:
```
WORKSPACE=/tools/benjamin/project
Workspaces (1):
  /tools/$USER -> /tools/benjamin
Mounts (1):
  /tools [ro]
```

## 오류 메시지 테스트

| # | 시나리오 | 출력 메시지 | 결과 |
|---|----------|-------------|------|
| 12 | 미허용 경로에서 실행 | `FAIL: workspace must be under your home directory or a configured workspace path` | ✅ PASS |

## 요약

- **전체**: 12/12 PASS
- **하위 호환성**: 기존 config(workspaces 미설정)에서 동작 변경 없음
- **보안**: 다른 사용자 경로 및 미등록 경로 차단 확인
- **Landlock override**: ro mount 상위에서도 workspace cwd는 rw 정상 동작
