// Copyright DualFire. All Rights Reserved.

#include "UI/DualFireMenuWidgets.h"

#include "DualFire.h"
#include "UI/DualFireMenuButton.h"
#include "UI/DualFireUIPlayerController.h"

#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "TimerManager.h"

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
	bAutoRestoreFocus = true;
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
	if (MissionLevelName.IsNone())
	{
		UE_LOG(LogDualFire, Error, TEXT("[UI] MissionLevelName is not assigned."));
		return;
	}

	if (IsValid(StartMissionButton))
	{
		StartMissionButton->SetIsEnabled(false);
	}
	UE_LOG(LogDualFire, Log, TEXT("[UI] Opening mission level: %s"), *MissionLevelName.ToString());
	UGameplayStatics::OpenLevel(this, MissionLevelName);
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
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

UDualFireSettingsWidget::UDualFireSettingsWidget()
{
	bAutoRestoreFocus = true;
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

FReply UDualFireSettingsWidget::NativeOnPreviewKeyDown(
	const FGeometry& InGeometry,
	const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right)
	{
		CloseSettings();
		return FReply::Handled();
	}

	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
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
