// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/DualFireUIPlayerController.h"
#include "Core/DualFireTypes.h"
#include "DualFireMissionPlayerController.generated.h"

class UDualFireMissionResultWidget;

UCLASS()
class DUALFIRE_API ADualFireMissionPlayerController : public ADualFireUIPlayerController
{
	GENERATED_BODY()

public:
	ADualFireMissionPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mission Result")
	TSoftClassPtr<UDualFireMissionResultWidget> MissionResultWidgetClass;

private:
	UFUNCTION()
	void HandleMissionEnded(EMissionResult Result);
};
