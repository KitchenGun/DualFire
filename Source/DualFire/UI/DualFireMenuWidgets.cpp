// Copyright DualFire. All Rights Reserved.

#include "UI/DualFireMenuWidgets.h"

#include "DualFire.h"
#include "UI/DualFireMenuButton.h"
#include "UI/DualFireUIPlayerController.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "Input/CommonUIInputTypes.h"
#include "InputAction.h"
#include "Kismet/KismetSystemLibrary.h"
#include "TimerManager.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

namespace
{
FText WindowModeToText(const EWindowMode::Type WindowMode)
{
	switch (WindowMode)
	{
	case EWindowMode::Fullscreen:
		return NSLOCTEXT("DualFireUI", "Fullscreen", "FULLSCREEN");
	case EWindowMode::Windowed:
		return NSLOCTEXT("DualFireUI", "Windowed", "WINDOWED");
	case EWindowMode::WindowedFullscreen:
	default:
		return NSLOCTEXT("DualFireUI", "Borderless", "BORDERLESS");
	}
}

void RegisterConfirmPrompt(UCommonUserWidget& Widget, const UInputAction* ConfirmInputAction)
{
	if (!IsValid(ConfirmInputAction))
	{
		UE_LOG(LogDualFire, Warning, TEXT("[UI] ConfirmInputAction is not configured on %s."), *Widget.GetName());
		return;
	}

	const TWeakObjectPtr<UCommonUserWidget> WeakWidget(&Widget);
	FBindUIActionArgs BindArgs(ConfirmInputAction, true, FSimpleDelegate::CreateLambda([WeakWidget]()
	{
		UCommonUserWidget* BoundWidget = WeakWidget.Get();
		if (!IsValid(BoundWidget) || !IsValid(BoundWidget->WidgetTree))
		{
			return;
		}

		TArray<UWidget*> Widgets;
		BoundWidget->WidgetTree->GetAllWidgets(Widgets);
		for (UWidget* ChildWidget : Widgets)
		{
			UDualFireMenuButton* MenuButton = Cast<UDualFireMenuButton>(ChildWidget);
			if (IsValid(MenuButton) && MenuButton->ExecuteFocusedSelectAction())
			{
				return;
			}
		}

		UE_LOG(LogDualFire, Warning, TEXT("[UI] Select input has no focused menu button on %s."),
			*BoundWidget->GetName());
	}));
	BindArgs.OverrideDisplayName = NSLOCTEXT("DualFireUI", "SelectAction", "SELECT");
	Widget.RegisterUIActionBinding(BindArgs);
}
}

UDualFireStartMenuWidget::UDualFireStartMenuWidget()
{
	bAutoRestoreFocus = true;
	bIsBackHandler = true;
	bIsBackActionDisplayedInActionBar = true;
	OverrideBackActionDisplayName = NSLOCTEXT("DualFireUI", "BackAction", "BACK");
}

TOptional<FUIInputConfig> UDualFireStartMenuWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

void UDualFireStartMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!ensure(IsValid(StartMissionButton) && IsValid(SettingsButton) && IsValid(ExitButton)))
	{
		return;
	}

	StartMissionButton->OnClicked().AddUObject(this, &ThisClass::StartMission);
	SettingsButton->OnClicked().AddUObject(this, &ThisClass::OpenSettings);
	ExitButton->OnClicked().AddUObject(this, &ThisClass::ExitGame);
	RegisterConfirmPrompt(*this, ConfirmInputAction);
}

void UDualFireStartMenuWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	// Common UI의 포커스 복원을 먼저 요청하고 다음 틱에도 비어 있을 때만 보완한다.
	RequestRefreshFocus();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(this, &ThisClass::ApplyFallbackFocus);
	}
}

UWidget* UDualFireStartMenuWidget::NativeGetDesiredFocusTarget() const
{
	return StartMissionButton;
}

bool UDualFireStartMenuWidget::NativeOnHandleBackAction()
{
	ExitGame();
	return true;
}

void UDualFireStartMenuWidget::ApplyFallbackFocus()
{
	APlayerController* Controller = GetOwningPlayer();
	if (!IsValid(Controller) || HasAnyUserFocus() || HasUserFocusedDescendants(Controller))
	{
		return;
	}

	if (UWidget* FocusTarget = GetDesiredFocusTarget())
	{
		FocusTarget->SetUserFocus(Controller);
	}
}

void UDualFireStartMenuWidget::StartMission()
{
	ADualFireUIPlayerController* Controller = Cast<ADualFireUIPlayerController>(GetOwningPlayer());
	if (!IsValid(Controller) || !IsValid(CampaignWidgetClass))
	{
		UE_LOG(LogDualFire, Warning, TEXT("[UI] Campaign screen is not configured."));
		return;
	}

	if (!IsValid(Controller->PushWidgetToLayer(EDualFireUILayer::Menu, CampaignWidgetClass)))
	{
		UE_LOG(LogDualFire, Error, TEXT("[UI] Failed to open the campaign screen."));
	}
}

void UDualFireStartMenuWidget::OpenSettings()
{
	ADualFireUIPlayerController* Controller = Cast<ADualFireUIPlayerController>(GetOwningPlayer());
	if (!IsValid(Controller) || !IsValid(SettingsWidgetClass))
	{
		UE_LOG(LogDualFire, Warning, TEXT("[UI] Settings screen is not configured."));
		return;
	}

	Controller->PushWidgetToLayer(EDualFireUILayer::Menu, SettingsWidgetClass);
}

void UDualFireStartMenuWidget::ExitGame()
{
	ADualFireUIPlayerController* Controller = Cast<ADualFireUIPlayerController>(GetOwningPlayer());
	if (!IsValid(Controller) || !IsValid(ExitConfirmWidgetClass))
	{
		UE_LOG(LogDualFire, Warning, TEXT("[UI] Exit confirmation screen is not configured."));
		return;
	}

	UDualFirePrimaryLayout* RootLayout = Controller->GetRootLayout();
	UCommonActivatableWidgetStack* ModalStack = IsValid(RootLayout)
		? RootLayout->GetLayerStack(EDualFireUILayer::Modal)
		: nullptr;
	if (!IsValid(ModalStack))
	{
		UE_LOG(LogDualFire, Warning, TEXT("[UI] Modal layer is unavailable."));
		return;
	}

	// 활성 Modal이 있는 동안에는 같은 확인창을 다시 쌓지 않는다.
	if (IsValid(ModalStack->GetActiveWidget()))
	{
		return;
	}

	Controller->PushWidgetToLayer(EDualFireUILayer::Modal, ExitConfirmWidgetClass);
}

UDualFireSettingsWidget::UDualFireSettingsWidget()
{
	bAutoRestoreFocus = true;
	bIsBackHandler = true;
	bIsBackActionDisplayedInActionBar = true;
	OverrideBackActionDisplayName = NSLOCTEXT("DualFireUI", "BackAction", "BACK");
}

TOptional<FUIInputConfig> UDualFireSettingsWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

void UDualFireSettingsWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!ensure(IsValid(WindowModeButton) && IsValid(VSyncButton) &&
		IsValid(ApplyButton) && IsValid(BackButton) && IsValid(StatusLabel)))
	{
		return;
	}

	WindowModeButton->OnClicked().AddUObject(this, &ThisClass::CycleWindowMode);
	VSyncButton->OnClicked().AddUObject(this, &ThisClass::ToggleVSync);
	ApplyButton->OnClicked().AddUObject(this, &ThisClass::ApplySettings);
	BackButton->OnClicked().AddUObject(this, &ThisClass::CloseSettings);
	RegisterConfirmPrompt(*this, ConfirmInputAction);
}

void UDualFireSettingsWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	LoadCurrentSettings();
	// Common UI의 포커스 복원을 먼저 요청하고 다음 틱에도 비어 있을 때만 보완한다.
	RequestRefreshFocus();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(this, &ThisClass::ApplyFallbackFocus);
	}
}

UWidget* UDualFireSettingsWidget::NativeGetDesiredFocusTarget() const
{
	return WindowModeButton;
}

void UDualFireSettingsWidget::ApplyFallbackFocus()
{
	APlayerController* Controller = GetOwningPlayer();
	if (!IsValid(Controller) || HasAnyUserFocus() || HasUserFocusedDescendants(Controller))
	{
		return;
	}

	if (UWidget* FocusTarget = GetDesiredFocusTarget())
	{
		FocusTarget->SetUserFocus(Controller);
	}
}

bool UDualFireSettingsWidget::NativeOnHandleBackAction()
{
	CloseSettings();
	return true;
}

void UDualFireSettingsWidget::LoadCurrentSettings()
{
	UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!IsValid(Settings))
	{
		UE_LOG(LogDualFire, Warning, TEXT("[UI] GameUserSettings is unavailable."));
		return;
	}

	PendingWindowMode = Settings->GetFullscreenMode();
	bPendingVSync = Settings->IsVSyncEnabled();
	StatusLabel->SetText(FText::GetEmpty());
	UpdateSettingLabels();
}

void UDualFireSettingsWidget::UpdateSettingLabels()
{
	WindowModeButton->SetLabelText(FText::Format(
		NSLOCTEXT("DualFireUI", "WindowModeFormat", "WINDOW MODE  {0}"),
		WindowModeToText(PendingWindowMode)));
	VSyncButton->SetLabelText(FText::Format(
		NSLOCTEXT("DualFireUI", "VSyncFormat", "VSYNC  {0}"),
		bPendingVSync
			? NSLOCTEXT("DualFireUI", "Enabled", "ON")
			: NSLOCTEXT("DualFireUI", "Disabled", "OFF")));
}

void UDualFireSettingsWidget::CycleWindowMode()
{
	switch (PendingWindowMode)
	{
	case EWindowMode::Fullscreen:
		PendingWindowMode = EWindowMode::WindowedFullscreen;
		break;
	case EWindowMode::WindowedFullscreen:
		PendingWindowMode = EWindowMode::Windowed;
		break;
	case EWindowMode::Windowed:
	default:
		PendingWindowMode = EWindowMode::Fullscreen;
		break;
	}

	StatusLabel->SetText(FText::GetEmpty());
	UpdateSettingLabels();
}

void UDualFireSettingsWidget::ToggleVSync()
{
	bPendingVSync = !bPendingVSync;
	StatusLabel->SetText(FText::GetEmpty());
	UpdateSettingLabels();
}

void UDualFireSettingsWidget::ApplySettings()
{
	UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!IsValid(Settings))
	{
		return;
	}

	Settings->SetFullscreenMode(PendingWindowMode);
	Settings->SetVSyncEnabled(bPendingVSync);
	Settings->ApplySettings(false);
	Settings->SaveSettings();
	StatusLabel->SetText(NSLOCTEXT("DualFireUI", "SettingsApplied", "SETTINGS APPLIED"));
	UE_LOG(LogDualFire, Log, TEXT("[UI] Display settings applied."));
}

void UDualFireSettingsWidget::CloseSettings()
{
	DeactivateWidget();
}

UDualFireExitConfirmWidget::UDualFireExitConfirmWidget()
{
	bAutoRestoreFocus = true;
	bIsBackHandler = true;
	bIsBackActionDisplayedInActionBar = true;
	bIsModal = true;
	OverrideBackActionDisplayName = NSLOCTEXT("DualFireUI", "BackAction", "BACK");
}

TOptional<FUIInputConfig> UDualFireExitConfirmWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

void UDualFireExitConfirmWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!ensure(IsValid(ConfirmButton) && IsValid(CancelButton)))
	{
		return;
	}

	ConfirmButton->OnClicked().AddUObject(this, &ThisClass::ConfirmExit);
	CancelButton->OnClicked().AddUObject(this, &ThisClass::CancelExit);
	RegisterConfirmPrompt(*this, ConfirmInputAction);
}

UWidget* UDualFireExitConfirmWidget::NativeGetDesiredFocusTarget() const
{
	return CancelButton;
}

bool UDualFireExitConfirmWidget::NativeOnHandleBackAction()
{
	CancelExit();
	return true;
}

void UDualFireExitConfirmWidget::ConfirmExit()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void UDualFireExitConfirmWidget::CancelExit()
{
	DeactivateWidget();
}
