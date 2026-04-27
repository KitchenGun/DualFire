// Copyright DualFire. All Rights Reserved.

#include "DualFirePlayerPawn.h"

#include "DualFireMovementComponent.h"
#include "Core/DualFireCollisionChannels.h"
#include "Weapon/WeaponComponent.h"

#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"

ADualFirePlayerPawn::ADualFirePlayerPawn()
{
    // 중력/물리 없는 2D 슈팅 — Tick 불필요
    PrimaryActorTick.bCanEverTick = false;

    // 카메라를 StageCameraActor로 이관 — Pawn Possess 시 카메라 탐색 비활성화
    bFindCameraComponentWhenViewTarget = false;

    // ── SceneRoot ────────────────────────────────────────────────────────────
    // MovementComp의 UpdatedComponent 역할. 비주얼과 히트박스를 분리해 책임 명확화.
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    // ── Mesh (비주얼 전담) ────────────────────────────────────────────────
    Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(SceneRoot);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 충돌은 HitboxComp 전담

    // ── HitboxComp (피격 감지 전담) ───────────────────────────────────────────
    // Profile="PlayerPawn": ObjectType=PlayerHitbox, EnemyBullet=Overlap, 나머지 Ignore
    HitboxComp = CreateDefaultSubobject<USphereComponent>(TEXT("HitboxComp"));
    HitboxComp->SetupAttachment(SceneRoot);
    HitboxComp->InitSphereRadius(12.f);
    HitboxComp->SetCollisionProfileName(DualFireProfile::PlayerPawn);
    // OnComponentBeginOverlap 바인딩은 BeginPlay에서 (CDO 단계에서 AddDynamic 불가)

    // ── MovementComp ─────────────────────────────────────────────────────────
    // UpdatedComponent를 SceneRoot로 설정해 XY 평면 이동 대상을 루트로 지정
    MovementComp = CreateDefaultSubobject<UDualFireMovementComponent>(TEXT("MovementComp"));
    MovementComp->UpdatedComponent = SceneRoot;

    // ── WeaponComp ────────────────────────────────────────────────────────────
    WeaponComp = CreateDefaultSubobject<UWeaponComponent>(TEXT("WeaponComp"));
}

// ── APawn 오버라이드 ──────────────────────────────────────────────────────────

void ADualFirePlayerPawn::BeginPlay()
{
    Super::BeginPlay();

    // 동적 델리게이트 바인딩 — BeginPlay에서만 가능 (UFUNCTION 리플렉션 필요)
    HitboxComp->OnComponentBeginOverlap.AddDynamic(
        this, &ADualFirePlayerPawn::OnHitboxOverlapBegin);

    SetupInputMappingContext();
}

void ADualFirePlayerPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    if (!IsValid(EIC))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ADualFirePlayerPawn: EnhancedInputComponent가 없습니다. "
                 "프로젝트 설정 > Input > Default Input Component Class를 확인하세요."));
        return;
    }

    // IA_Move: Triggered — 키를 누르는 동안 매 프레임 호출
    if (UInputAction* MoveIA = IA_Move.LoadSynchronous())
    {
        EIC->BindAction(MoveIA, ETriggerEvent::Triggered,
            this, &ADualFirePlayerPawn::OnMoveInput);
    }
    else
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ADualFirePlayerPawn: IA_Move 에셋이 할당되지 않았습니다."));
    }

    // 발사 입력 — Triggered: 키를 누르는 동안 연사 (쿨다운으로 발사 간격 제어)
    if (UInputAction* GroundIA = IA_FireGround.LoadSynchronous())
    {
        EIC->BindAction(GroundIA, ETriggerEvent::Triggered,
            this, &ADualFirePlayerPawn::OnFireGroundInput);
    }

    if (UInputAction* AirIA = IA_FireAir.LoadSynchronous())
    {
        EIC->BindAction(AirIA, ETriggerEvent::Triggered,
            this, &ADualFirePlayerPawn::OnFireAirInput);
    }

    if (UInputAction* UniversalIA = IA_FireUniversal.LoadSynchronous())
    {
        EIC->BindAction(UniversalIA, ETriggerEvent::Triggered,
            this, &ADualFirePlayerPawn::OnFireUniversalInput);
    }
}

UPawnMovementComponent* ADualFirePlayerPawn::GetMovementComponent() const
{
    return MovementComp;
}

// ── Enhanced Input 핸들러 ─────────────────────────────────────────────────────

void ADualFirePlayerPawn::OnMoveInput(const FInputActionValue& Value)
{
    // IA_Move Axis2D: X=좌우 입력 → 월드 Y축, Y=앞뒤 입력 → 월드 X축
    const FVector2D Axis = Value.Get<FVector2D>();

    AddMovementInput(FVector::RightVector,   Axis.X);
    AddMovementInput(FVector::ForwardVector, Axis.Y);
}

// ── 발사 핸들러 ───────────────────────────────────────────────────────────────

void ADualFirePlayerPawn::OnFireGroundInput(const FInputActionValue& Value)
{
    if (IsValid(WeaponComp))
    {
        WeaponComp->FireGround();
    }
}

void ADualFirePlayerPawn::OnFireAirInput(const FInputActionValue& Value)
{
    if (IsValid(WeaponComp))
    {
        WeaponComp->FireAir();
    }
}

void ADualFirePlayerPawn::OnFireUniversalInput(const FInputActionValue& Value)
{
    if (IsValid(WeaponComp))
    {
        WeaponComp->FireUniversal();
    }
}

// ── 히트박스 오버랩 ───────────────────────────────────────────────────────────

void ADualFirePlayerPawn::OnHitboxOverlapBegin(
    UPrimitiveComponent* OverlappedComp,
    AActor*              OtherActor,
    UPrimitiveComponent* OtherComp,
    int32                OtherBodyIndex,
    bool                 bFromSweep,
    const FHitResult&    SweepResult)
{
    // EnemyBullet 채널(ECC_GameTraceChannel2)이 아닌 오버랩은 무시
    if (!IsValid(OtherComp) ||
        OtherComp->GetCollisionObjectType() != DualFireChannel::EnemyBullet)
    {
        return;
    }

    // HP/잔기 시스템 미구현 단계 — 피격 감지 확인용 로그
    // TODO: 추후 TakeHit() 또는 GameMode의 OnPlayerHit() 호출로 교체
    UE_LOG(LogTemp, Warning,
        TEXT("ADualFirePlayerPawn [%s]: EnemyBullet 피격 감지 — OtherActor=%s"),
        *GetName(),
        IsValid(OtherActor) ? *OtherActor->GetName() : TEXT("None"));
}

// ── 이동 제어 위임 ────────────────────────────────────────────────────────────

void ADualFirePlayerPawn::SetSpeedMultiplier(float InMultiplier)
{
    if (IsValid(MovementComp))
    {
        MovementComp->SetSpeedMultiplier(InMultiplier);
    }
}

void ADualFirePlayerPawn::SetMovementLocked(bool bLocked)
{
    if (IsValid(MovementComp))
    {
        MovementComp->SetMovementLocked(bLocked);
    }
}

// ── 내부 헬퍼 ────────────────────────────────────────────────────────────────

void ADualFirePlayerPawn::SetupInputMappingContext()
{
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!IsValid(PC))
    {
        return;
    }

    UEnhancedInputLocalPlayerSubsystem* Subsystem =
        ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer());
    if (!IsValid(Subsystem))
    {
        return;
    }

    // TSoftObjectPtr → 동기 로드 (BeginPlay 1회성, IMC 에셋은 경량이므로 허용)
    if (UInputMappingContext* IMC = IMC_Player.LoadSynchronous())
    {
        Subsystem->AddMappingContext(IMC, InputMappingPriority);
    }
    else
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ADualFirePlayerPawn: IMC_Player 에셋이 할당되지 않았습니다. "
                 "BP_DualFirePlayerPawn Details > Input > IMC_Player를 설정하세요."));
    }
}
