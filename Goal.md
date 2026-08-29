# Goal: DualFire P0 코어 완결

- Goal ID: `GOAL-P0-CORE-CLOSURE-20260829`
- 상태: `in_progress`
- 소유자: Sol
- 시작일: 2026-08-29

## 목표

현재 속성 판정, 로드아웃, 생존, 1스테이지 흐름을 유지하면서 P0 전투 화면을 중앙 3:4 판정 필드와 목업형 HUD 배치로 완성한다.

## 완료 조건

- Enhanced Input 기반 Hold/Toggle 저속 이동이 동작한다.
- Ground/Air 적이 데이터 기반 렌더 높이로 구분되며 판정 평면은 유지된다.
- 전투 HUD가 체력, 보호막, 잔기, Special 1/2 속성·입력·쿨다운을 표시한다.
- 전투 HUD가 활성화되어도 기존 gameplay input이 유지된다.
- 4:3~21:9 가로 화면에서 전체 월드는 보이되 판정 필드는 중앙 `900x1200`으로 유지된다.
- 좌우 영역은 충돌 없는 딤 플레인과 재사용 가능한 경계 밴드로 구분된다.
- 전투 HUD는 중앙 필드 내부 좌하·우하에만 배치되고 점수·체인·SUPER 자리를 만들지 않는다.
- DualFireEditor 빌드, CompileAllBlueprints, 관련 Automation Test가 통과한다.
- 사용자 PIE 확인 전에는 `awaiting_user_pie`, 확인 후에만 `complete`로 전환한다.

## 범위 제외

- 실제 보스, 슈퍼웨폰 런타임, 스코어·체인, 디버그 HUD, CSV, F1~F10
- 격납고 UI와 로드아웃 표시 변경
- `Docs/`, worksheet, 사용자 수정 중인 `AGENTS.md`, `GM_Test` redirector
- 현재 Cube 자동 클리어 경로 변경

## 작업 상태

| TASK-ID | 작업 | 상태 | 자동 검증 |
| --- | --- | --- | --- |
| `P0-SLOW-INPUT` | 사용자 설정 기반 Hold/Toggle 저속 이동 | completed | PASS: Editor 빌드, SlowInput Automation 2건, IA/IMC/BP 참조 |
| `P0-ATTRIBUTE-VISUAL` | 데이터 기반 Ground/Air 렌더 높이 | completed | PASS: Editor 빌드, RenderHeight Automation, DT 행·그림자 참조 |
| `P0-LOADOUT-PRESENTATION` | 격납고 속성·입력 표시 | cancelled | 사용자 결정으로 C++·WBP·Automation 변경 초기화 |
| `P0-COMBAT-HUD` | 생존·Special 1/2 전투 HUD | completed | PASS: Editor 빌드, CombatHUD Automation, WBP 컴파일, HUD 텍스처 알파·참조 검사 |
| `P0-INPUT-REGRESSION` | CombatHUD 활성화 후 gameplay input 유지 | completed | PASS: PIE 로그 원인 확인, Game 입력 모드 Automation, Editor 빌드 |
| `P0-INTEGRATION-VERIFY` | 빌드·Blueprint·Automation 회귀 | completed | PASS: Editor 빌드, Blueprint 오류·경고·로드 실패 0, DualFire Automation 7/7 |
| `P0-VIEWPORT-FRAME` | 전체 뷰와 중앙 3:4 판정 필드 분리 | in_progress | pending |
| `P0-FIELD-BOUNDARY` | 좌우 딤 플레인과 그림자·에지 밴드 | pending | pending |
| `P0-HUD-MOCKUP-LAYOUT` | 필드 내부 좌하·우하 전투 HUD 재배치 | pending | pending |
| `P0-FRAMING-VERIFY` | 빌드·Blueprint·Automation 회귀 | pending | pending |

## PIE 인계

- 담당: 사용자
- 상태: `not_run`
- 확인 항목: 16:9 좌우 월드 노출, 중앙 3:4 이동·스폰·탄환 경계, 스크롤 중 경계 밴드 정렬과 투명 정렬, 필드 내부 HUD 배치, 기존 이동·발사·저속 입력, 생존 HUD 갱신, Special 1/2 독립 쿨다운, Cube 자동 클리어
