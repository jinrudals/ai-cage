# 환경변수 레퍼런스

ai-cage는 다음 환경변수로 동작을 제어합니다.

| 변수 | 필수 | 설명 |
|---|---|---|
| `WRAPPED_AI_IMAGE_NAME` | △ | 실행할 AI 도구 이름. Apptainer 백엔드에서는 `<images>/<name>.sif` 경로로 변환. Landlock 백엔드에서는 해당 이름을 직접 실행 |
| `WRAPPED_AI_COMMAND` | | 샌드박스 내에서 실행할 명령. 설정 시 `/bin/sh -lc "<command>"` 형태로 실행 |
| `WRAPPED_AI_DEBUG` | | `1`, `true`, `yes` 중 하나로 설정하면 디버그 출력 활성화 |

## 실행 우선순위

| 우선순위 | 조건 | 동작 |
|---|---|---|
| 1 | `WRAPPED_AI_COMMAND` 설정됨 | `sh -lc "<command>"` 실행 |
| 2 | 추가 인자 (`--` 뒤) | 해당 명령 실행 |
| 3 | `WRAPPED_AI_IMAGE_NAME` 설정됨 | 해당 이름으로 실행 (Landlock) / `apptainer run` (Apptainer) |
| 4 | 아무것도 없음 | 로그인 쉘 실행 (Landlock) / 에러 (Apptainer) |

## 플래그

| 플래그 | 설명 |
|---|---|
| `--shell` | 쉘 모드 진입. Apptainer: `apptainer shell`, Landlock: 로그인 쉘 |
| `--` | 이후 인자를 AI 도구에 직접 전달 |
