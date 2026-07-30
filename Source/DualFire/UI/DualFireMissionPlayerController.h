// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Core/DualFireTypes.h"
#include "DualFireMissionPlayerController.generated.h"

UCLASS()
class DUALFIRE_API ADualFireMissionPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mission Result")
	FName ResultLevelName = TEXT("/Game/Level/LV_Result");

private:
	UFUNCTION()
	void HandleMissionEnded(EMissionResult Result);

	bool bResultTravelStarted = false;
};
