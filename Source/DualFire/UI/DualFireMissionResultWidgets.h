// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "Core/DualFireMissionResultTypes.h"
#include "UI/DualFireMenuScreenWidget.h"
#include "DualFireMissionResultWidgets.generated.h"

class UCommonTextBlock;
class UImage;
class UProgressBar;
class UTexture2D;
class UVerticalBox;
class UDualFireMenuButton;

UCLASS(BlueprintType, Blueprintable)
class DUALFIRE_API UDualFireMissionResultMetricRowWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	void SetMetricResult(const FDualFireMissionMetricResult& MetricResult, UTexture2D* RankTexture);

protected:
	UPROPERTY(BlueprintReadOnly, Category="Mission Result", meta=(BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> Text_MetricName;

	UPROPERTY(BlueprintReadOnly, Category="Mission Result", meta=(BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> Text_MetricValue;

	UPROPERTY(BlueprintReadOnly, Category="Mission Result", meta=(BindWidgetOptional))
	TObjectPtr<UProgressBar> Progress_Metric;

	UPROPERTY(BlueprintReadOnly, Category="Mission Result", meta=(BindWidgetOptional))
	TObjectPtr<UImage> Image_Rank;

	UPROPERTY(BlueprintReadOnly, Category="Mission Result", meta=(BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> Text_RankFallback;
};

UCLASS(BlueprintType, Blueprintable)
class DUALFIRE_API UDualFireMissionResultWidget : public UDualFireMenuScreenWidget
{
	GENERATED_BODY()

public:
	UDualFireMissionResultWidget();

	void SetMissionResultData(const FDualFireMissionResultData& InResultData);

	UFUNCTION(BlueprintCallable, Category="Mission Result")
	void ReturnToMissionSelect();

	UFUNCTION(BlueprintCallable, Category="Mission Result")
	void ReplayMission();

protected:
	virtual void NativeOnInitialized() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual bool NativeOnHandleBackAction() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mission Result|Widgets")
	TSubclassOf<UDualFireMissionResultMetricRowWidget> MetricRowClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mission Result|Rank")
	TObjectPtr<UTexture2D> RankTextureS;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mission Result|Rank")
	TObjectPtr<UTexture2D> RankTextureA;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mission Result|Rank")
	TObjectPtr<UTexture2D> RankTextureB;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mission Result|Rank")
	TObjectPtr<UTexture2D> RankTextureC;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mission Result|Rank")
	TObjectPtr<UTexture2D> RankTextureD;

	UPROPERTY(BlueprintReadOnly, Category="Mission Result", meta=(BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> Text_ResultTitle;

	UPROPERTY(BlueprintReadOnly, Category="Mission Result", meta=(BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> Text_Mission;

	UPROPERTY(BlueprintReadOnly, Category="Mission Result", meta=(BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> Text_ClearTime;

	UPROPERTY(BlueprintReadOnly, Category="Mission Result", meta=(BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> Text_Difficulty;

	UPROPERTY(BlueprintReadOnly, Category="Mission Result", meta=(BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> Text_FailureReason;

	UPROPERTY(BlueprintReadOnly, Category="Mission Result", meta=(BindWidgetOptional))
	TObjectPtr<UDualFireMenuButton> MissionSelectButton;

	UPROPERTY(BlueprintReadOnly, Category="Mission Result", meta=(BindWidgetOptional))
	TObjectPtr<UDualFireMenuButton> ReplayButton;

	UPROPERTY(BlueprintReadOnly, Category="Mission Result", meta=(BindWidgetOptional))
	TObjectPtr<UVerticalBox> MetricsBox;

	UPROPERTY(BlueprintReadOnly, Category="Mission Result", meta=(BindWidgetOptional))
	TObjectPtr<UWidget> RankPanel;

	UPROPERTY(BlueprintReadOnly, Category="Mission Result", meta=(BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> Text_RankLabel;

	UPROPERTY(BlueprintReadOnly, Category="Mission Result", meta=(BindWidgetOptional))
	TObjectPtr<UImage> Image_OverallRank;

	UPROPERTY(BlueprintReadOnly, Category="Mission Result", meta=(BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> Text_OverallRankFallback;

	/** WBP에 남아 있는 미구현 해금 영역. 해금 기능 추가 전까지 숨긴다. */
	UPROPERTY(BlueprintReadOnly, Category="Mission Result", meta=(BindWidgetOptional))
	TObjectPtr<UWidget> UnlockPanel;

private:
	friend class UDualFireMissionResultMetricRowWidget;

	void RefreshResultView();
	UTexture2D* GetRankTexture(EDualFireMissionRank Rank) const;
	static FText GetRankText(EDualFireMissionRank Rank);
	static FText FormatElapsedTime(float ElapsedTime);

	FDualFireMissionResultData ResultData;
	bool bTravelStarted = false;
};
