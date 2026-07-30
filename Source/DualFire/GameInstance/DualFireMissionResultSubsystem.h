// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/DualFireMissionResultTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DualFireMissionResultSubsystem.generated.h"

/** 레벨 전환 동안 미션 결과 데이터를 보관한다. */
UCLASS()
class DUALFIRE_API UDualFireMissionResultSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Mission Result")
	void StoreResult(const FDualFireMissionResultData& InResultData);

	UFUNCTION(BlueprintPure, Category="Mission Result")
	bool HasPendingResult() const { return bHasPendingResult; }

	UFUNCTION(BlueprintPure, Category="Mission Result")
	FDualFireMissionResultData GetPendingResult() const { return PendingResult; }

	UFUNCTION(BlueprintCallable, Category="Mission Result")
	void ClearPendingResult();

private:
	UPROPERTY()
	FDualFireMissionResultData PendingResult;

	bool bHasPendingResult = false;
};
