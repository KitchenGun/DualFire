// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/DualFireUIPlayerController.h"
#include "Core/DualFireTypes.h"
#include "DualFireMissionPlayerController.generated.h"

class UInputMappingContext;
class UDualFireCombatHUDWidget;

UCLASS()
class DUALFIRE_API ADualFireMissionPlayerController : public ADualFireUIPlayerController
{
	GENERATED_BODY()

public:
	ADualFireMissionPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mission Result")
	FName ResultLevelName = TEXT("/Game/Level/LV_Result");

	/** Game 레이어에 한 번만 생성되는 전투 HUD의 native/WBP 클래스. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI|Combat HUD")
	TSubclassOf<UDualFireCombatHUDWidget> CombatHUDClass;

private:
	void AddGameplayInputMapping(APawn* InPawn);
	void RemoveGameplayInputMapping();
	void EnsureCombatHUD();
	void SetCombatHUDObservedPawn(APawn* InPawn);

	UFUNCTION()
	void HandleMissionEnded(EMissionResult Result);

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> ActiveGameplayInputMapping;

	UPROPERTY(Transient)
	TObjectPtr<UDualFireCombatHUDWidget> CombatHUD;

	bool bResultTravelStarted = false;
};
