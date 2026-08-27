// Copyright DualFire. All Rights Reserved.

#include "UI/DualFireMenuWidgets.h"

#include "DualFire.h"
#include "UI/DualFireMenuButton.h"

#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/KismetSystemLibrary.h"

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

}

UDualFireStartMenuWidget::UDualFireStartMenuWidget()
{
	bIsBackActionDisplayedInActionBar = true;
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

void UDualFireStartMenuWidget::StartMission()
{
	if (!IsValid(PushScreenToLayer(EDualFireUILayer::Menu, CampaignWidgetClass)))
	{
		UE_LOG(LogDualFire, Error, TEXT("[UI] Failed to open the campaign screen."));
	}
}

void UDualFireStartMenuWidget::OpenSettings()
{
	PushScreenToLayer(EDualFireUILayer::Menu, SettingsWidgetClass);
}

void UDualFireStartMenuWidget::ExitGame()
{
	PushUniqueModalScreen(ExitConfirmWidgetClass);
}

UDualFireSettingsWidget::UDualFireSettingsWidget()
{
	bIsBackActionDisplayedInActionBar = true;
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
}

void UDualFireSettingsWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	LoadCurrentSettings();
}

UWidget* UDualFireSettingsWidget::NativeGetDesiredFocusTarget() const
{
	return WindowModeButton;
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
	bIsBackActionDisplayedInActionBar = true;
	bIsModal = true;
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
