# DualFire 개발 원칙

DualFire의 코드는 사람이 읽기 쉽고, AI 에이전트가 안전하게 수정할 수 있고, 자동화로 검증하기 쉬워야 한다.

## Codex 운영 규칙

- 작업 시작 시 `Goal.md`와 worksheet 관련 파일이 있으면 먼저 확인한다.
- `Goal.md`와 worksheet 상태 수정은 Sol 지휘자만 수행한다.
- 작업자는 배정된 `TASK-ID`와 관련 자료만 읽고, 범위 밖 판단은 Sol에 반환한다.
- 모델 배정은 `.codex/MODEL_ROUTING.md`를 따른다.
- 동일 파일을 여러 작업자가 동시에 수정하지 않는다.
- 작업별 writer는 한 명만 둔다.
- 읽기 작업은 우선 병렬화하고, 쓰기 작업은 소유권을 분리한다.
- 장기 독립 수정은 별도 세션과 Git Worktree를 사용한다.
- 하위 에이전트가 다시 하위 에이전트를 생성하지 않는다.
- 모든 작업은 완료조건, 검증 결과, 인계 형식을 남긴다.

## 핵심 가치

| 가치 | 기준 |
| --- | --- |
| 의도 명확성 | 클래스, 함수, 변수 이름만 봐도 역할과 수정 범위가 드러나야 한다. |
| 좁은 수정 범위 | 기능 책임을 파일/클래스/함수 단위로 분리해 한 변경이 다른 영역을 건드리지 않게 한다. |
| 검증 가능성 | 빌드, Blueprint 컴파일, PIE 로그, 작은 검증 함수로 변경 결과를 확인할 수 있어야 한다. |
| 숨은 규칙 최소화 | 유효성, 호출 순서, 소유권, 수명주기 규칙은 타입, 이름, `ensure`, `check`, 로그로 드러낸다. |
| 왜 중심 주석 | 코드가 설명하지 못하는 설계 이유만 짧게 주석으로 남긴다. |
| 단순한 구조 | 실제 중복과 복잡도를 줄이지 못하는 추상화는 추가하지 않는다. |

## 작업 규칙

- `Docs/` 기획 문서는 사용자가 명시적으로 요청하지 않으면 수정하지 않는다.
- 기존 Blueprint/C++ 에셋을 삭제하거나 대체하기보다 호환 경로를 먼저 둔다.
- Data와 Logic은 분리한다. 스테이지/적/무장 값은 가능한 DataTable 또는 BP 기본값으로 둔다.
- Actor Spawn/Destroy 반복 경로는 먼저 `UActorPoolSubsystem` 사용 가능 여부를 확인한다.
- 다수 액터 Tick은 개별 Tick보다 중앙 매니저 Tick을 우선 검토한다.
- AI가 수정해야 할 진입점은 이름으로 드러나야 한다. 예: `InitFromEnemyRow`, `ApplyRuntimeConfig`, `RegisterEnemyAI`.
- 포인터, 데이터 행, 클래스 참조가 없을 수 있는 경계에는 `IsValid`, `ensure`, 경고 로그 중 하나를 둔다.

## 검증 기준

작업 후 가능한 범위에서 아래를 실행한다.

```powershell
D:\UE_5.8\Engine\Build\BatchFiles\Build.bat DualFireEditor Win64 Development -Project=E:\DualFire\DualFire.uproject -WaitMutex
D:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe E:\DualFire\DualFire.uproject -run=CompileAllBlueprints -ProjectOnly -Unattended -NullRHI -NoSound -nop4 -nosplash
```

PIE 검증이 필요한 작업은 Unreal MCP `EditorToolset.EditorAppToolset.StartPIE`를 우선 사용한다.
