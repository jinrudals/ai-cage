# ai-cage

공유 서버 환경에서 Agentic AI 코딩 도구(Kiro CLI, OpenAI Codex 등)를 안전하게 사용하기 위한 격리 래퍼.

여러 사용자가 동시에 사용하는 서버에서 AI 에이전트의 동작이 다른 사용자에게 영향을 미치지 않도록 격리하되, NFS 등 공유 스토리지는 읽기 전용으로 참조할 수 있도록 자동 구성합니다.

## 백엔드

두 가지 샌드박싱 백엔드를 지원합니다:

| 백엔드 | 설명 | 요구사항 |
|--------|------|----------|
| **Landlock** (기본) | 커널 LSM 기반 파일시스템 접근 제어. 컨테이너 없이 동작 | Linux 커널 ≥ 5.13 |
| **Apptainer** | Apptainer 컨테이너 기반 완전 격리 | [Apptainer](https://apptainer.org/) ≥ 1.0 |

## 의존성

- GCC (또는 C99 호환 컴파일러)
- GNU Autotools (`autoconf`, `automake`) — 소스에서 빌드 시
- `findmnt` (util-linux) — NFS 마운트 자동 감지에 사용
- Apptainer ≥ 1.0 — Apptainer 백엔드 사용 시

## 빌드

```sh
autoreconf -i

# Landlock 백엔드 (기본)
./configure --prefix=/usr/local --with-config-dir=/etc/ai

# Apptainer 백엔드
./configure --prefix=/usr/local --with-config-dir=/etc/ai \
    --with-backend=apptainer \
    --with-apptainer-bin=/path/to/apptainer \
    --with-apptainer-images=/path/to/images

make
```

## 설치

```sh
sudo make install
```

## 사용법

```sh
# Kiro CLI 실행
WRAPPED_AI_IMAGE_NAME=kiro ai-cage

# Codex 실행 (커맨드 래핑)
WRAPPED_AI_COMMAND="codex --help" ai-cage

# 디버그 모드
WRAPPED_AI_DEBUG=1 WRAPPED_AI_IMAGE_NAME=kiro ai-cage

# 셸 진입
WRAPPED_AI_IMAGE_NAME=kiro ai-cage --shell
```

## 구성 요소

- `src/main.c` — 공통 진입점 (config 파싱, 마운트 목록 구성)
- `src/backend_landlock.c` — Landlock LSM 백엔드
- `src/backend_apptainer.c` — Apptainer 컨테이너 백엔드
- `src/backend.h` — 백엔드 인터페이스 정의
- `config.json` — 마운트 정책 및 워크스페이스 설정
- `scripts/codex` — Bash 기반 Codex 래퍼 스크립트
- `examples/images/` — Apptainer 컨테이너 정의 파일 (Kiro, Codex)

## 문서

- [docs/configuration.md](docs/configuration.md) — config.json 설정 가이드
- [docs/environment.md](docs/environment.md) — 환경변수 레퍼런스
