# DualFire 개발 현황

작성 기준: 2026-07-06 / Unreal Engine 5.8 / `main`

이 문서는 외부에서 GitHub 저장소를 볼 때 현재 구현 단계와 검증 범위를 빠르게 파악하기 위한 요약입니다.
개발 기준은 `AGENTS.md`의 "사람이 읽기 쉽고, AI 에이전트가 안전하게 수정할 수 있고, 자동화로 검증하기 쉬운 코드" 원칙을 따른다.

## 현재 요약

DualFire는 자동 스크롤 기반 탑다운 슈팅 프로토타입입니다. 현재 핵심 기반은 플레이어 이동, 고정 플레이 영역 카메라, 로드아웃 기반 3슬롯 발사, 오브젝트 풀, DataTable 기반 일반 적 웨이브까지 구현되어 있습니다.

## 구현 완료

| 영역 | 상태 | 주요 파일/에셋 |
| --- | --- | --- |
| 카메라 워크 | 완료 | `Source/DualFire/Camera/StageCameraActor.*` |
| 플레이 영역 | 완료 | 1600x1200, 4:3 고정, `PlayableInset` 적용 |
| 자동 스크롤 | 완료 | +X 방향 200cm/s, 플레이어 동행 보정 |
| 플레이어 이동 | 완료 | `DualFireMovementComponent`, 카메라 bounds 기반 clamp |
| 로드아웃 데이터 모델 | 1차 완료 | `DT_Loadout*`, `LoadoutManagerSubsystem`, `LoadoutDataLibrary` |
| 플레이어 무장 | 1차 완료 | 3슬롯 발사, 런타임 projectile config 주입 |
| 탄환 풀링 | 완료 | `ActorPoolSubsystem`, `PoolableActor`, `BaseProjectile` |
| 일반 적 2종 | 완료 | `BP_EnemyAir`, `BP_EnemyGround` |
| 적 AI 중앙 틱 | 완료 | `StageController`가 bounds/player 위치 1회 계산 후 AI 순회 |
| DataTable 웨이브 | 완료 | `DT_Enemies`, `DT_Waves`, `BP_StageController` |
| 임시 클리어 | 완료 | 엘리트 미구현 전까지 80초 도달 시 mission clear 처리 |
| 적 스폰 방향 보정 | 완료 | `StageController`: 모델 정면(+X)과 이동 방향(-X) 불일치 해결(180도 회전 스폰) |
| 발사체 비주얼 | 완료 | `BaseProjectile`: 언릿 구체 메시 + `ProjectileColor` 파라미터. 플레이어=흰색, `BP_EnemyProjectile`=빨간색으로 구분 |

## 적 구현 범위

| 적 | 속성 | HP | 이동 | 공격 |
| --- | --- | --- | --- | --- |
| `ENEMY_AIR` | Air | 1 | Linear | 플레이어 조준 단발, 2.0초 간격, 탄속 400 |
| `ENEMY_GROUND` | Ground | 2 | EnterStop | 플레이어 조준 단발, 1.5초 간격, 탄속 350 |

`DT_Waves`는 일반몹 웨이브만 포함합니다. 양면 적은 엘리트 전용으로 분리되어 W09/W10/W12는 현재 제외되어 있습니다.

## 최근 검증

| 검증 | 결과 |
| --- | --- |
| `Build.bat DualFireEditor Win64 Development` | 성공 |
| `CompileAllBlueprints -ProjectOnly` | 0 errors / 0 warnings |
| UE Python 에셋 검증 | `DT_Enemies`, `DT_Waves`, Enemy/Stage/GameMode BP 연결 확인 |
| Unreal MCP PIE | W01/W02/W03/W04 웨이브 트리거 로그 확인 |
| PIE 런타임 프로퍼티 조회 | 적 스폰 회전 yaw=180 확인, 살아있는 발사체의 `ProjectileColor`가 적=빨강/플레이어=흰색으로 분리됨을 확인 |

## 아직 미구현/후속 작업

| 영역 | 상태 |
| --- | --- |
| 엘리트/보스 전투 | 미구현 |
| 탄환 중앙 틱/탄막 최적화 | 미구현, 필요 시 프로파일링 후 승격 |
| 장기 스트레스 테스트 | 미완료 |
| UI/HUD | 미구현 |
| 점수/보상/진행도 | 미구현 |
| 사운드/VFX/피격 연출 | 미구현 (발사체는 임시 언릿 구체로만 구분) |
| 스테이지 데이터 확장 | 1차 프로토타입만 작성 |

## 다음 우선순위

1. PIE에서 80초 전체 진행 검증 및 풀 재사용 로그 확인
2. 적 200체 + 연사 스트레스 테스트로 Game thread 비용 확인
3. 엘리트 전용 양면 적/패턴 설계
4. HUD와 피격/클리어/실패 피드백 추가

## 참고 커밋

| 커밋 | 내용 |
| --- | --- |
| `c78cc9c` | 발사체에 육안 식별용 비주얼 추가, 적 탄환은 빨간색으로 구분 |
| `5c0f563` | 적 스폰 회전을 이동 방향(-X)에 맞춰 모델 정면과 진행 방향 일치 |
| `8c45f38` | 일반 적 2종 데이터와 스테이지 에셋 추가 |
| `6b906ea` | 중앙 틱 기반 일반 적 웨이브 처리 구현 |
| `60edfe2` | 오브젝트 풀 기반 탄환 수명주기 추가 |
| `901aff3` | 프로토타입 카메라 검증 항목 확정 |
| `5383bd4` | 카메라 자동 스크롤과 플레이 영역 고정 구현 |
