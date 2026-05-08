# ai-cage

공유 서버 환경에서 Agentic AI 코딩 도구(Kiro CLI, OpenAI Codex 등)를 안전하게 사용하기 위한 Apptainer 컨테이너 기반 격리 래퍼.

여러 사용자가 동시에 사용하는 서버에서 AI 에이전트의 동작이 다른 사용자에게 영향을 미치지 않도록 컨테이너로 격리하되, NFS 등 공유 스토리지는 읽기 전용으로 참조할 수 있도록 바인드 마운트를 자동 구성합니다.

## 의존성

- [Apptainer](https://apptainer.org/) ≥ 1.0
- GCC (또는 C99 호환 컴파일러)
- GNU Autotools (`autoconf`, `automake`) — 소스에서 빌드 시
- `findmnt` (util-linux) — NFS 마운트 자동 감지에 사용

## 빌드

```sh
autoreconf -i
./configure --prefix=/usr/local --with-config-dir=/etc/ai
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
WRAPPED_AI_IMAGE_NAME=codex WRAPPED_AI_COMMAND="codex --help" ai-cage

# 디버그 모드
WRAPPED_AI_DEBUG=1 WRAPPED_AI_IMAGE_NAME=kiro ai-cage

# 컨테이너 셸 진입
WRAPPED_AI_IMAGE_NAME=kiro ai-cage --shell
```

## 구성 요소

- `ai-cage.c` — C 기반 setuid 래퍼 바이너리
- `scripts/codex` — Bash 기반 Codex 래퍼 스크립트
- `examples/images/` — Apptainer 컨테이너 정의 파일 (Kiro, Codex)
- `config.json` — 마운트 및 Apptainer 경로 설정

## 문서

- [docs/configuration.md](docs/configuration.md) — config.json 설정 가이드
- [docs/environment.md](docs/environment.md) — 환경변수 레퍼런스
