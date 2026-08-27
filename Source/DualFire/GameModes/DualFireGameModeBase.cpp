// Copyright DualFire. All Rights Reserved.

#include "DualFireGameModeBase.h"

#include "DualFire.h"
#include "Camera/StageCameraActor.h"
#include "Player/DualFirePlayerPawn.h"
#include "Stage/StageController.h"
#include "Loadout/LoadoutManagerSubsystem.h"
#include "GameInstance/DualFireMissionFlowSubsystem.h"
#include "Core/LoadoutDataLibrary.h"
#include "UI/DualFireMissionPlayerController.h"

#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	int32 RankToScore(EDualFireMissionRank Rank)
	{
		switch (Rank)
		{
		case EDualFireMissionRank::S: return 4;
		case EDualFireMissionRank::A: return 3;
		case EDualFireMissionRank::B: return 2;
		case EDualFireMissionRank::C: return 1;
		case EDualFireMissionRank::D: return 0;
		default: return 0;
		}
	}

	EDualFireMissionRank ScoreToRank(int32 Score)
	{
		switch (FMath::Clamp(Score, 0, 4))
		{
		case 4: return EDualFireMissionRank::S;
		case 3: return EDualFireMissionRank::A;
		case 2: return EDualFireMissionRank::B;
		case 1: return EDualFireMissionRank::C;
		default: return EDualFireMissionRank::D;
		}
	}

	EDualFireMissionRank RankKillRate(float Rate)
	{
		if (Rate >= 1.0f) return EDualFireMissionRank::S;
		if (Rate >= 0.85f) return EDualFireMissionRank::A;
		if (Rate >= 0.70f) return EDualFireMissionRank::B;
		if (Rate >= 0.50f) return EDualFireMissionRank::C;
		return EDualFireMissionRank::D;
	}

	EDualFireMissionRank RankHits(int32 Count)
	{
		if (Count == 0) return EDualFireMissionRank::S;
		if (Count <= 3) return EDualFireMissionRank::A;
		if (Count <= 6) return EDualFireMissionRank::B;
		if (Count <= 10) return EDualFireMissionRank::C;
		return EDualFireMissionRank::D;
	}

	EDualFireMissionRank RankDeaths(int32 Count)
	{
		if (Count == 0) return EDualFireMissionRank::S;
		if (Count <= 1) return EDualFireMissionRank::A;
		if (Count <= 2) return EDualFireMissionRank::B;
		if (Count <= 3) return EDualFireMissionRank::C;
		return EDualFireMissionRank::D;
	}
}

ADualFireGameModeBase::ADualFireGameModeBase()
{
    // ── 기본 폰 클래스 ─────────────────────────────────────────────────────────
    // BP_DualFireGameModeBase에서 BP_DualFirePlayerPawn으로 오버라이드 권장
    DefaultPawnClass = ADualFirePlayerPawn::StaticClass();
	PlayerControllerClass = ADualFireMissionPlayerController::StaticClass();
}

void ADualFireGameModeBase::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    Super::InitGame(MapName, Options, ErrorMessage);

    // Pawn 스폰(GetDefaultPawnClassForController_Implementation)보다 먼저 실행되어야
    // TestLoadout이 기체 선택에도 반영된다. BeginPlay는 이미 늦음(Pawn 스폰 이후 호출).
    UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
    if (!IsValid(GI))
    {
        return;
    }

    ULoadoutManagerSubsystem* LM = GI->GetSubsystem<ULoadoutManagerSubsystem>();
    if (!IsValid(LM))
    {
        return;
    }

	UDualFireMissionFlowSubsystem* Flow = GI->GetSubsystem<UDualFireMissionFlowSubsystem>();
	if (!IsValid(Flow))
	{
		UE_LOG(LogDualFire, Error, TEXT("[GameMode] InitGame: MissionFlow 없음 — 미션 시작 차단"));
		return;
	}

	FText LoadoutError;
	FName InvalidField;
	if (Flow->HasLaunchContext())
	{
		const FMissionLaunchContext Launch = Flow->GetLaunchContext();
		if (!LM->TrySetActiveLoadout(Launch.Loadout, LoadoutError, InvalidField))
		{
			UE_LOG(LogDualFire, Error,
				TEXT("[GameMode] InitGame: 출격 로드아웃 무효 — Field:%s Error:%s"),
				*InvalidField.ToString(), *LoadoutError.ToString());
		}
		return;
	}

	if (Flow->HasPreparationContext())
	{
		UE_LOG(LogDualFire, Error,
			TEXT("[GameMode] InitGame: 준비 컨텍스트가 출격 승인되지 않음 — 미션 시작 차단"));
		return;
	}

	FText FlowError;
	FName FlowField;
	if (!Flow->TryBeginPreparation(TEXT("MISSION_01"), TEXT("NORMAL"), FlowError, FlowField))
	{
		UE_LOG(LogDualFire, Error,
			TEXT("[GameMode] InitGame: 직접 실행 준비 실패 — Field:%s Error:%s"),
			*FlowField.ToString(), *FlowError.ToString());
		return;
	}

	const FLoadout DirectLoadout = ULoadoutDataLibrary::MakeLoadoutFromRowHandles(TestLoadout);
	if (!Flow->TryFinalizeLaunch(DirectLoadout, LoadoutError, InvalidField))
	{
		UE_LOG(LogDualFire, Error,
			TEXT("[GameMode] InitGame: TestLoadout 검증 실패 — Field:%s Error:%s"),
			*InvalidField.ToString(), *LoadoutError.ToString());
		return;
	}

	UE_LOG(LogDualFire, Log,
		TEXT("[GameMode] InitGame: LV_Test 직접 실행 — MISSION_01/STAGE_TEST TestLoadout 적용"));
}

UClass* ADualFireGameModeBase::GetDefaultPawnClassForController_Implementation(AController* InController)
{
    UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
    if (IsValid(GI))
    {
        if (ULoadoutManagerSubsystem* LM = GI->GetSubsystem<ULoadoutManagerSubsystem>())
        {
            if (TSubclassOf<ADualFirePlayerPawn> AircraftClass = LM->ResolveAircraftClass())
            {
                return AircraftClass;
            }
        }
    }

    return Super::GetDefaultPawnClassForController_Implementation(InController);
}

void ADualFireGameModeBase::BeginPlay()
{
    Super::BeginPlay();

    // ── StageCameraActor 스폰 ─────────────────────────────────────────────────
    if (!IsValid(StageCameraClass))
    {
        UE_LOG(LogDualFire, Warning,
            TEXT("ADualFireGameModeBase: StageCameraClass 미설정 — "
                 "Details > Camera > StageCameraClass를 할당하세요."));
    }
    else
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride =
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        StageCamera = GetWorld()->SpawnActor<AStageCameraActor>(
            StageCameraClass,
            StageCameraStartTransform,
            SpawnParams);

        if (!IsValid(StageCamera))
        {
            UE_LOG(LogDualFire, Error,
                TEXT("ADualFireGameModeBase: StageCameraActor 스폰 실패."));
        }
        else
        {
            // ── 첫 번째 PlayerController에 ViewTarget 설정 ────────────────────
            APlayerController* PC = GetWorld()->GetFirstPlayerController();
            if (IsValid(PC))
            {
                PC->SetViewTargetWithBlend(StageCamera, 0.f);
            }
            else
            {
                UE_LOG(LogDualFire, Warning,
                    TEXT("ADualFireGameModeBase: PlayerController를 찾을 수 없습니다."));
            }
        }
    }

    StartMission();
}

void ADualFireGameModeBase::StartMission()
{
    if (!IsValid(StageControllerClass))
    {
        UE_LOG(LogDualFire, Error,
            TEXT("[GameMode] StartMission: StageControllerClass 미설정 — 미션 시작 차단"));
        return;
    }

    // ── 1. LoadoutManager → PlayerPawn 주입 ──────────────────────────────────
    UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
    if (!IsValid(GI))
    {
        UE_LOG(LogDualFire, Error, TEXT("[GameMode] StartMission: GameInstance 없음 — 미션 시작 차단"));
        return;
    }

    ULoadoutManagerSubsystem* LM = GI->GetSubsystem<ULoadoutManagerSubsystem>();
	UDualFireMissionFlowSubsystem* Flow = GI->GetSubsystem<UDualFireMissionFlowSubsystem>();
    ADualFirePlayerPawn* Pawn = Cast<ADualFirePlayerPawn>(
        UGameplayStatics::GetPlayerPawn(this, 0));
    if (!IsValid(LM) || !IsValid(Flow) || !Flow->HasLaunchContext() || !IsValid(Pawn))
    {
        UE_LOG(LogDualFire, Error,
            TEXT("[GameMode] StartMission: 출격 컨텍스트, LoadoutManager 또는 PlayerPawn 없음 — 미션 시작 차단"));
        return;
    }

    FText LoadoutError;
    FName InvalidField;
    if (!LM->TryApplyActiveLoadout(Pawn, LoadoutError, InvalidField))
    {
        UE_LOG(LogDualFire, Error,
            TEXT("[GameMode] StartMission: 로드아웃 적용 실패 — Field:%s Error:%s"),
            *InvalidField.ToString(),
            *LoadoutError.ToString());
        return;
    }

    // ── 2. StageController 스폰 ───────────────────────────────────────────────
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    ActiveStageController = GetWorld()->SpawnActor<AStageController>(
        StageControllerClass, FVector::ZeroVector, FRotator::ZeroRotator, Params);

    if (!IsValid(ActiveStageController))
    {
        UE_LOG(LogDualFire, Error, TEXT("[GameMode] StartMission: StageController 스폰 실패 — 미션 시작 차단"));
        return;
    }

    UE_LOG(LogDualFire, Log, TEXT("[GameMode] StartMission 완료"));
}

// ── 미션 종료 ───────────────────────────────────────────────────────────────────

void ADualFireGameModeBase::OnMissionFail(const EDualFireMissionFailureReason FailureReason)
{
    EndMission(EMissionResult::Failed, FailureReason);
}

void ADualFireGameModeBase::OnMissionClear()
{
    EndMission(EMissionResult::Cleared, EDualFireMissionFailureReason::None);
}

void ADualFireGameModeBase::EndMission(
	const EMissionResult Result,
	const EDualFireMissionFailureReason FailureReason)
{
    // 중복 종료 방지 (잔여 기체 0 사망과 엘리트 타임아웃이 동시에 들어오는 경우 등)
    if (MissionResult != EMissionResult::None)
    {
        return;
    }
    MissionResult = Result;
	BuildMissionResultData(Result, FailureReason);
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UDualFireMissionFlowSubsystem* Flow =
			GameInstance->GetSubsystem<UDualFireMissionFlowSubsystem>())
		{
			Flow->StoreResult(MissionResultData);
		}
	}

    const TCHAR* ResultText = (Result == EMissionResult::Cleared) ? TEXT("CLEARED") : TEXT("FAILED");

    // §9.4 검증 리포트 (간이판 — 콘솔 로그)
    UE_LOG(LogDualFire, Warning, TEXT("========================="));
    UE_LOG(LogDualFire, Warning, TEXT(" Mission Result : %s"), ResultText);
    UE_LOG(LogDualFire, Warning, TEXT("========================="));

    // 입력 잠금 — 종료 후 플레이어 조작 차단
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        PC->SetIgnoreMoveInput(true);
        PC->SetIgnoreLookInput(true);
        if (APawn* PlayerPawn = PC->GetPawn())
        {
            PlayerPawn->DisableInput(PC);
        }
    }

    OnMissionEnded.Broadcast(Result);
}

void ADualFireGameModeBase::BuildMissionResultData(
	const EMissionResult Result,
	const EDualFireMissionFailureReason FailureReason)
{
	MissionResultData = FDualFireMissionResultData();
	MissionResultData.Result = Result;
	MissionResultData.FailureReason = FailureReason;

	if (UGameInstance* GI = GetGameInstance())
	{
		if (const UDualFireMissionFlowSubsystem* Flow = GI->GetSubsystem<UDualFireMissionFlowSubsystem>();
			IsValid(Flow) && Flow->HasLaunchContext())
		{
			const FMissionLaunchContext Launch = Flow->GetLaunchContext();
			MissionResultData.MissionCode = Launch.Preparation.MissionRow.MissionCode;
			MissionResultData.MissionName = Launch.Preparation.MissionRow.DisplayName;
			MissionResultData.Difficulty = FText::FromName(Launch.Preparation.DifficultyID);
			MissionResultData.StageID = Launch.Preparation.StageID;
		}
	}

	if (IsValid(ActiveStageController))
	{
		MissionResultData.ElapsedTime = ActiveStageController->GetElapsedTime();
	}

	if (Result != EMissionResult::Cleared)
	{
		return;
	}

	FDualFireMissionMetricResult ScoreMetric;
	ScoreMetric.Metric = EDualFireMissionMetric::Score;
	ScoreMetric.DisplayName = FText::FromString(TEXT("SCORE"));
	ScoreMetric.DisplayValue = FText::FromString(TEXT("N/A"));
	MissionResultData.Metrics.Add(ScoreMetric);

	int32 WeightedScore = 0;
	int32 TotalWeight = 0;
	const auto AddKillMetric = [this, &WeightedScore, &TotalWeight](
		EDualFireMissionMetric Metric,
		const TCHAR* Label,
		int32 Defeated,
		int32 Spawned)
	{
		FDualFireMissionMetricResult MetricResult;
		MetricResult.Metric = Metric;
		MetricResult.DisplayName = FText::FromString(Label);
		MetricResult.bApplicable = Spawned > 0;

		if (MetricResult.bApplicable)
		{
			const int32 ClampedDefeated = FMath::Clamp(Defeated, 0, Spawned);
			MetricResult.Progress = static_cast<float>(ClampedDefeated) / static_cast<float>(Spawned);
			MetricResult.Rank = RankKillRate(MetricResult.Progress);
			MetricResult.DisplayValue = FText::FromString(FString::Printf(
				TEXT("%d / %d  (%d%%)"),
				ClampedDefeated,
				Spawned,
				FMath::RoundToInt(MetricResult.Progress * 100.0f)));
			WeightedScore += RankToScore(MetricResult.Rank) * 2;
			TotalWeight += 2;
		}
		else
		{
			MetricResult.DisplayValue = FText::FromString(TEXT("N/A"));
			MetricResult.Rank = EDualFireMissionRank::NotApplicable;
		}

		MissionResultData.Metrics.Add(MetricResult);
	};

	AddKillMetric(
		EDualFireMissionMetric::AirTargets,
		TEXT("AIR TARGETS"),
		ActiveStageController ? ActiveStageController->GetAirEnemiesDefeated() : 0,
		ActiveStageController ? ActiveStageController->GetAirEnemiesSpawned() : 0);
	AddKillMetric(
		EDualFireMissionMetric::GroundTargets,
		TEXT("GROUND TARGETS"),
		ActiveStageController ? ActiveStageController->GetGroundEnemiesDefeated() : 0,
		ActiveStageController ? ActiveStageController->GetGroundEnemiesSpawned() : 0);

	const ADualFirePlayerPawn* PlayerPawn = Cast<ADualFirePlayerPawn>(
		UGameplayStatics::GetPlayerPawn(this, 0));
	const int32 HitCount = IsValid(PlayerPawn) ? PlayerPawn->GetMissionHitCount() : 0;
	const int32 DeathCount = IsValid(PlayerPawn) ? PlayerPawn->GetMissionDeathCount() : 0;

	FDualFireMissionMetricResult HitMetric;
	HitMetric.Metric = EDualFireMissionMetric::HitsTaken;
	HitMetric.DisplayName = FText::FromString(TEXT("HITS TAKEN"));
	HitMetric.DisplayValue = FText::AsNumber(HitCount);
	HitMetric.Rank = RankHits(HitCount);
	HitMetric.Progress = static_cast<float>(RankToScore(HitMetric.Rank)) / 4.0f;
	HitMetric.bApplicable = true;
	MissionResultData.Metrics.Add(HitMetric);
	WeightedScore += RankToScore(HitMetric.Rank);
	TotalWeight += 1;

	FDualFireMissionMetricResult DeathMetric;
	DeathMetric.Metric = EDualFireMissionMetric::Deaths;
	DeathMetric.DisplayName = FText::FromString(TEXT("DEATHS"));
	DeathMetric.DisplayValue = FText::AsNumber(DeathCount);
	DeathMetric.Rank = RankDeaths(DeathCount);
	DeathMetric.Progress = static_cast<float>(RankToScore(DeathMetric.Rank)) / 4.0f;
	DeathMetric.bApplicable = true;
	MissionResultData.Metrics.Add(DeathMetric);
	WeightedScore += RankToScore(DeathMetric.Rank);
	TotalWeight += 1;

	MissionResultData.OverallRank = TotalWeight > 0
		? ScoreToRank(FMath::RoundToInt(static_cast<float>(WeightedScore) / TotalWeight))
		: EDualFireMissionRank::NotApplicable;
}
