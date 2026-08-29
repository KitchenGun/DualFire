# Goal: DualFire P0 코어 완결

- Goal ID: `GOAL-P0-CORE-CLOSURE-20260829`
- 상태: `in_progress`
- 소유자: Sol
- 시작일: 2026-08-29

## 목표

현재 속성 판정, 로드아웃, 생존, 1스테이지 흐름을 유지하면서 P0에서 빠진 저속 이동, Ground/Air 시각 식별, 최소 전투 HUD를 완성한다.

## 완료 조건

- Enhanced Input 기반 Hold/Toggle 저속 이동이 동작한다.
- Ground/Air 적이 데이터 기반 렌더 높이로 구분되며 판정 평면은 유지된다.
- 전투 HUD가 체력, 보호막, 잔기, Special 1/2 속성·입력·쿨다운을 표시한다.
- 전투 HUD가 활성화되어도 기존 gameplay input이 유지된다.
- DualFireEditor 빌드, CompileAllBlueprints, 관련 Automation Test가 통과한다.
- 사용자 PIE 확인 전에는 `awaiting_user_pie`, 확인 후에만 `complete`로 전환한다.

## 범위 제외

- 실제 보스, 슈퍼웨폰 런타임, 스코어·체인, 디버그 HUD, CSV, F1~F10
- `Docs/`, worksheet, 사용자 수정 중인 `AGENTS.md`
- 현재 Cube 자동 클리어 경로 변경

## 작업 상태

| TASK-ID | 작업 | 상태 | 자동 검증 |
| --- | --- | --- | --- |
| `P0-SLOW-INPUT` | 사용자 설정 기반 Hold/Toggle 저속 이동 | completed | PASS: Editor 빌드, SlowInput Automation 2건, IA/IMC/BP 참조 |
| `P0-ATTRIBUTE-VISUAL` | 데이터 기반 Ground/Air 렌더 높이 | completed | PASS: Editor 빌드, RenderHeight Automation, DT 행·그림자 참조 |
| `P0-LOADOUT-PRESENTATION` | 격납고 속성·입력 표시 | cancelled | 사용자 결정으로 C++·WBP·Automation 변경 초기화 |
| `P0-COMBAT-HUD` | 생존·Special 1/2 전투 HUD | completed | PASS: Editor 빌드, CombatHUD Automation, WBP 컴파일, HUD 텍스처 알파·참조 검사 |
| `P0-INPUT-REGRESSION` | CombatHUD 활성화 후 gameplay input 유지 | pending | PIE 로그에서 Menu 입력 모드 전환 원인 확인 |
| `P0-INTEGRATION-VERIFY` | 빌드·Blueprint·Automation 회귀 | pending | not_run after reset |

## PIE 인계

- 담당: 사용자
- 상태: `not_run`
- 확인 항목: 기존 이동·발사 입력, 저속 Hold/Toggle, Ground/Air 그림자 간격과 판정, 전투 HUD 속성·입력, 생존 HUD 갱신, Special 1/2 독립 쿨다운, Cube 자동 클리어
