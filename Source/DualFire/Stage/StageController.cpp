// Copyright DualFire. All Rights Reserved.

#include "Stage/StageController.h"
#include "Stage/DualFirePrototypeBossCube.h"
#include "Core/ActorPoolSubsystem.h"
#include "Enemy/EnemyBase.h"
#include "Enemy/EnemyAIComponent.h"
#include "Camera/StageCameraActor.h"
#include "GameModes/DualFireGameModeBase.h"
#include "GameInstance/DualFireGameInstance.h"
#include "Weapon/Projectile/BaseProjectile.h"
#include "DualFire.h"

#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "Curves/CurveFloat.h"
#include "EngineUtils.h"
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

bool AStageController::ConfigureStage(const FName InStageID, FText& OutError)
{
	OutError = FText::GetEmpty();
	FName InvalidField = NAME_None;
	const UDualFireGameInstance* GI = Cast<UDualFireGameInstance>(GetGameInstance());
	const UDataTable* StageTable = IsValid(GI) ? GI->GetStageDataTable() : nullptr;
	if (!IsValid(StageTable))
	{
		OutError = FText::FromString(TEXT("Stage DataTable이 지정되지 않았습니다."));
		LogConfigurationError(InStageID, TEXT("StageDataTable"), OutError);
		return false;
	}

	const FStageRow* StageRow = StageTable->FindRow<FStageRow>(
		InStageID, TEXT("StageController.ConfigureStage"), false);
	if (!StageRow)
	{
		OutError = FText::FromString(FString::Printf(
			TEXT("StageID '%s' 행을 찾을 수 없습니다."), *InStageID.ToString()));
		LogConfigurationError(InStageID, TEXT("StageID"), OutError);
		return false;
	}

	TArray<FWaveRow> ValidatedWaves;
	if (!BuildValidatedWaves(InStageID, ValidatedWaves, OutError, InvalidField) ||
		!ValidateStageRow(InStageID, *StageRow, ValidatedWaves, OutError, InvalidField))
	{
		LogConfigurationError(InStageID, InvalidField, OutError);
		return false;
	}

	StageID = InStageID;
	ActiveStageRow = *StageRow;
	ActiveWaves = MoveTemp(ValidatedWaves);
	ActivePauseTriggers = StageRow->PauseTriggers;
	ActivePauseTriggers.Sort([](const FStagePauseTrigger& A, const FStagePauseTrigger& B)
	{
		return A.TriggerTime < B.TriggerTime;
	});
	NextWaveIndex = 0;
	NextPauseTriggerIndex = 0;
	ElapsedTime = 0.0f;
	bConfigured = true;
	return true;
}

void AStageController::BeginPlay()
{
	Super::BeginPlay();
	if (!bConfigured)
	{
		const FText Error = FText::FromString(TEXT("ConfigureStage가 BeginPlay 전에 성공하지 않았습니다."));
		LogConfigurationError(StageID, TEXT("ConfigureStage"), Error);
		SetActorTickEnabled(false);
		return;
	}

	CachePlayerPawn(UGameplayStatics::GetPlayerPawn(this, 0));
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PC->OnPossessedPawnChanged.AddDynamic(this, &AStageController::HandlePossessedPawnChanged);
	}

	RegisterPlacedEnemies();
	PrewarmPools();
	SetState(EStageState::Timeline);
	UpdateCameraScrollSpeed();
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

	ActiveEnemies.Reset();
	PauseScopedEnemies.Reset();
	if (AStageCameraActor* Camera = GetStageCamera())
	{
		Camera->SetPaused(false);
	}

	Super::EndPlay(EndPlayReason);
}

void AStageController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (CurrentState == EStageState::Timeline)
	{
		TickTimeline(DeltaTime);
		if (CurrentState == EStageState::Timeline)
		{
			TickEnemyAI(DeltaTime);
		}
	}
	else if (CurrentState == EStageState::EliteCombat)
	{
		ElapsedTime += DeltaTime;
	}
}

bool AStageController::SetEliteTriggerTimeForPIE(float InTriggerTime)
{
#if WITH_EDITOR
	if (GetWorld() && GetWorld()->WorldType == EWorldType::PIE &&
		CurrentState == EStageState::Timeline)
	{
		EliteTriggerTime = FMath::Max(InTriggerTime, 0.1f);
		return true;
	}
#endif

	return false;
}

bool AStageController::BuildValidatedWaves(
	const FName InStageID,
	TArray<FWaveRow>& OutWaves,
	FText& OutError,
	FName& OutField) const
{
	OutWaves.Reset();
	if (!IsValid(WaveDataTable))
	{
		OutField = TEXT("WaveDataTable");
		OutError = FText::FromString(TEXT("Wave DataTable이 지정되지 않았습니다."));
		return false;
	}
	if (!IsValid(EnemyDataTable))
	{
		OutField = TEXT("EnemyDataTable");
		OutError = FText::FromString(TEXT("Enemy DataTable이 지정되지 않았습니다."));
		return false;
	}

	TArray<FWaveRow*> Rows;
	WaveDataTable->GetAllRows<FWaveRow>(TEXT("StageController.BuildValidatedWaves"), Rows);
	for (const FWaveRow* Row : Rows)
	{
		if (!Row || Row->StageID != InStageID)
		{
			continue;
		}
		if (Row->WaveID.IsNone() || Row->EnemyID.IsNone() || Row->TriggerTime < 0.0f ||
			Row->Count < 1 || Row->SpawnInterval < 0.0f)
		{
			OutField = TEXT("WaveRow");
			OutError = FText::FromString(FString::Printf(
				TEXT("WaveID '%s'의 필수 값이 유효하지 않습니다."), *Row->WaveID.ToString()));
			return false;
		}
		FEnemyRow EnemyRow;
		if (!FindEnemyRow(Row->EnemyID, EnemyRow))
		{
			OutField = TEXT("EnemyID");
			OutError = FText::FromString(FString::Printf(
				TEXT("WaveID '%s'가 존재하지 않는 EnemyID '%s'를 참조합니다."),
				*Row->WaveID.ToString(), *Row->EnemyID.ToString()));
			return false;
		}
		OutWaves.Add(*Row);
	}

	OutWaves.Sort([](const FWaveRow& A, const FWaveRow& B)
	{
		return A.TriggerTime < B.TriggerTime;
	});
	return true;
}

bool AStageController::ValidateStageRow(
	const FName InStageID,
	const FStageRow& Row,
	const TArray<FWaveRow>& Waves,
	FText& OutError,
	FName& OutField) const
{
	if (Row.StageID.IsNone() || Row.StageID != InStageID)
	{
		OutField = TEXT("StageID");
		OutError = FText::FromString(TEXT("StageRow의 StageID가 요청 ID와 일치하지 않습니다."));
		return false;
	}
	if (Row.MinScrollSpeed < 0.0f || Row.MaxScrollSpeed <= 0.0f ||
		Row.MaxScrollSpeed < Row.MinScrollSpeed)
	{
		OutField = TEXT("ScrollSpeedRange");
		OutError = FText::FromString(TEXT("최소·최대 스크롤 속도 범위가 유효하지 않습니다."));
		return false;
	}
	if (!IsValid(Row.NormalizedScrollCurve) || Row.NormalizedScrollCurve->FloatCurve.GetNumKeys() == 0)
	{
		OutField = TEXT("NormalizedScrollCurve");
		OutError = FText::FromString(TEXT("정규화 Scroll Curve가 지정되지 않았거나 키가 없습니다."));
		return false;
	}
	for (auto It = Row.NormalizedScrollCurve->FloatCurve.GetKeyIterator(); It; ++It)
	{
		const FRichCurveKey& Key = *It;
		if (Key.Value < 0.0f || Key.Value > 1.0f)
		{
			OutField = TEXT("NormalizedScrollCurve");
			OutError = FText::FromString(FString::Printf(
				TEXT("Curve 키 %.3f초 값 %.3f가 0~1 범위를 벗어났습니다."), Key.Time, Key.Value));
			return false;
		}
	}

	TSet<int64> TriggerMilliseconds;
	for (const FStagePauseTrigger& Trigger : Row.PauseTriggers)
	{
		const int64 TriggerKey = FMath::RoundToInt64(Trigger.TriggerTime * 1000.0f);
		if (Trigger.TriggerTime < 0.0f || TriggerMilliseconds.Contains(TriggerKey))
		{
			OutField = TEXT("PauseTriggers");
			OutError = FText::FromString(FString::Printf(
				TEXT("멈춤 트리거 %.3f초가 음수이거나 중복입니다."), Trigger.TriggerTime));
			return false;
		}
		TriggerMilliseconds.Add(TriggerKey);

		if (Trigger.ResumeCondition == EStagePauseResumeCondition::RealTime)
		{
			if (Trigger.ResumeDelay <= 0.0f || !Trigger.TargetEnemyID.IsNone())
			{
				OutField = TEXT("ResumeDelay");
				OutError = FText::FromString(TEXT("실제 시간 재개 조건은 양수 ResumeDelay만 사용해야 합니다."));
				return false;
			}
		}
		else if (Trigger.ResumeCondition == EStagePauseResumeCondition::WaveDefeated)
		{
			const bool bHasWaveAtTrigger = Waves.ContainsByPredicate([&Trigger](const FWaveRow& Wave)
			{
				return FMath::IsNearlyEqual(Wave.TriggerTime, Trigger.TriggerTime, KINDA_SMALL_NUMBER);
			});
			if (!bHasWaveAtTrigger || Trigger.ResumeDelay > 0.0f || !Trigger.TargetEnemyID.IsNone())
			{
				OutField = TEXT("ResumeCondition");
				OutError = FText::FromString(TEXT("웨이브 전멸 조건에는 같은 시각 웨이브가 하나 이상 필요합니다."));
				return false;
			}
		}
		else if (Trigger.ResumeCondition == EStagePauseResumeCondition::EnemyDefeated)
		{
			FEnemyRow TargetRow;
			if (Trigger.TargetEnemyID.IsNone() || Trigger.ResumeDelay > 0.0f ||
				!FindEnemyRow(Trigger.TargetEnemyID, TargetRow))
			{
				OutField = TEXT("TargetEnemyID");
				OutError = FText::FromString(FString::Printf(
					TEXT("멈춤 트리거가 존재하지 않는 EnemyID '%s'를 참조합니다."),
					*Trigger.TargetEnemyID.ToString()));
				return false;
			}
		}
		else
		{
			OutField = TEXT("ResumeCondition");
			OutError = FText::FromString(TEXT("지원하지 않는 재개 조건입니다."));
			return false;
		}
	}
	return true;
}

void AStageController::LogConfigurationError(
	const FName InStageID,
	const FName Field,
	const FText& Error) const
{
	UE_LOG(LogDualFire, Error, TEXT("[Stage] ConfigureStage 실패 — StageID:%s Field:%s Error:%s"),
		*InStageID.ToString(), *Field.ToString(), *Error.ToString());
}

// ── Timeline 단계 ─────────────────────────────────────────────────────────────

void AStageController::TickTimeline(float DeltaTime)
{
	float RemainingTime = FMath::Max(DeltaTime, 0.0f);
	while (RemainingTime > KINDA_SMALL_NUMBER && CurrentState == EStageState::Timeline)
	{
		ProcessTimelineBoundary();
		if (bStagePaused)
		{
			TickStagePause(RemainingTime);
			continue;
		}
		if (ElapsedTime >= EliteTriggerTime - KINDA_SMALL_NUMBER)
		{
			SetState(EStageState::EliteCombat);
			break;
		}

		const float Boundary = GetNextTimelineBoundary();
		const float Advance = FMath::Min(RemainingTime, FMath::Max(Boundary - ElapsedTime, 0.0f));
		if (Advance <= KINDA_SMALL_NUMBER)
		{
			ElapsedTime = Boundary;
			continue;
		}

		ElapsedTime += Advance;
		RemainingTime -= Advance;
		UpdateCameraScrollSpeed();
	}
	ProcessTimelineBoundary();
}

void AStageController::ProcessTimelineBoundary()
{
	if (CurrentState != EStageState::Timeline)
	{
		return;
	}

	while (NextPauseTriggerIndex < ActivePauseTriggers.Num() &&
		ActivePauseTriggers[NextPauseTriggerIndex].TriggerTime <= ElapsedTime + KINDA_SMALL_NUMBER)
	{
		const float NextWaveTime = NextWaveIndex < ActiveWaves.Num()
			? ActiveWaves[NextWaveIndex].TriggerTime
			: TNumericLimits<float>::Max();
		if (!ShouldProcessPauseFirst(
			ActivePauseTriggers[NextPauseTriggerIndex].TriggerTime,
			NextWaveTime))
		{
			break;
		}
		BeginStagePause(ActivePauseTriggers[NextPauseTriggerIndex]);
		++NextPauseTriggerIndex;
		break;
	}

	while (NextWaveIndex < ActiveWaves.Num() &&
		ActiveWaves[NextWaveIndex].TriggerTime <= ElapsedTime + KINDA_SMALL_NUMBER)
	{
		const int32 ScopeGeneration = bStagePaused &&
			ActivePauseTrigger.ResumeCondition == EStagePauseResumeCondition::WaveDefeated &&
			FMath::IsNearlyEqual(ActiveWaves[NextWaveIndex].TriggerTime, ActivePauseTrigger.TriggerTime)
			? PauseScopeGeneration
			: 0;
		TriggerWave(ActiveWaves[NextWaveIndex], ScopeGeneration);
		++NextWaveIndex;
	}
	EvaluateStagePause();

	if (!bStagePaused && ElapsedTime >= EliteTriggerTime - KINDA_SMALL_NUMBER &&
		CurrentState == EStageState::Timeline)
	{
		SetState(EStageState::EliteCombat);
	}
}

float AStageController::GetNextTimelineBoundary() const
{
	return FindNextTimelineBoundary(
		EliteTriggerTime,
		ActivePauseTriggers,
		NextPauseTriggerIndex,
		ActiveWaves,
		NextWaveIndex,
		ElapsedTime);
}

void AStageController::BeginStagePause(const FStagePauseTrigger& Trigger)
{
	ActivePauseTrigger = Trigger;
	PauseRealTime = 0.0f;
	PendingPauseScopedSpawns = 0;
	PauseScopedEnemies.Reset();
	bTargetEnemyDefeated = false;
	bStagePaused = true;
	++PauseScopeGeneration;
	if (AStageCameraActor* Camera = GetStageCamera())
	{
		Camera->SetPaused(true);
	}
	UE_LOG(LogDualFire, Log, TEXT("[Stage] 멈춤 시작 — StageID:%s Time:%.3f Condition:%s"),
		*StageID.ToString(), Trigger.TriggerTime, *UEnum::GetValueAsString(Trigger.ResumeCondition));
}

void AStageController::TickStagePause(float& RemainingTime)
{
	if (ActivePauseTrigger.ResumeCondition != EStagePauseResumeCondition::RealTime)
	{
		RemainingTime = 0.0f;
		return;
	}

	const float Needed = FMath::Max(ActivePauseTrigger.ResumeDelay - PauseRealTime, 0.0f);
	const float Advance = FMath::Min(RemainingTime, Needed);
	PauseRealTime += Advance;
	RemainingTime -= Advance;
	EvaluateStagePause();
}

void AStageController::EvaluateStagePause()
{
	if (!bStagePaused)
	{
		return;
	}
	for (auto It = PauseScopedEnemies.CreateIterator(); It; ++It)
	{
		if (!It->IsValid() || It->Get()->IsHidden())
		{
			It.RemoveCurrent();
		}
	}
	if (ShouldResumePause(
		ActivePauseTrigger,
		PauseRealTime,
		PauseScopedEnemies.Num(),
		PendingPauseScopedSpawns,
		bTargetEnemyDefeated))
	{
		EndStagePause();
	}
}

void AStageController::EndStagePause()
{
	if (!bStagePaused)
	{
		return;
	}
	bStagePaused = false;
	PauseScopedEnemies.Reset();
	PendingPauseScopedSpawns = 0;
	if (AStageCameraActor* Camera = GetStageCamera())
	{
		Camera->SetPaused(false);
	}
	UpdateCameraScrollSpeed();
	UE_LOG(LogDualFire, Log, TEXT("[Stage] 멈춤 종료 — StageID:%s Time:%.3f"),
		*StageID.ToString(), ElapsedTime);
}

float AStageController::EvaluateScrollSpeed(const FStageRow& StageRow, const float StageTime)
{
	if (!IsValid(StageRow.NormalizedScrollCurve))
	{
		return 0.0f;
	}
	const float Normalized = FMath::Clamp(
		StageRow.NormalizedScrollCurve->GetFloatValue(StageTime), 0.0f, 1.0f);
	return FMath::Lerp(StageRow.MinScrollSpeed, StageRow.MaxScrollSpeed, Normalized);
}

bool AStageController::ShouldResumePause(
	const FStagePauseTrigger& Trigger,
	const float InPauseRealTime,
	const int32 ActiveScopedEnemies,
	const int32 PendingScopedSpawns,
	const bool bInTargetEnemyDefeated)
{
	switch (Trigger.ResumeCondition)
	{
	case EStagePauseResumeCondition::RealTime:
		return InPauseRealTime >= Trigger.ResumeDelay - KINDA_SMALL_NUMBER;
	case EStagePauseResumeCondition::WaveDefeated:
		return ActiveScopedEnemies == 0 && PendingScopedSpawns == 0;
	case EStagePauseResumeCondition::EnemyDefeated:
		return bInTargetEnemyDefeated;
	default:
		return false;
	}
}

void AStageController::UpdateCameraScrollSpeed()
{
	if (AStageCameraActor* Camera = GetStageCamera())
	{
		Camera->SetScrollSpeed(EvaluateScrollSpeed(ActiveStageRow, ElapsedTime));
		Camera->SetPaused(bStagePaused);
	}
}

void AStageController::TriggerWave(const FWaveRow& Wave, const int32 ScopeGeneration)
{
	UE_LOG(LogDualFire, Log, TEXT("[Stage] 웨이브 트리거 — %s (Count=%d)"),
		*Wave.WaveID.ToString(), Wave.Count);
	if (ScopeGeneration > 0)
	{
		PendingPauseScopedSpawns += Wave.Count;
	}
	SpawnWaveSequential(Wave, 0, ScopeGeneration);
}

void AStageController::SpawnWaveSequential(
	FWaveRow Wave,
	const int32 AlreadySpawned,
	const int32 ScopeGeneration)
{
	if (AlreadySpawned >= Wave.Count)
	{
		return;
	}

	AEnemyBase* SpawnedEnemy = nullptr;
	const TSubclassOf<AEnemyBase> EnemyClass = ResolveEnemyClass(Wave.EnemyID);
	if (!IsValid(EnemyClass))
	{
		UE_LOG(LogDualFire, Error, TEXT("[Stage] EnemyID '%s'에 대한 클래스 없음 — 스폰 생략"),
			*Wave.EnemyID.ToString());
	}
	else
	{
		FEnemyRow EnemyRow;
		if (!FindEnemyRow(Wave.EnemyID, EnemyRow))
		{
			UE_LOG(LogDualFire, Error, TEXT("[Stage] EnemyID '%s' 데이터 없음 — 스폰 생략"),
				*Wave.EnemyID.ToString());
		}
		else
		{
			const FVector SpawnLoc = ResolveSpawnAnchor(Wave.SpawnAnchor, Wave.SpawnOffset);
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			UActorPoolSubsystem* Pool = GetWorld()->GetSubsystem<UActorPoolSubsystem>();
			AEnemyBase* Enemy = IsValid(Pool)
				? Cast<AEnemyBase>(Pool->AcquireActor(EnemyClass, FTransform(EnemyFacingRotation, SpawnLoc)))
				: GetWorld()->SpawnActor<AEnemyBase>(EnemyClass, SpawnLoc, EnemyFacingRotation, Params);

			if (IsValid(Enemy))
			{
				if (UEnemyAIComponent* AI = Enemy->GetAIComponent();
					IsValid(AI) && !IsValid(AI->ProjectileClass) && IsValid(EnemyProjectileClass))
				{
					AI->ProjectileClass = EnemyProjectileClass;
				}

				if (!Enemy->InitFromEnemyRow(EnemyRow))
				{
					UE_LOG(LogDualFire, Error,
						TEXT("[Stage] EnemyID '%s' 초기화 실패 — 등록 없이 반환"),
						*Wave.EnemyID.ToString());
					if (IsValid(Pool)) Pool->ReleaseActor(Enemy);
					else Enemy->Destroy();
				}
				else
				{
					RegisterEnemy(Enemy);
					SpawnedEnemy = Enemy;
				}
			}
		}
	}
	MarkPauseScopedSpawnComplete(ScopeGeneration, SpawnedEnemy);

	const int32 NextCount = AlreadySpawned + 1;
	if (NextCount < Wave.Count && Wave.SpawnInterval > 0.0f)
	{
		FTimerHandle SeqHandle;
		FTimerDelegate Del;
		Del.BindUObject(this, &AStageController::SpawnWaveSequential, Wave, NextCount, ScopeGeneration);
		GetWorld()->GetTimerManager().SetTimer(SeqHandle, Del, Wave.SpawnInterval, false);
		SequenceSpawnTimerHandles.Add(SeqHandle);
	}
	else if (NextCount < Wave.Count)
	{
		// SpawnInterval == 0이면 즉시 재귀
		SpawnWaveSequential(Wave, NextCount, ScopeGeneration);
	}
}

void AStageController::MarkPauseScopedSpawnComplete(
	const int32 ScopeGeneration,
	AEnemyBase* SpawnedEnemy)
{
	if (ScopeGeneration <= 0 || ScopeGeneration != PauseScopeGeneration ||
		ActivePauseTrigger.ResumeCondition != EStagePauseResumeCondition::WaveDefeated)
	{
		return;
	}
	PendingPauseScopedSpawns = FMath::Max(PendingPauseScopedSpawns - 1, 0);
	if (IsValid(SpawnedEnemy))
	{
		PauseScopedEnemies.Add(SpawnedEnemy);
	}
	EvaluateStagePause();
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
		GetWorld()->GetTimerManager().ClearTimer(PrototypeBossArrivalTimeoutHandle);
		for (FTimerHandle& Handle : SequenceSpawnTimerHandles)
		{
			GetWorld()->GetTimerManager().ClearTimer(Handle);
		}
		SequenceSpawnTimerHandles.Reset();
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

	for (AEnemyBase* Enemy : ActiveEnemies)
	{
		if (UEnemyAIComponent* AI = IsValid(Enemy) ? Enemy->GetAIComponent() : nullptr; IsValid(AI))
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
		GM->OnMissionFail(EDualFireMissionFailureReason::StageConditionFailed);
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
	const FVector2D Ratios = GetSpawnAnchorRatios(Anchor);
	const float SpawnX = FMath::Lerp(Bounds.Min.X, Bounds.Max.X, Ratios.X);
	const float SpawnY = FMath::Lerp(Bounds.Min.Y, Bounds.Max.Y, Ratios.Y);
	return FVector(SpawnX, SpawnY, 0.0f) + Offset;
}

FVector2D AStageController::GetSpawnAnchorRatios(ESpawnAnchor Anchor)
{
	switch (Anchor)
	{
	case ESpawnAnchor::TopLeft:
		return FVector2D(1.0f, 0.0f);
	case ESpawnAnchor::TopCenter:
		return FVector2D(1.0f, 0.5f);
	case ESpawnAnchor::TopRight:
		return FVector2D(1.0f, 1.0f);
	case ESpawnAnchor::Left:
		return FVector2D(0.5f, 0.0f);
	case ESpawnAnchor::Center:
		return FVector2D(0.5f, 0.5f);
	case ESpawnAnchor::Right:
		return FVector2D(0.5f, 1.0f);
	case ESpawnAnchor::BottomLeft:
		return FVector2D(0.0f, 0.0f);
	case ESpawnAnchor::BottomCenter:
		return FVector2D(0.0f, 0.5f);
	case ESpawnAnchor::BottomRight:
		return FVector2D(0.0f, 1.0f);
	}
	return FVector2D(0.5f, 0.5f);
}

float AStageController::FindNextTimelineBoundary(
	const float InEliteTriggerTime,
	const TArray<FStagePauseTrigger>& PauseTriggers,
	const int32 PauseTriggerIndex,
	const TArray<FWaveRow>& Waves,
	const int32 WaveIndex,
	const float InElapsedTime)
{
	float Boundary = InEliteTriggerTime;
	if (PauseTriggers.IsValidIndex(PauseTriggerIndex))
	{
		Boundary = FMath::Min(Boundary, PauseTriggers[PauseTriggerIndex].TriggerTime);
	}
	if (Waves.IsValidIndex(WaveIndex))
	{
		Boundary = FMath::Min(Boundary, Waves[WaveIndex].TriggerTime);
	}
	return FMath::Max(Boundary, InElapsedTime);
}

bool AStageController::ShouldProcessPauseFirst(
	const float PauseTriggerTime,
	const float WaveTriggerTime)
{
	return PauseTriggerTime <= WaveTriggerTime + KINDA_SMALL_NUMBER;
}

AStageCameraActor* AStageController::GetStageCamera() const
{
	if (ADualFireGameModeBase* GM = Cast<ADualFireGameModeBase>(UGameplayStatics::GetGameMode(this)))
	{
		return GM->GetStageCamera();
	}
	return nullptr;
}

void AStageController::RegisterEnemy(AEnemyBase* Enemy)
{
	if (!IsValid(Enemy) || ActiveEnemies.Contains(Enemy))
	{
		return;
	}

	ActiveEnemies.AddUnique(Enemy);

	if (Enemy->CountsTowardMissionMetrics())
	{
		RecordEnemySpawned(Enemy->GetEnemyAttributes_Implementation());
	}
}

void AStageController::UnregisterEnemy(AEnemyBase* Enemy)
{
	if (!IsValid(Enemy) || ActiveEnemies.RemoveSingleSwap(Enemy) == 0)
	{
		return;
	}
	PauseScopedEnemies.Remove(Enemy);
	EvaluateStagePause();
}

void AStageController::RecordEnemySpawned(const FEnemyAttribute& Attribute)
{
	AirEnemiesSpawned += Attribute.HasAir() ? 1 : 0;
	GroundEnemiesSpawned += Attribute.HasGround() ? 1 : 0;
}

void AStageController::NotifyEnemyDefeated(AEnemyBase* Enemy)
{
	if (!IsValid(Enemy) || !ActiveEnemies.Contains(Enemy))
	{
		return;
	}
	if (bStagePaused && ActivePauseTrigger.ResumeCondition == EStagePauseResumeCondition::EnemyDefeated &&
		Enemy->GetRuntimeEnemyID() == ActivePauseTrigger.TargetEnemyID)
	{
		bTargetEnemyDefeated = true;
		EvaluateStagePause();
	}
	if (!Enemy->CountsTowardMissionMetrics())
	{
		return;
	}

	const FEnemyAttribute Attribute = Enemy->GetEnemyAttributes_Implementation();
	AirEnemiesDefeated += Attribute.HasAir() ? 1 : 0;
	GroundEnemiesDefeated += Attribute.HasGround() ? 1 : 0;
}

void AStageController::RegisterPlacedEnemies()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	for (TActorIterator<AEnemyBase> It(World); It; ++It)
	{
		AEnemyBase* Enemy = *It;
		if (IsValid(Enemy) && !Enemy->IsHidden() && Enemy->GetActorEnableCollision())
		{
			RegisterEnemy(Enemy);
		}
	}
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

	for (int32 Index = ActiveEnemies.Num() - 1; Index >= 0; --Index)
	{
		AEnemyBase* Enemy = ActiveEnemies[Index];
		if (!IsValid(Enemy) || Enemy->IsHidden())
		{
			ActiveEnemies.RemoveAtSwap(Index);
			continue;
		}

		if (UEnemyAIComponent* AI = Enemy->GetAIComponent(); IsValid(AI))
		{
			AI->UpdateAI(DeltaTime, Bounds, PlayerLocation, bPlayerLocationValid);
		}
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
