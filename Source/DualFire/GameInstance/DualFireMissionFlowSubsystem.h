// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/DualFireMissionFlowTypes.h"
#include "Core/DualFireMissionResultTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DualFireMissionFlowSubsystem.generated.h"

UCLASS()
class DUALFIRE_API UDualFireMissionFlowSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Mission Flow")
	bool TryBeginPreparation(FName MissionID, FName DifficultyID, FText& OutError, FName& OutInvalidField);

	UFUNCTION(BlueprintCallable, Category="Mission Flow")
	bool TryFinalizeLaunch(const FLoadout& Loadout, FText& OutError, FName& OutInvalidField);

	UFUNCTION(BlueprintCallable, Category="Mission Flow")
	bool PrepareReplay(FText& OutError);

	UFUNCTION(BlueprintCallable, Category="Mission Flow")
	void ReturnToMissionSelect();

	UFUNCTION(BlueprintCallable, Category="Mission Flow")
	void StoreResult(const FDualFireMissionResultData& InResultData);

	UFUNCTION(BlueprintPure, Category="Mission Flow")
	bool HasPendingResult() const { return bHasPendingResult; }

	UFUNCTION(BlueprintPure, Category="Mission Flow")
	FDualFireMissionResultData GetPendingResult() const { return PendingResult; }

	UFUNCTION(BlueprintCallable, Category="Mission Flow")
	void ClearPendingResult();

	UFUNCTION(BlueprintPure, Category="Mission Flow")
	bool HasPreparationContext() const { return bHasPreparationContext; }

	UFUNCTION(BlueprintPure, Category="Mission Flow")
	bool HasLaunchContext() const { return bHasLaunchContext; }

	UFUNCTION(BlueprintPure, Category="Mission Flow")
	FMissionPreparationContext GetPreparationContext() const { return PreparationContext; }

	UFUNCTION(BlueprintPure, Category="Mission Flow")
	FMissionLaunchContext GetLaunchContext() const { return LaunchContext; }

	UFUNCTION(BlueprintCallable, Category="Mission Flow")
	EDualFireStartRoute ConsumeStartRoute();

private:
	const FMissionRow* FindMissionRow(FName MissionID) const;
	bool ValidateStageReference(const FMissionRow& MissionRow, FText& OutError, FName& OutInvalidField) const;

	UPROPERTY()
	FMissionPreparationContext PreparationContext;

	UPROPERTY()
	FMissionLaunchContext LaunchContext;

	UPROPERTY()
	FDualFireMissionResultData PendingResult;

	bool bHasPreparationContext = false;
	bool bHasLaunchContext = false;
	bool bHasPendingResult = false;
	EDualFireStartRoute PendingStartRoute = EDualFireStartRoute::None;
};
