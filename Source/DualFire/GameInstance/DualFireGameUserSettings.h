// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/DualFireTypes.h"
#include "GameFramework/GameUserSettings.h"
#include "DualFireGameUserSettings.generated.h"

/** Persistent player preferences that are independent of a mission session. */
UCLASS(Config=GameUserSettings)
class DUALFIRE_API UDualFireGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="Input")
	ESlowInputMode GetSlowInputMode() const { return SlowInputMode; }

	UFUNCTION(BlueprintCallable, Category="Input")
	void SetSlowInputMode(ESlowInputMode InSlowInputMode);

private:
	UPROPERTY(Config)
	ESlowInputMode SlowInputMode = ESlowInputMode::Hold;
};
