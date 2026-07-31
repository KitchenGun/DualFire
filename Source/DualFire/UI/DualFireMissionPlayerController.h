// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Core/DualFireTypes.h"
#include "DualFireMissionPlayerController.generated.h"

class UInputMappingContext;

UCLASS()
class DUALFIRE_API ADualFireMissionPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mission Result")
	FName ResultLevelName = TEXT("/Game/Level/LV_Result");

private:
	void AddGameplayInputMapping(APawn* InPawn);
	void RemoveGameplayInputMapping();

	UFUNCTION()
	void HandleMissionEnded(EMissionResult Result);

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> ActiveGameplayInputMapping;

	bool bGameplayInputMappingAdded = false;
	bool bResultTravelStarted = false;
};
