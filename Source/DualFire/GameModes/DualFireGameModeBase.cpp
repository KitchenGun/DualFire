// Copyright DualFire. All Rights Reserved.

#include "DualFireGameModeBase.h"

#include "DualFire.h"
#include "Camera/StageCameraActor.h"
#include "Player/DualFirePlayerPawn.h"
#include "Stage/StageController.h"
#include "Loadout/LoadoutManager.h"

#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

ADualFireGameModeBase::ADualFireGameModeBase()
{
    // ── 기본 폰 클래스 ─────────────────────────────────────────────────────────
    // BP_DualFireGameModeBase에서 BP_DualFirePlayerPawn으로 오버라이드 권장
    DefaultPawnClass = ADualFirePlayerPawn::StaticClass();
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
        return;
    }

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
        return;
    }

    // ── 첫 번째 PlayerController에 ViewTarget 설정 ────────────────────────────
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

void ADualFireGameModeBase::StartMission()
{
    // ── 1. LoadoutManager → PlayerPawn 주입 ──────────────────────────────────
    UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
    if (IsValid(GI))
    {
        ULoadoutManager* LM = GI->GetSubsystem<ULoadoutManager>();
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
