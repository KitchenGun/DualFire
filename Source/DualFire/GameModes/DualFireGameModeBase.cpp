// Copyright DualFire. All Rights Reserved.

#include "DualFireGameModeBase.h"

#include "DualFire.h"
#include "Camera/StageCameraActor.h"
#include "Player/DualFirePlayerPawn.h"
#include "Stage/StageController.h"
#include "Loadout/LoadoutManagerSubsystem.h"
#include "Core/LoadoutDataLibrary.h"

#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

ADualFireGameModeBase::ADualFireGameModeBase()
{
    // ── 기본 폰 클래스 ─────────────────────────────────────────────────────────
    // BP_DualFireGameModeBase에서 BP_DualFirePlayerPawn으로 오버라이드 권장
    DefaultPawnClass = ADualFirePlayerPawn::StaticClass();
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
