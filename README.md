# ai-cage

Apptainer 컨테이너 기반 AI CLI 래퍼. NFS 마운트를 자동 감지하고 설정 파일에 따라 바인드 마운트를 구성하여 AI 코딩 도구(Kiro CLI, OpenAI Codex)를 격리된 환경에서 실행합니다.

## 구성 요소

- `ai-cage.c` — C 기반 setuid 래퍼 바이너리
- `scripts/codex` — Bash 기반 Codex 래퍼 스크립트
- `examples/images/` — Apptainer 컨테이너 정의 파일 (Kiro, Codex)
- `config.json` — 마운트 및 Apptainer 경로 설정

## 빌드

```sh
./configure --prefix=/usr/local --config-dir=/etc/ai
make build
```

## 설치

```sh
sudo make install
sudo make install-config
```
