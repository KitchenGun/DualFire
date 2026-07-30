// Copyright DualFire. All Rights Reserved.

#include "DualFireGameModeBase.h"

#include "DualFire.h"
#include "Camera/StageCameraActor.h"
#include "Player/DualFirePlayerPawn.h"
#include "Stage/StageController.h"
#include "Loadout/LoadoutManagerSubsystem.h"
#include "Core/LoadoutDataLibrary.h"
#include "UI/DualFireMissionPlayerController.h"

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

    // 격납고 등 외부에서 이미 SetActiveLoadout()을 호출하고 넘어온 경우는 덮어쓰지 않는다.
    if (!LM->GetActiveLoadout().AircraftID.IsNone())
    {
        return;
    }

    LM->SetActiveLoadout(ULoadoutDataLibrary::MakeLoadoutFromRowHandles(TestLoadout));
    UE_LOG(LogDualFire, Log, TEXT("[GameMode] InitGame: 외부 로드아웃 없음 — TestLoadout으로 폴백"));
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
    // ── 1. LoadoutManager → PlayerPawn 주입 ──────────────────────────────────
    UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
    if (IsValid(GI))
    {
        ULoadoutManagerSubsystem* LM = GI->GetSubsystem<ULoadoutManagerSubsystem>();
        ADualFirePlayerPawn* Pawn = Cast<ADualFirePlayerPawn>(
            UGameplayStatics::GetPlayerPawn(this, 0));

        if (IsValid(LM) && IsValid(Pawn))
        {
            LM->ApplyToPlayer(Pawn);
        }
        else
        {
            UE_LOG(LogDualFire, Warning,
                TEXT("[GameMode] StartMission: LoadoutManager 또는 PlayerPawn 없음 — 기본값으로 진행"));
        }
    }

    // ── 2. StageController 스폰 ───────────────────────────────────────────────
    if (IsValid(StageControllerClass))
    {
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        ActiveStageController = GetWorld()->SpawnActor<AStageController>(
            StageControllerClass, FVector::ZeroVector, FRotator::ZeroRotator, Params);

        if (!IsValid(ActiveStageController))
        {
            UE_LOG(LogDualFire, Error, TEXT("[GameMode] StageController 스폰 실패"));
        }
    }
    else
    {
        UE_LOG(LogDualFire, Warning,
            TEXT("[GameMode] StageControllerClass 미설정 — 웨이브 없이 진행"));
    }

    UE_LOG(LogDualFire, Log, TEXT("[GameMode] StartMission 완료"));
}

// ── 미션 종료 ───────────────────────────────────────────────────────────────────

void ADualFireGameModeBase::OnMissionFail()
{
    EndMission(EMissionResult::Failed);
}

void ADualFireGameModeBase::OnMissionClear()
{
    EndMission(EMissionResult::Cleared);
}

void ADualFireGameModeBase::EndMission(EMissionResult Result)
{
    // 중복 종료 방지 (잔여 기체 0 사망과 엘리트 타임아웃이 동시에 들어오는 경우 등)
    if (MissionResult != EMissionResult::None)
    {
        return;
    }
    MissionResult = Result;
	BuildMissionResultData(Result);

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

void ADualFireGameModeBase::BuildMissionResultData(EMissionResult Result)
{
	MissionResultData = FDualFireMissionResultData();
	MissionResultData.Result = Result;
	MissionResultData.MissionCode = MissionCode;
	MissionResultData.MissionName = MissionDisplayName;
	MissionResultData.Difficulty = DifficultyDisplayName;

	if (IsValid(ActiveStageController))
	{
		MissionResultData.StageID = ActiveStageController->StageID;
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
