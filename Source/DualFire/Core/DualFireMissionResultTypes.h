// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/DualFireTypes.h"
#include "DualFireMissionResultTypes.generated.h"

UENUM(BlueprintType)
enum class EDualFireMissionRank : uint8
{
	S,
	A,
	B,
	C,
	D,
	NotApplicable UMETA(DisplayName="N/A"),
};

UENUM(BlueprintType)
enum class EDualFireMissionMetric : uint8
{
	Score,
	AirTargets,
	GroundTargets,
	HitsTaken,
	Deaths,
};

USTRUCT(BlueprintType)
struct DUALFIRE_API FDualFireMissionMetricResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Mission Result")
	EDualFireMissionMetric Metric = EDualFireMissionMetric::Score;

	UPROPERTY(BlueprintReadOnly, Category="Mission Result")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category="Mission Result")
	FText DisplayValue;

	UPROPERTY(BlueprintReadOnly, Category="Mission Result")
	float Progress = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Mission Result")
	EDualFireMissionRank Rank = EDualFireMissionRank::NotApplicable;

	UPROPERTY(BlueprintReadOnly, Category="Mission Result")
	bool bApplicable = false;
};

USTRUCT(BlueprintType)
struct DUALFIRE_API FDualFireMissionResultData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Mission Result")
	EMissionResult Result = EMissionResult::None;

	UPROPERTY(BlueprintReadOnly, Category="Mission Result")
	EDualFireMissionFailureReason FailureReason = EDualFireMissionFailureReason::None;

	UPROPERTY(BlueprintReadOnly, Category="Mission Result")
	FName StageID = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category="Mission Result")
	FText MissionCode;

	UPROPERTY(BlueprintReadOnly, Category="Mission Result")
	FText MissionName;

	UPROPERTY(BlueprintReadOnly, Category="Mission Result")
	float ElapsedTime = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Mission Result")
	FText Difficulty;

	UPROPERTY(BlueprintReadOnly, Category="Mission Result")
	TArray<FDualFireMissionMetricResult> Metrics;

	UPROPERTY(BlueprintReadOnly, Category="Mission Result")
	EDualFireMissionRank OverallRank = EDualFireMissionRank::NotApplicable;
};
