// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/DualFireDataTypes.h"
#include "DualFireMissionFlowTypes.generated.h"

USTRUCT(BlueprintType)
struct DUALFIRE_API FMissionPreparationContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Mission Flow")
	FMissionRow MissionRow;

	UPROPERTY(BlueprintReadOnly, Category="Mission Flow")
	FName MissionID = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category="Mission Flow")
	FName DifficultyID = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category="Mission Flow")
	FName StageID = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category="Mission Flow")
	TSoftObjectPtr<UWorld> MissionLevel;

	bool IsValid() const
	{
		return !MissionID.IsNone() && !DifficultyID.IsNone() &&
			!StageID.IsNone() && !MissionLevel.IsNull();
	}
};

USTRUCT(BlueprintType)
struct DUALFIRE_API FMissionLaunchContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Mission Flow")
	FMissionPreparationContext Preparation;

	UPROPERTY(BlueprintReadOnly, Category="Mission Flow")
	FLoadout Loadout;

	bool IsValid() const
	{
		return Preparation.IsValid() && Loadout.IsComplete();
	}
};
