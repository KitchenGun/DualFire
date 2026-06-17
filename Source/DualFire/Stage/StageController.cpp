// Copyright DualFire. All Rights Reserved.

#include "Stage/StageController.h"
#include "Enemy/EnemyBase.h"
#include "Camera/StageCameraActor.h"
#include "GameModes/DualFireGameModeBase.h"
#include "DualFire.h"

#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

AStageController::AStageController()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false; // BeginPlay에서 활성화
}

void AStageController::BeginPlay()
{
	Super::BeginPlay();

	BuildActiveWaves();
	SetState(EStageState::Timeline);

	SetActorTickEnabled(true);
}

void AStageController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (CurrentState == EStageState::Timeline)
	{
		TickTimeline(DeltaTime);
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
	// else: DataTable 조회 — 미구현 (다음 청크)

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

	GetWorld()->SpawnActor<AEnemyBase>(EnemyClass, SpawnLoc, FRotator::ZeroRotator, Params);

	const int32 NextCount = AlreadySpawned + 1;
	if (NextCount < Wave.Count && Wave.SpawnInterval > 0.0f)
	{
		FTimerHandle SeqHandle;
		FTimerDelegate Del;
		Del.BindUObject(this, &AStageController::SpawnWaveSequential, Wave, NextCount);
		GetWorld()->GetTimerManager().SetTimer(SeqHandle, Del, Wave.SpawnInterval, false);
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
		SetActorTickEnabled(false); // 웨이브 Tick 중단
		GetWorld()->GetTimerManager().SetTimer(
			EliteTimeLimitHandle,
			this,
			&AStageController::OnEliteTimeLimitExpired,
			EliteTimeLimit,
			false);
		break;

	case EStageState::Ended:
		SetActorTickEnabled(false);
		GetWorld()->GetTimerManager().ClearTimer(EliteTimeLimitHandle);
		// 결과별 GameMode 호출은 OnEliteDefeated / OnEliteTimeLimitExpired에서 수행
		break;

	default:
		break;
	}
}

void AStageController::OnEliteDefeated()
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

	// X: 화면 우측 바깥에서 스폰
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
