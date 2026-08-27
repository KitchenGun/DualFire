// Copyright DualFire. All Rights Reserved.

#include "UI/DualFireMissionFlowWidgets.h"

#include "CommonTextBlock.h"
#include "DualFire.h"
#include "Engine/DataTable.h"
#include "GameInstance/DualFireGameInstance.h"
#include "GameInstance/DualFireMissionFlowSubsystem.h"
#include "UI/DualFireMenuButton.h"

namespace
{
FText FormatEnvironmentTags(const TArray<FName>& Tags)
{
	TArray<FString> Names;
	Names.Reserve(Tags.Num());
	for (const FName Tag : Tags)
	{
		Names.Add(Tag.ToString());
	}
	return FText::FromString(FString::Join(Names, TEXT("  /  ")));
}
}

void UDualFireCampaignMapWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!ensure(IsValid(MissionButton) && IsValid(BackButton)))
	{
		return;
	}
	MissionButton->OnClicked().AddUObject(this, &ThisClass::SelectMission);
	BackButton->OnClicked().AddUObject(this, &ThisClass::BackToTitle);
	BackButton->SetLabelText(NSLOCTEXT("DualFireUI", "CampaignBack", "BACK"));
}

void UDualFireCampaignMapWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	RefreshMissionDetails();
}

UWidget* UDualFireCampaignMapWidget::NativeGetDesiredFocusTarget() const
{
	return MissionButton;
}

bool UDualFireCampaignMapWidget::NativeOnHandleBackAction()
{
	BackToTitle();
	return true;
}

void UDualFireCampaignMapWidget::SelectMission()
{
	UDualFireMissionFlowSubsystem* Flow = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDualFireMissionFlowSubsystem>()
		: nullptr;
	FText Error;
	FName InvalidField;
	if (!IsValid(Flow) || !Flow->TryBeginPreparation(MissionID, DifficultyID, Error, InvalidField))
	{
		if (IsValid(Text_Status))
		{
			Text_Status->SetText(Error.IsEmpty() ? FText::FromString(TEXT("MISSION DATA IS UNAVAILABLE.")) : Error);
		}
		UE_LOG(LogDualFire, Error, TEXT("[Campaign] 미션 선택 실패 — Field:%s Error:%s"),
			*InvalidField.ToString(), *Error.ToString());
		return;
	}

	if (!IsValid(PushScreenToLayer(EDualFireUILayer::Menu, BriefingWidgetClass)))
	{
		UE_LOG(LogDualFire, Error, TEXT("[Campaign] 브리핑 화면 열기 실패"));
	}
}

void UDualFireCampaignMapWidget::BackToTitle()
{
	DeactivateWidget();
}

void UDualFireCampaignMapWidget::RefreshMissionDetails()
{
	const UDualFireGameInstance* GI = Cast<UDualFireGameInstance>(GetGameInstance());
	const UDataTable* Table = IsValid(GI) ? GI->GetMissionDataTable() : nullptr;
	const FMissionRow* Row = IsValid(Table)
		? Table->FindRow<FMissionRow>(MissionID, TEXT("Campaign.RefreshMissionDetails"), false)
		: nullptr;
	if (!Row)
	{
		if (IsValid(Text_Status))
		{
			Text_Status->SetText(FText::FromString(TEXT("MISSION DATA IS UNAVAILABLE.")));
		}
		return;
	}

	MissionButton->SetLabelText(Row->MissionCode);
	if (IsValid(Text_MissionName)) Text_MissionName->SetText(Row->DisplayName);
	if (IsValid(Text_Environment)) Text_Environment->SetText(FormatEnvironmentTags(Row->EnvironmentTags));
	if (IsValid(Text_Difficulty)) Text_Difficulty->SetText(FText::FromName(Row->NormalDifficultyID));
	if (IsValid(Text_Status)) Text_Status->SetText(FText::GetEmpty());
}

void UDualFireMissionBriefingWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!ensure(IsValid(ContinueButton) && IsValid(SkipButton) && IsValid(BackButton)))
	{
		return;
	}
	ContinueButton->OnClicked().AddUObject(this, &ThisClass::ContinueToHangar);
	SkipButton->OnClicked().AddUObject(this, &ThisClass::ContinueToHangar);
	BackButton->OnClicked().AddUObject(this, &ThisClass::BackToCampaign);
	ContinueButton->SetLabelText(NSLOCTEXT("DualFireUI", "BriefingContinue", "CONTINUE"));
	SkipButton->SetLabelText(NSLOCTEXT("DualFireUI", "BriefingSkip", "SKIP"));
	BackButton->SetLabelText(NSLOCTEXT("DualFireUI", "BriefingBack", "BACK"));
}

void UDualFireMissionBriefingWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	RefreshBriefing();
}

UWidget* UDualFireMissionBriefingWidget::NativeGetDesiredFocusTarget() const
{
	return ContinueButton;
}

bool UDualFireMissionBriefingWidget::NativeOnHandleBackAction()
{
	BackToCampaign();
	return true;
}

void UDualFireMissionBriefingWidget::ContinueToHangar()
{
	if (!IsValid(PushScreenToLayer(EDualFireUILayer::Menu, HangarWidgetClass)))
	{
		if (IsValid(Text_Status)) Text_Status->SetText(FText::FromString(TEXT("HANGAR IS UNAVAILABLE.")));
		UE_LOG(LogDualFire, Error, TEXT("[Briefing] 격납고 화면 열기 실패"));
	}
}

void UDualFireMissionBriefingWidget::BackToCampaign()
{
	DeactivateWidget();
}

void UDualFireMissionBriefingWidget::RefreshBriefing()
{
	const UDualFireMissionFlowSubsystem* Flow = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDualFireMissionFlowSubsystem>()
		: nullptr;
	if (!IsValid(Flow) || !Flow->HasPreparationContext())
	{
		if (IsValid(Text_Status)) Text_Status->SetText(FText::FromString(TEXT("MISSION CONTEXT IS UNAVAILABLE.")));
		return;
	}

	const FMissionPreparationContext Context = Flow->GetPreparationContext();
	if (IsValid(Text_MissionName)) Text_MissionName->SetText(FText::Format(
		NSLOCTEXT("DualFireUI", "BriefingMissionName", "{0}  //  {1}"),
		Context.MissionRow.MissionCode, Context.MissionRow.DisplayName));
	if (IsValid(Text_Environment)) Text_Environment->SetText(FormatEnvironmentTags(Context.MissionRow.EnvironmentTags));
	if (IsValid(Text_Difficulty)) Text_Difficulty->SetText(FText::FromName(Context.DifficultyID));
	if (IsValid(Text_Briefing)) Text_Briefing->SetText(Context.MissionRow.BriefingText);
	if (IsValid(Text_Objective)) Text_Objective->SetText(Context.MissionRow.ObjectiveText);
	if (IsValid(Text_EnemyHint)) Text_EnemyHint->SetText(Context.MissionRow.EnemyCompositionHint);
	if (IsValid(Text_Status)) Text_Status->SetText(FText::GetEmpty());
}
