// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Core/DualFireTypes.h"
#include "Core/DualFireDataTypes.h"
#include "Core/DualFireMissionResultTypes.h"
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
    virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
    virtual void BeginPlay() override;

    /**
     * 컨트롤러가 빙의할 Pawn 클래스를 결정.
     * LoadoutManagerSubsystem의 검증된 ActiveLoadout에 AircraftRow.AircraftClass가 있으면
     * 그 BP 클래스를 사용하고, 없으면 기본 DefaultPawnClass로 폴백.
     */
    virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

    // ── 테스트용 로드아웃 (격납고 UI 부재 시 폴백) ───────────────────────────────────

    /**
     * 격납고 레벨 등에서 LoadoutManagerSubsystem.TrySetActiveLoadout()을 미리 호출하고 넘어온 게
     * 아닐 때(= ActiveLoadout이 비어있을 때)만 사용되는 테스트용 기본 로드아웃.
     * BP 디테일 패널에서 각 슬롯을 데이터테이블 행 드롭다운으로 선택해 구성한다.
     * 격납고가 이미 선택해서 넘어온 경우는 절대 덮어쓰지 않는다 (InitGame 참고).
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Loadout|Test")
    FLoadoutRowHandles TestLoadout;

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

    /** 미션 실패 처리. 플레이어 격추와 스테이지 조건 실패를 결과 데이터에 구분해 기록한다. */
    UFUNCTION(BlueprintCallable, Category="Mission")
    void OnMissionFail(EDualFireMissionFailureReason FailureReason);

    /**
     * 미션 클리어 처리. 엘리트 격파(§5.5.4) 시 호출.
     * 검증 리포트 로그 출력. 이미 종료된 경우 무시.
     */
    UFUNCTION(BlueprintCallable, Category="Mission")
    void OnMissionClear();

    /** 현재 미션 결과. None=진행 중, Cleared/Failed=종료 */
    UFUNCTION(BlueprintPure, Category="Mission")
    EMissionResult GetMissionResult() const { return MissionResult; }

	UFUNCTION(BlueprintPure, Category="Mission")
	const FDualFireMissionResultData& GetMissionResultData() const { return MissionResultData; }

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

	UPROPERTY()
	FDualFireMissionResultData MissionResultData;

    /** 미션 종료 공통 처리 — 결과 확정, 입력 잠금, 리포트 출력, 델리게이트 */
    void EndMission(EMissionResult Result, EDualFireMissionFailureReason FailureReason);
	void BuildMissionResultData(EMissionResult Result, EDualFireMissionFailureReason FailureReason);
};
