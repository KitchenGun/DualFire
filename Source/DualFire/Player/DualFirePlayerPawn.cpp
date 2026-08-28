// Copyright DualFire. All Rights Reserved.

#include "DualFirePlayerPawn.h"

#include "DualFireMovementComponent.h"
#include "Camera/StageCameraActor.h"
#include "Core/DualFireCollisionChannels.h"
#include "GameModes/DualFireGameModeBase.h"
#include "Weapon/Projectile/BaseProjectile.h"
#include "DualFire.h"

#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "PaperFlipbook.h"
#include "PaperFlipbookComponent.h"

#include "EnhancedInputComponent.h"
#include "InputMappingContext.h"
#include "InputAction.h"

ADualFirePlayerPawn::ADualFirePlayerPawn()
{
    // 평상시 Tick은 끄고 리스폰 진입 중에만 사용한다.
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;

    // 카메라를 StageCameraActor로 이관 — Pawn Possess 시 카메라 탐색 비활성화
    bFindCameraComponentWhenViewTarget = false;

    // ── SceneRoot ────────────────────────────────────────────────────────────
    // MovementComp의 UpdatedComponent 역할. 비주얼과 히트박스를 분리해 책임 명확화.
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    // ── AircraftVisual (Paper2D 비주얼 전담) ──────────────────────────────
    AircraftVisual = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("AircraftVisual"));
    AircraftVisual->SetupAttachment(SceneRoot);
    AircraftVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    AircraftVisual->SetRelativeRotation(FRotator(0.f, 90.f, -90.f));
    AircraftVisual->SetLooping(false);
    AircraftVisual->Stop();
    AircraftVisual->CastShadow = false;

    // ── HitboxComp (피격 감지 전담) ───────────────────────────────────────────
    // Profile="PlayerPawn": ObjectType=PlayerHitbox, EnemyBullet=Overlap, 나머지 Ignore
    HitboxComp = CreateDefaultSubobject<USphereComponent>(TEXT("HitboxComp"));
    HitboxComp->SetupAttachment(SceneRoot);
    HitboxComp->InitSphereRadius(12.f);
    HitboxComp->SetCollisionProfileName(DualFireProfile::PlayerPawn);

    // ── MovementComp ─────────────────────────────────────────────────────────
    // UpdatedComponent를 SceneRoot로 설정해 XY 평면 이동 대상을 루트로 지정
    MovementComp = CreateDefaultSubobject<UDualFireMovementComponent>(TEXT("MovementComp"));
    MovementComp->UpdatedComponent = SceneRoot;

    // ── WeaponComp ────────────────────────────────────────────────────────────
    WeaponComp = CreateDefaultSubobject<UWeaponComponent>(TEXT("WeaponComp"));

    // ── HealthComp ────────────────────────────────────────────────────────────
    HealthComp = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComp"));
    HealthComp->bUseLife = true;   // 플레이어는 잔여 기체/리스폰 사용
}

// ── APawn 오버라이드 ──────────────────────────────────────────────────────────

void ADualFirePlayerPawn::BeginPlay()
{
    Super::BeginPlay();

    // 최종 사망(잔여 기체 소진) → 미션 실패 연결
    if (IsValid(HealthComp))
    {
		HealthComp->OnDeath.AddDynamic(this, &ADualFirePlayerPawn::OnPlayerFinalDeath);
		HealthComp->OnRespawnRequested.AddDynamic(this, &ADualFirePlayerPawn::OnPlayerRespawnRequested);
		HealthComp->OnDamageReceived.AddDynamic(this, &ADualFirePlayerPawn::OnMissionDamageReceived);
    }
}

void ADualFirePlayerPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (LifeFlowState != ELifeFlowState::Entering)
	{
		SetActorTickEnabled(false);
		return;
	}

	RespawnEntryElapsed += DeltaSeconds;
	const float Alpha = FMath::Clamp(
		RespawnEntryElapsed / FMath::Max(RespawnEntryDuration, UE_SMALL_NUMBER), 0.0f, 1.0f);
	const float EasedAlpha = FMath::InterpEaseOut(0.0f, 1.0f, Alpha, 2.0f);
	const FVector EntryStart = RespawnAnchor - FVector(RespawnEntryOffset, 0.0f, 0.0f);
	SetActorLocation(FMath::Lerp(EntryStart, RespawnAnchor, EasedAlpha));

	if (Alpha >= 1.0f)
	{
		FinishRespawnEntry();
	}
}

void ADualFirePlayerPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    if (!IsValid(EIC))
    {
        UE_LOG(LogDualFire, Error,
            TEXT("ADualFirePlayerPawn: EnhancedInputComponent가 없습니다. "
                 "프로젝트 설정 > Input > Default Input Component Class를 확인하세요."));
        return;
    }

    // IA_Move: Triggered — 키를 누르는 동안 매 프레임 호출
    if (UInputAction* MoveIA = IA_Move.LoadSynchronous())
    {
        EIC->BindAction(MoveIA, ETriggerEvent::Triggered,
            this, &ADualFirePlayerPawn::OnMoveInput);
        EIC->BindAction(MoveIA, ETriggerEvent::Completed,
            this, &ADualFirePlayerPawn::OnMoveInputCompleted);
        EIC->BindAction(MoveIA, ETriggerEvent::Canceled,
            this, &ADualFirePlayerPawn::OnMoveInputCompleted);
    }
    else
    {
        UE_LOG(LogDualFire, Warning,
            TEXT("ADualFirePlayerPawn: IA_Move 에셋이 할당되지 않았습니다."));
    }

    // 발사 입력 — Triggered: 키를 누르는 동안 연사 (쿨다운으로 발사 간격 제어)
    if (UInputAction* PrimaryIA = IA_FirePrimary.LoadSynchronous())
    {
        EIC->BindAction(PrimaryIA, ETriggerEvent::Triggered,
            this, &ADualFirePlayerPawn::OnFirePrimaryInput);
    }
    else
    {
        UE_LOG(LogDualFire, Warning,
            TEXT("ADualFirePlayerPawn: IA_FirePrimary 에셋이 할당되지 않았습니다."));
    }

    if (UInputAction* Special1IA = IA_FireSpecial1.LoadSynchronous())
    {
        EIC->BindAction(Special1IA, ETriggerEvent::Triggered,
            this, &ADualFirePlayerPawn::OnFireSpecial1Input);
    }
    else
    {
        UE_LOG(LogDualFire, Warning,
            TEXT("ADualFirePlayerPawn: IA_FireSpecial1 에셋이 할당되지 않았습니다."));
    }

    if (UInputAction* Special2IA = IA_FireSpecial2.LoadSynchronous())
    {
        EIC->BindAction(Special2IA, ETriggerEvent::Triggered,
            this, &ADualFirePlayerPawn::OnFireSpecial2Input);
    }
    else
    {
        UE_LOG(LogDualFire, Warning,
            TEXT("ADualFirePlayerPawn: IA_FireSpecial2 에셋이 할당되지 않았습니다."));
    }
}

UPawnMovementComponent* ADualFirePlayerPawn::GetMovementComponent() const
{
    return MovementComp;
}

// ── Enhanced Input 핸들러 ─────────────────────────────────────────────────────

void ADualFirePlayerPawn::OnMoveInput(const FInputActionValue& Value)
{
	if (IsGameplayLocked())
	{
		return;
	}

    // IA_Move Axis2D: X=좌우 입력 → 월드 Y축, Y=앞뒤 입력 → 월드 X축
    const FVector2D Axis = Value.Get<FVector2D>();

    AddMovementInput(FVector::RightVector,   Axis.X);
    AddMovementInput(FVector::ForwardVector, Axis.Y);
    UpdateAircraftBankPose(Axis.X);
}

void ADualFirePlayerPawn::OnMoveInputCompleted(const FInputActionValue& Value)
{
	if (IsGameplayLocked())
	{
		return;
	}

    SetAircraftBankPose(EAircraftBankPose::Neutral);
}

void ADualFirePlayerPawn::ApplyAircraftVisual(UPaperFlipbook* InFlipbook)
{
    if (!IsValid(AircraftVisual))
    {
        UE_LOG(LogDualFire, Error, TEXT("[Player] AircraftVisual 컴포넌트가 없습니다."));
        return;
    }

    AircraftVisual->SetFlipbook(InFlipbook);
    AircraftVisual->SetLooping(false);
    AircraftVisual->Stop();

    if (!IsValid(InFlipbook))
    {
        UE_LOG(LogDualFire, Warning, TEXT("[Player] 기체 BankFlipbook이 설정되지 않았습니다."));
        return;
    }

    if (InFlipbook->GetNumFrames() != 7)
    {
        UE_LOG(LogDualFire, Warning,
            TEXT("[Player] BankFlipbook 프레임 수가 7이 아닙니다: %s (%d)"),
            *InFlipbook->GetName(), InFlipbook->GetNumFrames());
    }

    SetAircraftBankPose(EAircraftBankPose::Neutral);
}

void ADualFirePlayerPawn::SetAircraftBankPose(EAircraftBankPose Pose)
{
    if (!IsValid(AircraftVisual) || !IsValid(AircraftVisual->GetFlipbook()))
    {
        return;
    }

    const int32 LastFrameIndex = FMath::Max(0, AircraftVisual->GetFlipbookLengthInFrames() - 1);
    const int32 FrameIndex = FMath::Clamp(static_cast<int32>(Pose), 0, LastFrameIndex);
    AircraftVisual->SetPlaybackPositionInFrames(FrameIndex, false);
}

void ADualFirePlayerPawn::UpdateAircraftBankPose(float HorizontalInput)
{
    EAircraftBankPose Pose = EAircraftBankPose::Neutral;

    if (HorizontalInput >= 0.8f)
    {
        Pose = EAircraftBankPose::Right45;
    }
    else if (HorizontalInput >= 0.5f)
    {
        Pose = EAircraftBankPose::Right30;
    }
    else if (HorizontalInput >= 0.2f)
    {
        Pose = EAircraftBankPose::Right15;
    }
    else if (HorizontalInput <= -0.8f)
    {
        Pose = EAircraftBankPose::Left45;
    }
    else if (HorizontalInput <= -0.5f)
    {
        Pose = EAircraftBankPose::Left30;
    }
    else if (HorizontalInput <= -0.2f)
    {
        Pose = EAircraftBankPose::Left15;
    }

    SetAircraftBankPose(Pose);
}

// ── 발사 핸들러 ───────────────────────────────────────────────────────────────

void ADualFirePlayerPawn::OnFirePrimaryInput(const FInputActionValue& Value)
{
	if (!IsGameplayLocked() && IsValid(WeaponComp))
    {
        WeaponComp->FirePrimary();
    }
}

void ADualFirePlayerPawn::OnFireSpecial1Input(const FInputActionValue& Value)
{
	if (!IsGameplayLocked() && IsValid(WeaponComp))
    {
        WeaponComp->FireSpecial1();
    }
}

void ADualFirePlayerPawn::OnFireSpecial2Input(const FInputActionValue& Value)
{
	if (!IsGameplayLocked() && IsValid(WeaponComp))
    {
        WeaponComp->FireSpecial2();
    }
}

// ── 디버그 콘솔 명령 ─────────────────────────────────────────────────────────

void ADualFirePlayerPawn::DF_Damage(int32 Amount)
{
    if (IsValid(HealthComp))
    {
        UE_LOG(LogDualFire, Log, TEXT("[Debug] DF_Damage %d 호출"), Amount);
        HealthComp->ApplyDamage(Amount);
    }
}

void ADualFirePlayerPawn::DF_RecoverHealth()
{
    if (IsValid(HealthComp))
    {
        HealthComp->RecoverHealth(HealthComp->MaxHealth);
        UE_LOG(LogDualFire, Log, TEXT("[Debug] DF_RecoverHealth — HP 만회"));
    }
}

void ADualFirePlayerPawn::DF_RecoverShield()
{
    if (IsValid(HealthComp))
    {
        HealthComp->FullRecoverShield();
        UE_LOG(LogDualFire, Log, TEXT("[Debug] DF_RecoverShield — Shield 만회"));
    }
}

void ADualFirePlayerPawn::DF_Kill()
{
    if (IsValid(HealthComp))
    {
        UE_LOG(LogDualFire, Log, TEXT("[Debug] DF_Kill — 즉사 데미지"));
        // 무적/보호막을 무시하고 HP를 직접 0으로: 무적 해제 + 보호막 제거 후 대형 데미지
		HealthComp->ClearAllInvincibility();
        HealthComp->CurrentShield = 0;
        HealthComp->ApplyDamage(9999);
    }
}

// ── 사망 처리 ────────────────────────────────────────────────────────────────

void ADualFirePlayerPawn::OnPlayerFinalDeath()
{
	++MissionDeathCount;
	EnterDeathState(true);
    UE_LOG(LogDualFire, Warning, TEXT("[Player] 최종 사망 → 미션 실패 요청"));

    if (ADualFireGameModeBase* GameMode =
        Cast<ADualFireGameModeBase>(UGameplayStatics::GetGameMode(this)))
    {
        GameMode->OnMissionFail(EDualFireMissionFailureReason::PlayerDestroyed);
    }
}

void ADualFirePlayerPawn::OnPlayerRespawnRequested()
{
	++MissionDeathCount;
	EnterDeathState(false);
}

void ADualFirePlayerPawn::EnterDeathState(bool bFinalDeath)
{
	LifeFlowState = bFinalDeath ? ELifeFlowState::FinalDead : ELifeFlowState::DeathDelay;
	SetGameplayLocked(true);
	SetAircraftBankPose(EAircraftBankPose::Neutral);

	if (IsValid(AircraftVisual))
	{
		AircraftVisual->SetHiddenInGame(true);
	}

	UE_LOG(LogDualFire, Log, TEXT("[Player] 사망 상태 진입 — Final:%s"), bFinalDeath ? TEXT("true") : TEXT("false"));
	if (!bFinalDeath)
	{
		GetWorldTimerManager().SetTimer(
			RespawnDelayHandle,
			this,
			&ADualFirePlayerPawn::BeginRespawnEntry,
			DeathDelay,
			false);
	}
}

void ADualFirePlayerPawn::BeginRespawnEntry()
{
	if (LifeFlowState != ELifeFlowState::DeathDelay)
	{
		return;
	}

	LifeFlowState = ELifeFlowState::Entering;
	RespawnEntryElapsed = 0.0f;
	RespawnAnchor = ResolveRespawnAnchor();
	SetActorLocation(RespawnAnchor - FVector(RespawnEntryOffset, 0.0f, 0.0f));

	if (IsValid(AircraftVisual))
	{
		AircraftVisual->SetHiddenInGame(false);
	}

	SetActorTickEnabled(true);
	UE_LOG(LogDualFire, Log, TEXT("[Player] 리스폰 진입 시작 — Duration:%.2f"), RespawnEntryDuration);
}

void ADualFirePlayerPawn::FinishRespawnEntry()
{
	SetActorLocation(RespawnAnchor);
	SetActorTickEnabled(false);

	if (IsValid(HealthComp))
	{
		HealthComp->CompleteRespawn();
	}
	if (IsValid(WeaponComp))
	{
		WeaponComp->ResetCooldowns();
	}
	ClearActiveEnemyProjectiles();

	LifeFlowState = ELifeFlowState::Alive;
	SetGameplayLocked(false);

	UE_LOG(LogDualFire, Log, TEXT("[Player] 리스폰 완료 — 입력/충돌 복구"));
}

void ADualFirePlayerPawn::SetGameplayLocked(bool bLocked)
{
	if (IsValid(HitboxComp))
	{
		HitboxComp->SetCollisionEnabled(bLocked ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryOnly);
	}

	if (IsValid(MovementComp))
	{
		MovementComp->StopMovementImmediately();
		MovementComp->ConsumeInputVector();
		if (bLocked)
		{
			MovementComp->Deactivate();
		}
		else
		{
			MovementComp->Activate(true);
		}
	}

}

FVector ADualFirePlayerPawn::ResolveRespawnAnchor() const
{
	if (const UWorld* World = GetWorld())
	{
		if (const ADualFireGameModeBase* GameMode = World->GetAuthGameMode<ADualFireGameModeBase>())
		{
			if (const AStageCameraActor* Camera = GameMode->GetStageCamera())
			{
				const FBox2D Bounds = Camera->GetPlayableBounds();
				return FVector(Bounds.Min.X, Bounds.GetCenter().Y, GetActorLocation().Z);
			}
		}
	}

	UE_LOG(LogDualFire, Warning, TEXT("[Player] 리스폰 앵커 계산 실패 — 현재 위치 사용"));
	return GetActorLocation();
}

void ADualFirePlayerPawn::ClearActiveEnemyProjectiles()
{
	TArray<TWeakObjectPtr<ABaseProjectile>> Projectiles;
	for (TActorIterator<ABaseProjectile> It(GetWorld()); It; ++It)
	{
		Projectiles.Add(*It);
	}

	int32 ClearedCount = 0;
	for (const TWeakObjectPtr<ABaseProjectile>& Projectile : Projectiles)
	{
		if (Projectile.IsValid() && Projectile->ClearForPlayerRespawn())
		{
			++ClearedCount;
		}
	}

	UE_LOG(LogDualFire, Log, TEXT("[Player] 리스폰 적탄 소거 — Count:%d"), ClearedCount);
}

void ADualFirePlayerPawn::OnMissionDamageReceived()
{
	++MissionHitCount;
}

UInputMappingContext* ADualFirePlayerPawn::ResolveInputMappingContext() const
{
    UInputMappingContext* MappingContext = IMC_Player.LoadSynchronous();
    if (!IsValid(MappingContext))
    {
        UE_LOG(LogDualFire, Warning,
            TEXT("ADualFirePlayerPawn: IMC_Player 에셋이 할당되지 않았습니다. "
                 "BP_PlayerPawn Details > Input > IMC_Player를 설정하세요."));
    }

    return MappingContext;
}
