// Copyright DualFire. All Rights Reserved.

#include "Stage/StageController.h"
#include "Stage/DualFirePrototypeBossCube.h"
#include "Core/ActorPoolSubsystem.h"
#include "Enemy/EnemyBase.h"
#include "Enemy/EnemyAIComponent.h"
#include "Camera/StageCameraActor.h"
#include "GameModes/DualFireGameModeBase.h"
#include "Weapon/Projectile/BaseProjectile.h"
#include "DualFire.h"

#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	// 모든 적 이동 패턴(Linear/EnterStop)이 월드 -X 방향으로 진행한다.
	// 액터 기본 전방(+X)과 반대이므로 스폰 시 Yaw 180도를 줘서 모델 전면이 진행 방향을 보게 한다.
	const FRotator EnemyFacingRotation(0.f, 180.f, 0.f);
}

AStageController::AStageController()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false; // BeginPlay에서 활성화
	PrototypeBossClass = ADualFirePrototypeBossCube::StaticClass();
}

void AStageController::BeginPlay()
{
	Super::BeginPlay();

	CachePlayerPawn(UGameplayStatics::GetPlayerPawn(this, 0));
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PC->OnPossessedPawnChanged.AddDynamic(this, &AStageController::HandlePossessedPawnChanged);
	}

	PrewarmPools();
	BuildActiveWaves();
	SetState(EStageState::Timeline);

	SetActorTickEnabled(true);
}

void AStageController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PC->OnPossessedPawnChanged.RemoveDynamic(this, &AStageController::HandlePossessedPawnChanged);
	}

	for (FTimerHandle& Handle : SequenceSpawnTimerHandles)
	{
		GetWorld()->GetTimerManager().ClearTimer(Handle);
	}
	SequenceSpawnTimerHandles.Reset();
	GetWorld()->GetTimerManager().ClearTimer(PrototypeBossArrivalTimeoutHandle);

	if (IsValid(ActivePrototypeBoss))
	{
		ActivePrototypeBoss->OnDestinationReached.RemoveAll(this);
		ActivePrototypeBoss->Destroy();
		ActivePrototypeBoss = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void AStageController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (CurrentState == EStageState::Timeline)
	{
		TickTimeline(DeltaTime);
		TickEnemyAI(DeltaTime);
	}
	else if (CurrentState == EStageState::EliteCombat)
	{
		ElapsedTime += DeltaTime;
	}
}

// ── 내부 초기화 ───────────────────────────────────────────────────────────────

void AStageController::BuildActiveWaves()
{
	ActiveWaves.Reset();
	NextWaveIndex = 0;

	if (bUseTestWaves)
	{
		ActiveWaves = TestWaves;
	}
	else if (IsValid(WaveDataTable))
	{
		TArray<FWaveRow*> Rows;
		WaveDataTable->GetAllRows<FWaveRow>(TEXT("StageController.BuildActiveWaves"), Rows);
		for (const FWaveRow* Row : Rows)
		{
			if (Row && (StageID.IsNone() || Row->StageID == StageID))
			{
				ActiveWaves.Add(*Row);
			}
		}
	}
	else
	{
		UE_LOG(LogDualFire, Warning, TEXT("[Stage] WaveDataTable 없음 — 웨이브 없음"));
	}

	// TriggerTime 오름차순 정렬
	ActiveWaves.Sort([](const FWaveRow& A, const FWaveRow& B)
	{
		return A.TriggerTime < B.TriggerTime;
	});

	UE_LOG(LogDualFire, Log, TEXT("[Stage] 웨이브 %d개 로드"), ActiveWaves.Num());
}

// ── Timeline 단계 ─────────────────────────────────────────────────────────────

void AStageController::TickTimeline(float DeltaTime)
{
	ElapsedTime += DeltaTime;

	// 웨이브 트리거
	while (NextWaveIndex < ActiveWaves.Num() &&
		   ActiveWaves[NextWaveIndex].TriggerTime <= ElapsedTime)
	{
		TriggerWave(ActiveWaves[NextWaveIndex]);
		++NextWaveIndex;
	}

	// 엘리트 트리거
	if (ElapsedTime >= EliteTriggerTime)
	{
		SetState(EStageState::EliteCombat);
	}
}

void AStageController::TriggerWave(const FWaveRow& Wave)
{
	UE_LOG(LogDualFire, Log, TEXT("[Stage] 웨이브 트리거 — %s (Count=%d)"),
		*Wave.WaveID.ToString(), Wave.Count);

	SpawnWaveSequential(Wave, 0);
}

void AStageController::SpawnWaveSequential(FWaveRow Wave, int32 AlreadySpawned)
{
	if (AlreadySpawned >= Wave.Count)
	{
		return;
	}

	const TSubclassOf<AEnemyBase> EnemyClass = ResolveEnemyClass(Wave.EnemyID);
	if (!IsValid(EnemyClass))
	{
		UE_LOG(LogDualFire, Warning, TEXT("[Stage] EnemyID '%s'에 대한 클래스 없음 — 스폰 생략"),
			*Wave.EnemyID.ToString());
		return;
	}

	const FVector SpawnLoc = ResolveSpawnAnchor(Wave.SpawnAnchor, Wave.SpawnOffset);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AEnemyBase* Enemy = nullptr;
	if (UActorPoolSubsystem* Pool = GetWorld()->GetSubsystem<UActorPoolSubsystem>())
	{
		Enemy = Cast<AEnemyBase>(
			Pool->AcquireActor(EnemyClass, FTransform(EnemyFacingRotation, SpawnLoc)));
	}
	else
	{
		Enemy = GetWorld()->SpawnActor<AEnemyBase>(EnemyClass, SpawnLoc, EnemyFacingRotation, Params);
	}

	if (IsValid(Enemy))
	{
		FEnemyRow EnemyRow;
		if (FindEnemyRow(Wave.EnemyID, EnemyRow))
		{
			Enemy->InitFromEnemyRow(EnemyRow);
		}
		else
		{
			UE_LOG(LogDualFire, Warning, TEXT("[Stage] EnemyID '%s' 데이터 없음 — BP 기본값 사용"),
				*Wave.EnemyID.ToString());
		}

		if (UEnemyAIComponent* AI = Enemy->GetAIComponent())
		{
			if (!IsValid(AI->ProjectileClass) && IsValid(EnemyProjectileClass))
			{
				AI->ProjectileClass = EnemyProjectileClass;
			}
			RegisterEnemyAI(AI);
		}

		RecordEnemySpawned(Enemy->GetEnemyAttributes_Implementation());
	}

	const int32 NextCount = AlreadySpawned + 1;
	if (NextCount < Wave.Count && Wave.SpawnInterval > 0.0f)
	{
		FTimerHandle SeqHandle;
		FTimerDelegate Del;
		Del.BindUObject(this, &AStageController::SpawnWaveSequential, Wave, NextCount);
		GetWorld()->GetTimerManager().SetTimer(SeqHandle, Del, Wave.SpawnInterval, false);
		SequenceSpawnTimerHandles.Add(SeqHandle);
	}
	else if (NextCount < Wave.Count)
	{
		// SpawnInterval == 0이면 즉시 재귀
		SpawnWaveSequential(Wave, NextCount);
	}
}

// ── 상태 전환 ─────────────────────────────────────────────────────────────────

void AStageController::SetState(EStageState NewState)
{
	if (CurrentState == NewState)
	{
		return;
	}

	CurrentState = NewState;
	OnStageStateChanged.Broadcast(NewState);

	UE_LOG(LogDualFire, Log, TEXT("[Stage] 상태 전환 → %s"),
		NewState == EStageState::Timeline    ? TEXT("Timeline") :
		NewState == EStageState::EliteCombat ? TEXT("EliteCombat") :
		                                       TEXT("Ended"));

	switch (NewState)
	{
	case EStageState::EliteCombat:
		BeginPrototypeBossSequence();
		break;

	case EStageState::Ended:
		SetActorTickEnabled(false);
		GetWorld()->GetTimerManager().ClearTimer(EliteTimeLimitHandle);
		GetWorld()->GetTimerManager().ClearTimer(PrototypeBossArrivalTimeoutHandle);
		for (FTimerHandle& Handle : SequenceSpawnTimerHandles)
		{
			GetWorld()->GetTimerManager().ClearTimer(Handle);
		}
		SequenceSpawnTimerHandles.Reset();
		// 결과별 GameMode 호출은 OnEliteDeath / OnEliteTimeLimitExpired에서 수행
		break;

	default:
		break;
	}
}

void AStageController::BeginPrototypeBossSequence()
{
	StopCombatForPrototypeBoss();

	AStageCameraActor* Camera = GetStageCamera();
	if (!IsValid(Camera) || !IsValid(PrototypeBossClass))
	{
		UE_LOG(LogDualFire, Error, TEXT("[Stage] 프로토타입 보스 생성 조건이 유효하지 않음"));
		HandlePrototypeBossArrivalTimeout();
		return;
	}

	const FBox2D Bounds = Camera->GetPlayableBounds();
	const float CenterY = FMath::Lerp(Bounds.Min.Y, Bounds.Max.Y, 0.5f);
	const FVector StartLocation(Bounds.Max.X + PrototypeBossSpawnOffset, CenterY, 0.0f);
	const FVector Destination(
		FMath::Lerp(Bounds.Max.X, Bounds.Min.X, PrototypeBossDestinationRatio),
		CenterY,
		0.0f);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ActivePrototypeBoss = GetWorld()->SpawnActor<ADualFirePrototypeBossCube>(
		PrototypeBossClass, StartLocation, FRotator::ZeroRotator, Params);

	if (!IsValid(ActivePrototypeBoss))
	{
		UE_LOG(LogDualFire, Error, TEXT("[Stage] 프로토타입 보스 Cube 생성 실패"));
		HandlePrototypeBossArrivalTimeout();
		return;
	}

	ActivePrototypeBoss->OnDestinationReached.AddUObject(
		this, &AStageController::HandlePrototypeBossDestinationReached);
	ActivePrototypeBoss->StartDescent(
		StartLocation, Destination, PrototypeBossDescentDuration);

	GetWorld()->GetTimerManager().SetTimer(
		PrototypeBossArrivalTimeoutHandle,
		this,
		&AStageController::HandlePrototypeBossArrivalTimeout,
		PrototypeBossArrivalTimeout,
		false);

	UE_LOG(LogDualFire, Log, TEXT("[Stage] 프로토타입 보스 Cube 등장 시작"));
}

void AStageController::StopCombatForPrototypeBoss()
{
	if (AStageCameraActor* Camera = GetStageCamera())
	{
		Camera->SetPaused(true);
	}

	for (FTimerHandle& Handle : SequenceSpawnTimerHandles)
	{
		GetWorld()->GetTimerManager().ClearTimer(Handle);
	}
	SequenceSpawnTimerHandles.Reset();

	for (UEnemyAIComponent* AI : ActiveEnemyAIComponents)
	{
		if (IsValid(AI))
		{
			AI->StopAttackTimer();
		}
	}
}

void AStageController::HandlePrototypeBossDestinationReached()
{
	if (CurrentState != EStageState::EliteCombat)
	{
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(PrototypeBossArrivalTimeoutHandle);
	UE_LOG(LogDualFire, Log, TEXT("[Stage] 프로토타입 보스 Cube 도착 -> 미션 클리어"));

	SetState(EStageState::Ended);
	if (ADualFireGameModeBase* GM = Cast<ADualFireGameModeBase>(UGameplayStatics::GetGameMode(this)))
	{
		GM->OnMissionClear();
	}
}

void AStageController::HandlePrototypeBossArrivalTimeout()
{
	if (CurrentState != EStageState::EliteCombat)
	{
		return;
	}

	UE_LOG(LogDualFire, Error, TEXT("[Stage] 프로토타입 보스 Cube 도착 실패 -> 미션 실패"));
	SetState(EStageState::Ended);
	if (ADualFireGameModeBase* GM = Cast<ADualFireGameModeBase>(UGameplayStatics::GetGameMode(this)))
	{
		GM->OnMissionFail();
	}
}

void AStageController::OnEliteDeath()
{
	if (CurrentState == EStageState::EliteCombat)
	{
		UE_LOG(LogDualFire, Log, TEXT("[Stage] 엘리트 격파 → 미션 클리어"));
		SetState(EStageState::Ended);

		if (ADualFireGameModeBase* GM = Cast<ADualFireGameModeBase>(UGameplayStatics::GetGameMode(this)))
		{
			GM->OnMissionClear();
		}
	}
}

void AStageController::OnEliteTimeLimitExpired()
{
	if (CurrentState == EStageState::EliteCombat)
	{
		UE_LOG(LogDualFire, Warning, TEXT("[Stage] 엘리트 제한 시간 초과 → 미션 실패"));
		SetState(EStageState::Ended);

		if (ADualFireGameModeBase* GM = Cast<ADualFireGameModeBase>(UGameplayStatics::GetGameMode(this)))
		{
			GM->OnMissionFail();
		}
	}
}

// ── 유틸리티 ─────────────────────────────────────────────────────────────────

TSubclassOf<AEnemyBase> AStageController::ResolveEnemyClass(FName EnemyID) const
{
	if (const TSubclassOf<AEnemyBase>* Found = EnemyClassMap.Find(EnemyID))
	{
		if (IsValid(*Found))
		{
			return *Found;
		}
	}

	return DefaultEnemyClass;
}

FVector AStageController::ResolveSpawnAnchor(ESpawnAnchor Anchor, const FVector& Offset) const
{
	AStageCameraActor* Cam = GetStageCamera();
	if (!IsValid(Cam))
	{
		// 카메라 없으면 Actor 위치 기반 폴백
		return GetActorLocation() + Offset;
	}

	const FBox2D Bounds = Cam->GetPlayableBounds();

	// X: 화면 위쪽 바깥에서 스폰 (+X = 진행 방향)
	const float SpawnX = Bounds.Max.X + SpawnMarginX;

	// Y: 앵커 열에 따라 좌/중/우
	float YRatio = 0.5f;
	switch (Anchor)
	{
	case ESpawnAnchor::TopLeft:
	case ESpawnAnchor::Left:
	case ESpawnAnchor::BottomLeft:
		YRatio = 0.0f;
		break;
	case ESpawnAnchor::TopCenter:
	case ESpawnAnchor::Center:
	case ESpawnAnchor::BottomCenter:
		YRatio = 0.5f;
		break;
	case ESpawnAnchor::TopRight:
	case ESpawnAnchor::Right:
	case ESpawnAnchor::BottomRight:
		YRatio = 1.0f;
		break;
	}
	const float SpawnY = FMath::Lerp(Bounds.Min.Y, Bounds.Max.Y, YRatio);

	return FVector(SpawnX, SpawnY, 0.0f) + Offset;
}

AStageCameraActor* AStageController::GetStageCamera() const
{
	if (ADualFireGameModeBase* GM = Cast<ADualFireGameModeBase>(UGameplayStatics::GetGameMode(this)))
	{
		return GM->GetStageCamera();
	}
	return nullptr;
}

void AStageController::RegisterEnemyAI(UEnemyAIComponent* AIComponent)
{
	if (IsValid(AIComponent))
	{
		ActiveEnemyAIComponents.AddUnique(AIComponent);
	}
}

void AStageController::UnregisterEnemyAI(UEnemyAIComponent* AIComponent)
{
	ActiveEnemyAIComponents.RemoveSingleSwap(AIComponent);
}

void AStageController::RecordEnemySpawned(const FEnemyAttribute& Attribute)
{
	AirEnemiesSpawned += Attribute.HasAir() ? 1 : 0;
	GroundEnemiesSpawned += Attribute.HasGround() ? 1 : 0;
}

void AStageController::NotifyEnemyDefeated(const FEnemyAttribute& Attribute)
{
	AirEnemiesDefeated += Attribute.HasAir() ? 1 : 0;
	GroundEnemiesDefeated += Attribute.HasGround() ? 1 : 0;
}

void AStageController::PrewarmPools()
{
	UActorPoolSubsystem* Pool = GetWorld() ? GetWorld()->GetSubsystem<UActorPoolSubsystem>() : nullptr;
	if (!IsValid(Pool))
	{
		return;
	}

	for (const TPair<FName, TSubclassOf<AEnemyBase>>& Pair : EnemyClassMap)
	{
		if (IsValid(Pair.Value))
		{
			Pool->Prewarm(Pair.Value, EnemyPrewarmCountPerClass);
		}
	}

	if (IsValid(EnemyProjectileClass))
	{
		Pool->Prewarm(EnemyProjectileClass, EnemyProjectilePrewarmCount);
	}

	for (TSubclassOf<AActor> ProjectileClass : PlayerProjectilePrewarmClasses)
	{
		if (IsValid(ProjectileClass))
		{
			Pool->Prewarm(ProjectileClass, PlayerProjectilePrewarmCountPerClass);
		}
	}
}

bool AStageController::FindEnemyRow(FName EnemyID, FEnemyRow& OutEnemyRow) const
{
	if (!IsValid(EnemyDataTable) || EnemyID.IsNone())
	{
		return false;
	}

	if (const FEnemyRow* Row = EnemyDataTable->FindRow<FEnemyRow>(
		EnemyID, TEXT("StageController.FindEnemyRow"), false))
	{
		OutEnemyRow = *Row;
		return true;
	}

	TArray<FEnemyRow*> Rows;
	EnemyDataTable->GetAllRows<FEnemyRow>(TEXT("StageController.FindEnemyRow"), Rows);
	for (const FEnemyRow* Row : Rows)
	{
		if (Row && Row->EnemyID == EnemyID)
		{
			OutEnemyRow = *Row;
			return true;
		}
	}

	return false;
}

void AStageController::TickEnemyAI(float DeltaTime)
{
	AStageCameraActor* Camera = GetStageCamera();
	if (!IsValid(Camera))
	{
		return;
	}

	const FBox2D Bounds = Camera->GetPlayableBounds();
	const FVector PlayerLocation = GetCachedPlayerLocation();
	const bool bPlayerLocationValid = bHasCachedPlayerLocation;

	for (int32 Index = ActiveEnemyAIComponents.Num() - 1; Index >= 0; --Index)
	{
		UEnemyAIComponent* AI = ActiveEnemyAIComponents[Index];
		if (!IsValid(AI) || !IsValid(AI->GetOwner()) || AI->GetOwner()->IsHidden())
		{
			ActiveEnemyAIComponents.RemoveAtSwap(Index);
			continue;
		}

		AI->UpdateAI(DeltaTime, Bounds, PlayerLocation, bPlayerLocationValid);
	}
}

FVector AStageController::GetCachedPlayerLocation()
{
	if (!CachedPlayerPawn.IsValid())
	{
		CachePlayerPawn(UGameplayStatics::GetPlayerPawn(this, 0));
	}

	if (CachedPlayerPawn.IsValid())
	{
		CachedPlayerLocation = CachedPlayerPawn->GetActorLocation();
		bHasCachedPlayerLocation = true;
	}

	return bHasCachedPlayerLocation ? CachedPlayerLocation : FVector::ZeroVector;
}

void AStageController::CachePlayerPawn(APawn* NewPawn)
{
	CachedPlayerPawn = NewPawn;
	if (CachedPlayerPawn.IsValid())
	{
		CachedPlayerLocation = CachedPlayerPawn->GetActorLocation();
		bHasCachedPlayerLocation = true;
	}
}

void AStageController::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	CachePlayerPawn(NewPawn);
}
