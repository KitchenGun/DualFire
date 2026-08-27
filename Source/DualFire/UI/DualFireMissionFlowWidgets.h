// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/DualFireMenuScreenWidget.h"
#include "DualFireMissionFlowWidgets.generated.h"

class UCommonTextBlock;
class UDualFireMenuButton;

UCLASS(BlueprintType, Blueprintable)
class DUALFIRE_API UDualFireCampaignMapWidget : public UDualFireMenuScreenWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Campaign")
	void SelectMission();

	UFUNCTION(BlueprintCallable, Category="Campaign")
	void BackToTitle();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnActivated() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual bool NativeOnHandleBackAction() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Campaign")
	FName MissionID = TEXT("MISSION_01");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Campaign")
	FName DifficultyID = TEXT("NORMAL");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Campaign")
	TSubclassOf<UCommonActivatableWidget> BriefingWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category="Campaign|Widgets", meta=(BindWidget))
	TObjectPtr<UDualFireMenuButton> MissionButton;

	UPROPERTY(BlueprintReadOnly, Category="Campaign|Widgets", meta=(BindWidget))
	TObjectPtr<UDualFireMenuButton> BackButton;

	UPROPERTY(BlueprintReadOnly, Category="Campaign|Widgets", meta=(BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> Text_MissionName;

	UPROPERTY(BlueprintReadOnly, Category="Campaign|Widgets", meta=(BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> Text_Environment;

	UPROPERTY(BlueprintReadOnly, Category="Campaign|Widgets", meta=(BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> Text_Difficulty;

	UPROPERTY(BlueprintReadOnly, Category="Campaign|Widgets", meta=(BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> Text_Status;

private:
	void RefreshMissionDetails();
};

UCLASS(BlueprintType, Blueprintable)
class DUALFIRE_API UDualFireMissionBriefingWidget : public UDualFireMenuScreenWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Briefing")
	void ContinueToHangar();

	UFUNCTION(BlueprintCallable, Category="Briefing")
	void BackToCampaign();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnActivated() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual bool NativeOnHandleBackAction() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Briefing")
	TSubclassOf<UCommonActivatableWidget> HangarWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category="Briefing|Widgets", meta=(BindWidget))
	TObjectPtr<UDualFireMenuButton> ContinueButton;

	UPROPERTY(BlueprintReadOnly, Category="Briefing|Widgets", meta=(BindWidget))
	TObjectPtr<UDualFireMenuButton> SkipButton;

	UPROPERTY(BlueprintReadOnly, Category="Briefing|Widgets", meta=(BindWidget))
	TObjectPtr<UDualFireMenuButton> BackButton;

	UPROPERTY(BlueprintReadOnly, Category="Briefing|Widgets", meta=(BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> Text_MissionName;

	UPROPERTY(BlueprintReadOnly, Category="Briefing|Widgets", meta=(BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> Text_Environment;

	UPROPERTY(BlueprintReadOnly, Category="Briefing|Widgets", meta=(BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> Text_Difficulty;

	UPROPERTY(BlueprintReadOnly, Category="Briefing|Widgets", meta=(BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> Text_Briefing;

	UPROPERTY(BlueprintReadOnly, Category="Briefing|Widgets", meta=(BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> Text_Objective;

	UPROPERTY(BlueprintReadOnly, Category="Briefing|Widgets", meta=(BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> Text_EnemyHint;

	UPROPERTY(BlueprintReadOnly, Category="Briefing|Widgets", meta=(BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> Text_Status;

private:
	void RefreshBriefing();
};
