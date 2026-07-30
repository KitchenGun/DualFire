// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/DualFireUIPlayerController.h"
#include "DualFireResultPlayerController.generated.h"

class UDualFireMissionResultWidget;
class UCommonActivatableWidget;

/** 결과 레벨의 Common UI 루트와 결과 화면을 생성한다. */
UCLASS()
class DUALFIRE_API ADualFireResultPlayerController : public ADualFireUIPlayerController
{
	GENERATED_BODY()

public:
	ADualFireResultPlayerController();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mission Result")
	TSoftClassPtr<UDualFireMissionResultWidget> MissionResultWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mission Result")
	TSoftClassPtr<UCommonActivatableWidget> MissionResultBackgroundClass;

private:
	void OpenLobbyWhenResultIsMissing();
};
