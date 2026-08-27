// Copyright DualFire. All Rights Reserved.

#include "GameInstance/DualFireMissionFlowSubsystem.h"

#include "DualFire.h"
#include "Engine/DataTable.h"
#include "GameInstance/DualFireGameInstance.h"
#include "Loadout/LoadoutManagerSubsystem.h"

namespace
{
void SetFlowError(FText& OutError, FName& OutInvalidField, const FName Field, const FText& Error)
{
	OutInvalidField = Field;
	OutError = Error;
}
}

bool UDualFireMissionFlowSubsystem::TryBeginPreparation(
	const FName MissionID,
	const FName DifficultyID,
	FText& OutError,
	FName& OutInvalidField)
{
	OutError = FText::GetEmpty();
	OutInvalidField = NAME_None;

	const FMissionRow* MissionRow = FindMissionRow(MissionID);
	if (!MissionRow)
	{
		SetFlowError(OutError, OutInvalidField, TEXT("MissionID"),
			FText::FromString(FString::Printf(TEXT("MissionID '%s' 행을 찾을 수 없습니다."), *MissionID.ToString())));
		return false;
	}
	if (DifficultyID.IsNone() || MissionRow->NormalDifficultyID != DifficultyID)
	{
		SetFlowError(OutError, OutInvalidField, TEXT("DifficultyID"),
			FText::FromString(FString::Printf(TEXT("MissionID '%s'는 난이도 '%s'를 지원하지 않습니다."),
				*MissionID.ToString(), *DifficultyID.ToString())));
		return false;
	}
	if (MissionRow->MissionLevel.IsNull())
	{
		SetFlowError(OutError, OutInvalidField, TEXT("MissionLevel"),
			FText::FromString(TEXT("미션 레벨이 지정되지 않았습니다.")));
		return false;
	}
	if (!ValidateStageReference(*MissionRow, OutError, OutInvalidField))
	{
		return false;
	}

	FMissionPreparationContext NewContext;
	NewContext.MissionRow = *MissionRow;
	NewContext.MissionID = MissionRow->MissionID;
	NewContext.DifficultyID = DifficultyID;
	NewContext.StageID = MissionRow->StageID;
	NewContext.MissionLevel = MissionRow->MissionLevel;

	PreparationContext = MoveTemp(NewContext);
	LaunchContext = FMissionLaunchContext();
	bHasPreparationContext = true;
	bHasLaunchContext = false;
	ClearPendingResult();
	return true;
}

bool UDualFireMissionFlowSubsystem::TryFinalizeLaunch(
	const FLoadout& Loadout,
	FText& OutError,
	FName& OutInvalidField)
{
	OutError = FText::GetEmpty();
	OutInvalidField = NAME_None;
	if (!bHasPreparationContext || !PreparationContext.IsValid())
	{
		SetFlowError(OutError, OutInvalidField, TEXT("PreparationContext"),
			FText::FromString(TEXT("먼저 유효한 미션 준비 컨텍스트를 선택해야 합니다.")));
		return false;
	}

	ULoadoutManagerSubsystem* LoadoutManager = GetGameInstance()
		? GetGameInstance()->GetSubsystem<ULoadoutManagerSubsystem>()
		: nullptr;
	if (!IsValid(LoadoutManager))
	{
		SetFlowError(OutError, OutInvalidField, TEXT("LoadoutManager"),
			FText::FromString(TEXT("로드아웃 관리자를 사용할 수 없습니다.")));
		return false;
	}
	if (!LoadoutManager->TrySetActiveLoadout(Loadout, OutError, OutInvalidField))
	{
		return false;
	}

	FMissionLaunchContext NewContext;
	NewContext.Preparation = PreparationContext;
	NewContext.Loadout = Loadout;
	LaunchContext = MoveTemp(NewContext);
	bHasLaunchContext = true;
	return true;
}

bool UDualFireMissionFlowSubsystem::PrepareReplay(FText& OutError)
{
	OutError = FText::GetEmpty();
	if (!bHasLaunchContext || !LaunchContext.Preparation.IsValid())
	{
		OutError = FText::FromString(TEXT("재생할 미션 컨텍스트가 없습니다."));
		return false;
	}

	PreparationContext = LaunchContext.Preparation;
	LaunchContext = FMissionLaunchContext();
	bHasPreparationContext = true;
	bHasLaunchContext = false;
	ClearPendingResult();
	PendingStartRoute = EDualFireStartRoute::Briefing;
	return true;
}

void UDualFireMissionFlowSubsystem::ReturnToMissionSelect()
{
	PreparationContext = FMissionPreparationContext();
	LaunchContext = FMissionLaunchContext();
	bHasPreparationContext = false;
	bHasLaunchContext = false;
	ClearPendingResult();
	PendingStartRoute = EDualFireStartRoute::Campaign;
}

void UDualFireMissionFlowSubsystem::StoreResult(const FDualFireMissionResultData& InResultData)
{
	PendingResult = InResultData;
	bHasPendingResult = true;
	UE_LOG(LogDualFire, Log, TEXT("[MissionFlow] 결과 저장: %s"), *UEnum::GetValueAsString(InResultData.Result));
}

void UDualFireMissionFlowSubsystem::ClearPendingResult()
{
	PendingResult = FDualFireMissionResultData();
	bHasPendingResult = false;
}

EDualFireStartRoute UDualFireMissionFlowSubsystem::ConsumeStartRoute()
{
	const EDualFireStartRoute Route = PendingStartRoute;
	PendingStartRoute = EDualFireStartRoute::None;
	return Route;
}

const FMissionRow* UDualFireMissionFlowSubsystem::FindMissionRow(const FName MissionID) const
{
	const UDualFireGameInstance* DualFireGI = Cast<UDualFireGameInstance>(GetGameInstance());
	const UDataTable* MissionTable = IsValid(DualFireGI) ? DualFireGI->GetMissionDataTable() : nullptr;
	if (!IsValid(MissionTable) || MissionID.IsNone())
	{
		return nullptr;
	}
	if (const FMissionRow* DirectRow = MissionTable->FindRow<FMissionRow>(MissionID, TEXT("MissionFlow.FindMissionRow"), false))
	{
		return DirectRow;
	}

	TArray<FMissionRow*> Rows;
	MissionTable->GetAllRows<FMissionRow>(TEXT("MissionFlow.FindMissionRow"), Rows);
	if (FMissionRow* const* FoundRow = Rows.FindByPredicate([MissionID](const FMissionRow* Row)
	{
		return Row && Row->MissionID == MissionID;
	}))
	{
		return *FoundRow;
	}
	return nullptr;
}

bool UDualFireMissionFlowSubsystem::ValidateStageReference(
	const FMissionRow& MissionRow,
	FText& OutError,
	FName& OutInvalidField) const
{
	const UDualFireGameInstance* DualFireGI = Cast<UDualFireGameInstance>(GetGameInstance());
	const UDataTable* StageTable = IsValid(DualFireGI) ? DualFireGI->GetStageDataTable() : nullptr;
	if (!IsValid(StageTable))
	{
		SetFlowError(OutError, OutInvalidField, TEXT("StageDataTable"),
			FText::FromString(TEXT("Stage DataTable이 지정되지 않았습니다.")));
		return false;
	}
	if (MissionRow.StageID.IsNone() ||
		!StageTable->FindRow<FStageRow>(MissionRow.StageID, TEXT("MissionFlow.ValidateStageReference"), false))
	{
		SetFlowError(OutError, OutInvalidField, TEXT("StageID"),
			FText::FromString(FString::Printf(TEXT("StageID '%s' 행을 찾을 수 없습니다."), *MissionRow.StageID.ToString())));
		return false;
	}
	return true;
}
