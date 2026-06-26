// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Core/DualFireTypes.h"
#include "DualFireGameModeBase.generated.h"

// Forward declarations — 헤더 인클루드 최소화
class AStageCameraActor;
class ADualFirePlayerPawn;
class AStageController;
class ULoadoutManagerSubsystem;

/** 미션 종료 시 브로드캐스트. 결과(Cleared/Failed)를 HUD·연출에 전달 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMissionEnded, EMissionResult, Result);

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

    // ── 미션 시작 ───────────────────────────────────────────────────────────────

    /** 스폰할 StageController 클래스. BP_StageController 등을 할당 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mission")
    TSubclassOf<AStageController> StageControllerClass;

    /**
     * 미션을 시작한다.
     *   1. LoadoutManager에서 ActiveLoadout을 읽어 PlayerPawn에 주입
     *   2. StageControllerClass를 스폰해 웨이브/타임라인 시작
     * BeginPlay 이후 외부(UI 등)에서 호출하거나, BeginPlay 마지막에서 자동 호출 가능.
     */
    UFUNCTION(BlueprintCallable, Category="Mission")
    void StartMission();

    UFUNCTION(BlueprintPure, Category="Mission")
    AStageController* GetStageController() const { return ActiveStageController; }

    // ── 미션 종료 ───────────────────────────────────────────────────────────────

    /**
     * 미션 실패 처리. 잔기 소진 사망(§5.3.7) 또는 엘리트 제한 시간 초과(§5.5.4) 시 호출.
     * 검증 리포트 로그 출력 + 입력 잠금. 이미 종료된 경우 무시.
     */
    UFUNCTION(BlueprintCallable, Category="Mission")
    void OnMissionFail();

    /**
     * 미션 클리어 처리. 엘리트 격파(§5.5.4) 시 호출.
     * 검증 리포트 로그 출력. 이미 종료된 경우 무시.
     */
    UFUNCTION(BlueprintCallable, Category="Mission")
    void OnMissionClear();

    /** 현재 미션 결과. None=진행 중, Cleared/Failed=종료 */
    UFUNCTION(BlueprintPure, Category="Mission")
    EMissionResult GetMissionResult() const { return MissionResult; }

    /** 미션 종료 시 발생. HUD 결과 화면·연출 구독용 */
    UPROPERTY(BlueprintAssignable, Category="Mission")
    FOnMissionEnded OnMissionEnded;

private:
    // BeginPlay에서 스폰 후 캐싱한 스테이지 카메라
    UPROPERTY()
    TObjectPtr<AStageCameraActor> StageCamera;

    // StartMission에서 스폰한 현재 스테이지 컨트롤러
    UPROPERTY()
    TObjectPtr<AStageController> ActiveStageController;

    // 현재 미션 결과 (중복 종료 방지용)
    EMissionResult MissionResult = EMissionResult::None;

    /** 미션 종료 공통 처리 — 결과 확정, 입력 잠금, 리포트 출력, 델리게이트 */
    void EndMission(EMissionResult Result);
};
