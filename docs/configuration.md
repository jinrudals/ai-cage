# config.json 설정 가이드

`config.json`은 ai-cage가 샌드박스를 구성할 때 참조하는 설정 파일입니다.

기본 경로: `--with-config-dir`로 지정한 디렉토리 아래 `config.json` (기본값: `/etc/ai/config.json`)

## 구조

```json
{
  "mounts": {
    "read-only": [ ... ],
    "read-write": [ ... ]
  }
}
```

## mounts

접근 정책을 정의합니다. `read-only`와 `read-write` 배열로 구분됩니다.

- **Landlock 백엔드**: 해당 경로에 대한 파일시스템 접근 권한을 Landlock LSM으로 제어
- **Apptainer 백엔드**: 해당 경로를 컨테이너에 바인드 마운트

각 항목은 다음 중 하나:

### exact-matched

```json
{ "exact-matched": "/tools" }
```

지정한 경로가 존재하면 그대로 정책에 추가합니다.

### NFS-mounts-starts-with

```json
{ "NFS-mounts-starts-with": "/data" }
```

`findmnt`로 감지된 NFS 마운트 중 해당 접두사로 시작하는 모든 마운트를 추가합니다.

## Landlock 백엔드 기본 정책

config.json의 마운트 정책 외에, Landlock 백엔드는 다음을 자동으로 허용합니다:

| 경로 | 권한 | 용도 |
|------|------|------|
| 현재 작업 디렉토리 | rw | AI 에이전트 작업 공간 |
| `$HOME` | rw | 쉘 설정, dotfiles 등 |
| `/usr`, `/lib`, `/bin`, `/sbin`, `/etc` 등 | ro | 시스템 바이너리, 라이브러리 |
| `/dev` | ro + write | `/dev/null`, `/dev/tty` 등 |
| `/run` | ro + write | DNS 소켓, D-Bus 등 |
| `/tmp` | rw | 임시 파일 |
| `/mnt` | ro | WSL resolv.conf 등 심볼릭 링크 대상 |

## Apptainer 백엔드 설정

Apptainer 백엔드 사용 시 `--with-apptainer-bin`과 `--with-apptainer-images` configure 옵션으로 경로를 지정합니다. config.json에는 마운트 정책만 기술합니다.
