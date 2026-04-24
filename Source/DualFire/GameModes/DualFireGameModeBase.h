// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DualFireGameModeBase.generated.h"

// Forward declarations — 헤더 인클루드 최소화
class AStageCameraActor;
class ADualFirePlayerPawn;

/**
 * DualFire 기본 게임모드.
 * BeginPlay에서 StageCameraActor를 스폰하고 첫 번째 PlayerController의
 * ViewTarget으로 설정. StageCamera 접근자를 외부(MovementComponent 등)에 노출.
 */
UCLASS(BlueprintType, Blueprintable)
class DUALFIRE_API ADualFireGameModeBase : public AGameModeBase
{
    GENERATED_BODY()

public:
    ADualFireGameModeBase();

    // ── AGameModeBase 오버라이드 ──────────────────────────────────────────────────
    virtual void BeginPlay() override;

    // ── 스테이지 카메라 설정 ──────────────────────────────────────────────────────

    // 스폰할 카메라 액터 클래스. BP에서 BP_StageCameraActor 등 할당
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera")
    TSubclassOf<AStageCameraActor> StageCameraClass;

    // 카메라 스폰 트랜스폼. 기본값 = 월드 원점
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Camera")
    FTransform StageCameraStartTransform;

    /** 현재 스테이지 카메라 반환. BeginPlay 이전엔 nullptr */
    UFUNCTION(BlueprintPure, Category="Camera")
    AStageCameraActor* GetStageCamera() const { return StageCamera; }

private:
    // BeginPlay에서 스폰 후 캐싱
    UPROPERTY()
    TObjectPtr<AStageCameraActor> StageCamera;
};
