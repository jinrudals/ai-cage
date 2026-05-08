# 환경변수 레퍼런스

ai-cage는 다음 환경변수로 동작을 제어합니다.

| 변수 | 필수 | 설명 |
|---|---|---|
| `WRAPPED_AI_IMAGE_NAME` | ✓ | 실행할 컨테이너 이미지 이름 (예: `kiro`, `codex`). `<images>/<name>.sif` 경로로 변환됨 |
| `WRAPPED_AI_COMMAND` | | 컨테이너 내에서 실행할 명령. 설정 시 `apptainer exec ... /bin/sh -lc "<command>"` 형태로 실행 |
| `WRAPPED_AI_DEBUG` | | `1`, `true`, `yes` 중 하나로 설정하면 디버그 출력 활성화 |

## 실행 모드

| 조건 | 모드 | apptainer 서브커맨드 |
|---|---|---|
| `--shell` 플래그 | 셸 모드 | `apptainer shell` |
| `WRAPPED_AI_COMMAND` 설정됨 | exec 모드 | `apptainer exec` |
| 그 외 | run 모드 | `apptainer run` |
