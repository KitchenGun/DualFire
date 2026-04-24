# DualFire

Unreal Engine 5.5 기반 2D 스크롤 슈팅 게임 프로젝트.  
Paper2D + XZ 평면 이동, 단일 플레이어 구성.

---

## 개발 환경

| 항목 | 값 |
|------|-----|
| 엔진 | Unreal Engine 5.5 |
| 렌더링 | DX12, Ray Tracing, Virtual Shadow Map |
| 이동 평면 | XZ (X=좌우, Z=상하, Y 고정) |
| 카메라 | 직교 투영 (OrthoWidth 2048) |

---

## 프로젝트 구조

```
DualFire/
├── Config/
│   └── DefaultEngine.ini        # 콜리전 채널, GameMode, 렌더링 설정
├── Content/
│   └── Blueprint/
│       ├── Camera/              # BP_StageCameraActor 등
│       ├── GameMode/            # BP_DualFireGameModeBase 등
│       └── Player/              # BP_DualFirePlayerPawn 등
└── Source/DualFire/
    ├── Camera/
    │   └── StageCameraActor     # 스테이지 카메라 액터
    ├── Core/
    │   └── DualFireCollisionChannels  # 커스텀 콜리전 채널/프로필 정의
    ├── GameModes/
    │   └── DualFireGameModeBase # 게임 모드 (카메라 스폰·관리)
    └── Player/
        ├── DualFirePlayerPawn   # 플레이어 폰
        └── DualFireMovementComponent  # XZ 이동 컴포넌트
```

---

## 클래스 개요

### `ADualFirePlayerPawn`
플레이어 캐릭터. Enhanced Input → `AddMovementInput` → `MovementComp` 흐름으로 이동.

| 컴포넌트 | 역할 |
|----------|------|
| `SceneRoot` | 루트, MovementComp의 UpdatedComponent |
| `ShipFlipbook` | Paper2D 비주얼 (충돌 없음) |
| `HitboxComp` | 피격 감지 전용 SphereComponent |
| `MovementComp` | XZ 이동, 화면 경계 클램핑 |

### `UDualFireMovementComponent`
`PawnMovementComponent` 기반 2D 이동. 가속도 보간(`VInterpConstantTo`) + `StageCameraActor::GetPlayableBounds()` 기반 화면 경계 클램핑.

| 프로퍼티 | 기본값 | 설명 |
|----------|--------|------|
| `MoveSpeed` | 900 | 최고 이동 속도 (units/s) |
| `Acceleration` | 8000 | 가속도 (units/s²) |
| `SpeedMultiplier` | 1.0 | 슈퍼웨폰 둔화/가속 배율 |
| `bMovementLocked` | false | 스턴·연출 중 이동 잠금 |

### `AStageCameraActor`
GameMode가 BeginPlay에서 스폰 후 PlayerController의 ViewTarget으로 설정.  
직교 카메라를 소유하며 +X 방향 자동 스크롤 및 플레이 가능 영역(`FBox2D`) 제공.

| 프로퍼티 | 기본값 | 설명 |
|----------|--------|------|
| `OrthoWidth` | 2048 | 직교 투영 너비 (월드 단위) |
| `ScrollSpeed` | 0 | +X 자동 스크롤 속도 (units/s) |
| `PlayableInset` | (64, 64) | 이동 가능 영역 마진 |

| 함수 | 설명 |
|------|------|
| `GetPlayableBounds()` | XZ 이동 가능 영역 반환 (FBox2D) |
| `SetScrollSpeed(float)` | 스크롤 속도 변경 |
| `SetPaused(bool)` | 카메라 일시 정지 |

### `ADualFireGameModeBase`
스테이지 카메라를 스폰·소유하고 외부에 접근자(`GetStageCamera()`)를 노출.  
`DefaultEngine.ini`의 `GlobalDefaultGameMode`로 자동 적용.

---

## 콜리전 채널

| 채널 | 용도 |
|------|------|
| `PlayerHitbox` (Ch1) | 플레이어 히트박스 |
| `EnemyBullet` (Ch2) | 적 총알 |
| `PlayerBullet` (Ch3) | 플레이어 총알 |
| `EnemyBody` (Ch4) | 적 몸통 |

콜리전 프로필은 `Config/DefaultEngine.ini`와 `Source/DualFire/Core/DualFireCollisionChannels.h`에 동기화 정의.

---

## 활성 플러그인

| 플러그인 | 범위 |
|----------|------|
| Paper2D | Runtime |
| CommonUI / CommonInput | Runtime |
| DataRegistry | Runtime |
| ModelingToolsEditorMode | Editor |
| UnrealMCP | Editor |

---

## 구현 현황

| 기능 | 상태 |
|------|------|
| 플레이어 이동 (XZ 평면) | ✅ 완료 |
| 화면 경계 클램핑 | ✅ 완료 |
| 스테이지 카메라 분리 | ✅ 완료 |
| 자동 스크롤 | ✅ 완료 (ScrollSpeed 설정) |
| 피격 감지 (히트박스) | ✅ 완료 |
| HP / 잔기 시스템 | 🔲 미구현 |
| 사격 시스템 | 🔲 미구현 |
| 적 / AI | 🔲 미구현 |
| UI / HUD | 🔲 미구현 |
| 사운드 / VFX | 🔲 미구현 |

---

## Blueprint 설정 (PIE 실행 전 필수)

1. `BP_StageCameraActor` 생성 (`AStageCameraActor` 상속)
2. `BP_DualFireGameModeBase` 생성 (`ADualFireGameModeBase` 상속)  
   → Details > Camera > `StageCameraClass`에 `BP_StageCameraActor` 할당
3. `BP_DualFirePlayerPawn` 생성 (`ADualFirePlayerPawn` 상속)  
   → Details > Input에 `IMC_Player`, `IA_Move` 에셋 할당
