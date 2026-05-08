# config.json 설정 가이드

`config.json`은 ai-cage가 Apptainer 컨테이너를 실행할 때 참조하는 설정 파일입니다.

기본 경로: `--with-config-dir`로 지정한 디렉토리 아래 `config.json` (기본값: `/etc/ai/config.json`)

## 구조

```json
{
  "apptainer": {
    "binary": "/path/to/apptainer",
    "images": "/path/to/images/directory"
  },
  "mounts": {
    "read-only": [ ... ],
    "read-write": [ ... ]
  }
}
```

## apptainer

| 키 | 설명 |
|---|---|
| `binary` | apptainer 실행 파일 경로 |
| `images` | `.sif` 이미지가 위치한 디렉토리. `WRAPPED_AI_IMAGE_NAME`에 지정한 이름으로 `<images>/<name>.sif`를 찾음 |

## mounts

바인드 마운트 규칙을 정의합니다. `read-only`와 `read-write` 배열로 구분됩니다.

각 항목은 다음 중 하나:

### exact-matched

```json
{ "exact-matched": "/tools" }
```

지정한 경로가 존재하면 그대로 바인드 마운트합니다.

### NFS-mounts-starts-with

```json
{ "NFS-mounts-starts-with": "/data" }
```

`findmnt`로 감지된 NFS 마운트 중 해당 접두사로 시작하는 모든 마운트를 바인드합니다.
