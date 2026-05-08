# ai-cage

Apptainer 컨테이너 기반 AI CLI 래퍼. NFS 마운트를 자동 감지하고 설정 파일에 따라 바인드 마운트를 구성하여 AI 코딩 도구(Kiro CLI, OpenAI Codex)를 격리된 환경에서 실행합니다.

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
