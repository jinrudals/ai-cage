# Data Model: Workspaces Configuration

## config.json Schema

```json
{
  "mounts": {
    "read-only": [...],
    "read-write": [...]
  },
  "workspaces": [
    "/workspace/project/$USER",
    "/workspace/project/common"
  ]
}
```

## Entities

### Workspace Entry (string)

| Attribute | Type | Description |
|-----------|------|-------------|
| path | string | 절대 경로. `$USER` 포함 시 실행 시 치환 |

### Validation Rules

- 각 항목은 `/`로 시작하는 절대 경로여야 함
- `$USER`는 경로 내 어디든 위치 가능 (보통 마지막 세그먼트)
- 빈 배열 또는 키 부재 시 기존 동작 유지

## State Transitions

없음. 정적 설정 파일이며 실행 시마다 읽음.

## Relationships

```
config.json
├── mounts (기존) → MountList 구성
└── workspaces (신규) → workspace 검증에 사용
                        cwd가 매칭되면 실행 허용
                        cwd는 항상 rw로 backend_run에 전달됨
```
