// Copyright DualFire. All Rights Reserved.

#include "DualFireGameModeBase.h"

#include "DualFire.h"
#include "Camera/StageCameraActor.h"
#include "Player/DualFirePlayerPawn.h"   // StaticClass() 호출에 완전한 타입 필요

#include "GameFramework/PlayerController.h"

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
    // BlendTime=0 → 즉시 전환 (페이드인 연출이 필요하면 BlendTime > 0 사용)
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
