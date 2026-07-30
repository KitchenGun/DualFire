// Copyright DualFire. All Rights Reserved.

#include "GameInstance/DualFireMissionResultSubsystem.h"

#include "DualFire.h"

void UDualFireMissionResultSubsystem::StoreResult(
	const FDualFireMissionResultData& InResultData)
{
	PendingResult = InResultData;
	bHasPendingResult = true;
	UE_LOG(LogDualFire, Log, TEXT("[MissionResult] 결과 데이터 저장: %s"),
		*UEnum::GetValueAsString(InResultData.Result));
}

void UDualFireMissionResultSubsystem::ClearPendingResult()
{
	PendingResult = FDualFireMissionResultData();
	bHasPendingResult = false;
	UE_LOG(LogDualFire, Log, TEXT("[MissionResult] 결과 데이터 초기화"));
}
