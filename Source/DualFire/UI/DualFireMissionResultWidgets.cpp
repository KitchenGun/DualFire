// Copyright DualFire. All Rights Reserved.

#include "UI/DualFireMissionResultWidgets.h"

#include "CommonTextBlock.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/VerticalBox.h"
#include "Components/Widget.h"
#include "Engine/GameInstance.h"
#include "GameInstance/DualFireMissionResultSubsystem.h"
#include "Input/CommonUIInputTypes.h"
#include "InputAction.h"
#include "Kismet/GameplayStatics.h"

void UDualFireMissionResultMetricRowWidget::SetMetricResult(
	const FDualFireMissionMetricResult& MetricResult,
	UTexture2D* RankTexture)
{
	if (IsValid(Text_MetricName))
	{
		Text_MetricName->SetText(MetricResult.DisplayName);
	}
	if (IsValid(Text_MetricValue))
	{
		Text_MetricValue->SetText(MetricResult.DisplayValue);
	}
	if (IsValid(Progress_Metric))
	{
		Progress_Metric->SetPercent(MetricResult.Progress);
		Progress_Metric->SetVisibility(
			MetricResult.bApplicable ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (IsValid(Image_Rank))
	{
		Image_Rank->SetBrushFromTexture(RankTexture, true);
		Image_Rank->SetVisibility(
			IsValid(RankTexture) ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (IsValid(Text_RankFallback))
	{
		Text_RankFallback->SetText(UDualFireMissionResultWidget::GetRankText(MetricResult.Rank));
		Text_RankFallback->SetVisibility(
			!IsValid(RankTexture) ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

UDualFireMissionResultWidget::UDualFireMissionResultWidget()
{
	bIsBackHandler = true;
	bIsBackActionDisplayedInActionBar = false;
}

TOptional<FUIInputConfig> UDualFireMissionResultWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

void UDualFireMissionResultWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 배경은 Game 레이어에서 화면을 채우고, 이 위젯은 16:9 정보 배치만 담당한다.
	if (UWidget* EmbeddedBackground = GetWidgetFromName(TEXT("ResultBackground")))
	{
		EmbeddedBackground->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (IsValid(ConfirmInputAction))
	{
		FBindUIActionArgs BindArgs(
			ConfirmInputAction,
			true,
			FSimpleDelegate::CreateUObject(this, &ThisClass::ContinueToLobby));
		BindArgs.OverrideDisplayName = NSLOCTEXT(
			"DualFireUI", "ContinueToLobbyAction", "CONTINUE TO LOBBY");
		BindArgs.bConsumeInput = true;
		RegisterUIActionBinding(BindArgs);
	}

	RefreshResultView();
}

bool UDualFireMissionResultWidget::NativeOnHandleBackAction()
{
	// 결과 화면에서는 Back으로 전투 화면이 다시 노출되지 않게 입력만 소비한다.
	return true;
}

void UDualFireMissionResultWidget::SetMissionResultData(
	const FDualFireMissionResultData& InResultData)
{
	ResultData = InResultData;
	bHasResultData = true;
	RefreshResultView();
}

void UDualFireMissionResultWidget::RefreshResultView()
{
	if (!bHasResultData)
	{
		return;
	}

	const bool bCleared = ResultData.Result == EMissionResult::Cleared;
	if (IsValid(Text_ResultTitle))
	{
		Text_ResultTitle->SetText(bCleared
			? NSLOCTEXT("DualFireUI", "MissionCleared", "MISSION COMPLETE")
			: NSLOCTEXT("DualFireUI", "MissionFailed", "MISSION FAILED"));
	}
	if (IsValid(Text_Mission))
	{
		Text_Mission->SetText(FText::Format(
			NSLOCTEXT("DualFireUI", "MissionResultName", "{0}  //  {1}"),
			ResultData.MissionCode,
			ResultData.MissionName));
	}
	if (IsValid(Text_ClearTime))
	{
		Text_ClearTime->SetText(FText::Format(
			NSLOCTEXT("DualFireUI", "ClearTime", "TIME  {0}"),
			FormatElapsedTime(ResultData.ElapsedTime)));
	}
	if (IsValid(Text_Difficulty))
	{
		Text_Difficulty->SetText(FText::Format(
			NSLOCTEXT("DualFireUI", "Difficulty", "DIFFICULTY  {0}"),
			ResultData.Difficulty));
	}

	if (IsValid(MetricsBox))
	{
		MetricsBox->ClearChildren();
		MetricsBox->SetVisibility(bCleared ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);

		if (bCleared && IsValid(MetricRowClass))
		{
			for (const FDualFireMissionMetricResult& Metric : ResultData.Metrics)
			{
				UDualFireMissionResultMetricRowWidget* Row =
					CreateWidget<UDualFireMissionResultMetricRowWidget>(this, MetricRowClass);
				if (IsValid(Row))
				{
					Row->SetMetricResult(Metric, GetRankTexture(Metric.Rank));
					MetricsBox->AddChildToVerticalBox(Row);
				}
			}
		}
	}

	if (IsValid(RankPanel))
	{
		RankPanel->SetVisibility(bCleared ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (IsValid(Text_RankLabel))
	{
		Text_RankLabel->SetVisibility(
			bCleared ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	UTexture2D* OverallRankTexture = GetRankTexture(ResultData.OverallRank);
	if (IsValid(Image_OverallRank))
	{
		Image_OverallRank->SetBrushFromTexture(OverallRankTexture, true);
		Image_OverallRank->SetVisibility(
			bCleared && IsValid(OverallRankTexture)
				? ESlateVisibility::HitTestInvisible
				: ESlateVisibility::Collapsed);
	}
	if (IsValid(Text_OverallRankFallback))
	{
		Text_OverallRankFallback->SetText(GetRankText(ResultData.OverallRank));
		Text_OverallRankFallback->SetVisibility(
			bCleared && !IsValid(OverallRankTexture)
				? ESlateVisibility::HitTestInvisible
				: ESlateVisibility::Collapsed);
	}

	const bool bShowUnlock = bCleared && ResultData.bHasUnlock;
	if (IsValid(UnlockPanel))
	{
		UnlockPanel->SetVisibility(bShowUnlock
			? ESlateVisibility::SelfHitTestInvisible
			: ESlateVisibility::Collapsed);
	}
	if (IsValid(Text_Unlock))
	{
		Text_Unlock->SetText(ResultData.UnlockText);
	}
}

void UDualFireMissionResultWidget::ContinueToLobby()
{
	if (bLobbyTravelStarted)
	{
		return;
	}

	bLobbyTravelStarted = true;
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UDualFireMissionResultSubsystem* ResultSubsystem =
			GameInstance->GetSubsystem<UDualFireMissionResultSubsystem>())
		{
			ResultSubsystem->ClearPendingResult();
		}
	}
	UGameplayStatics::OpenLevel(this, FName(TEXT("/Game/Level/LV_Start")));
}

UTexture2D* UDualFireMissionResultWidget::GetRankTexture(EDualFireMissionRank Rank) const
{
	switch (Rank)
	{
	case EDualFireMissionRank::S: return RankTextureS;
	case EDualFireMissionRank::A: return RankTextureA;
	case EDualFireMissionRank::B: return RankTextureB;
	case EDualFireMissionRank::C: return RankTextureC;
	case EDualFireMissionRank::D: return RankTextureD;
	default: return nullptr;
	}
}

FText UDualFireMissionResultWidget::GetRankText(EDualFireMissionRank Rank)
{
	switch (Rank)
	{
	case EDualFireMissionRank::S: return FText::FromString(TEXT("S"));
	case EDualFireMissionRank::A: return FText::FromString(TEXT("A"));
	case EDualFireMissionRank::B: return FText::FromString(TEXT("B"));
	case EDualFireMissionRank::C: return FText::FromString(TEXT("C"));
	case EDualFireMissionRank::D: return FText::FromString(TEXT("D"));
	default: return FText::FromString(TEXT("N/A"));
	}
}

FText UDualFireMissionResultWidget::FormatElapsedTime(float ElapsedTime)
{
	const int32 TotalMilliseconds = FMath::Max(0, FMath::RoundToInt(ElapsedTime * 1000.0f));
	const int32 Minutes = TotalMilliseconds / 60000;
	const int32 Seconds = (TotalMilliseconds / 1000) % 60;
	const int32 Milliseconds = TotalMilliseconds % 1000;
	return FText::FromString(FString::Printf(
		TEXT("%02d:%02d.%03d"), Minutes, Seconds, Milliseconds));
}
