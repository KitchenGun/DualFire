// Copyright DualFire. All Rights Reserved.

#include "UI/DualFireUIPlayerController.h"

#include "DualFire.h"
#include "CommonActivatableWidget.h"

ADualFireUIPlayerController::ADualFireUIPlayerController()
{
	RootLayoutClass = UDualFirePrimaryLayout::StaticClass();
}

void ADualFireUIPlayerController::BeginPlay()
{
	Super::BeginPlay();
	InitializeRootLayout();
}

void ADualFireUIPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveRootLayout();
	Super::EndPlay(EndPlayReason);
}

UCommonActivatableWidget* ADualFireUIPlayerController::PushWidgetToLayer(
	const EDualFireUILayer Layer,
	const TSubclassOf<UCommonActivatableWidget> WidgetClass)
{
	if (!IsValid(RootLayout))
	{
		UE_LOG(LogDualFire, Warning, TEXT("[UI] Cannot push widget before the root layout is initialized."));
		return nullptr;
	}

	return RootLayout->PushWidgetToLayer(Layer, WidgetClass);
}

bool ADualFireUIPlayerController::PopActiveWidget(const EDualFireUILayer Layer)
{
	return IsValid(RootLayout) && RootLayout->PopActiveWidget(Layer);
}

void ADualFireUIPlayerController::ClearLayer(const EDualFireUILayer Layer)
{
	if (IsValid(RootLayout))
	{
		RootLayout->ClearLayer(Layer);
	}
}

void ADualFireUIPlayerController::InitializeRootLayout()
{
	if (!IsLocalPlayerController() || IsValid(RootLayout))
	{
		return;
	}

	if (!IsValid(RootLayoutClass))
	{
		UE_LOG(LogDualFire, Error, TEXT("[UI] RootLayoutClass is not assigned on %s."), *GetName());
		return;
	}

	RootLayout = CreateWidget<UDualFirePrimaryLayout>(this, RootLayoutClass);
	if (!IsValid(RootLayout))
	{
		UE_LOG(LogDualFire, Error, TEXT("[UI] Failed to create root layout %s."), *GetNameSafe(RootLayoutClass));
		return;
	}

	if (!RootLayout->AddToPlayerScreen())
	{
		UE_LOG(LogDualFire, Error, TEXT("[UI] Failed to add root layout %s to the player screen."), *GetNameSafe(RootLayout));
		RootLayout = nullptr;
		return;
	}

	if (IsValid(InitialWidgetClass) &&
		!IsValid(RootLayout->PushWidgetToLayer(InitialWidgetLayer, InitialWidgetClass)))
	{
		UE_LOG(LogDualFire, Error,
			TEXT("[UI] Failed to push initial widget %s."),
			*GetNameSafe(InitialWidgetClass));
	}

	UE_LOG(LogDualFire, Log, TEXT("[UI] Root layout initialized: %s"), *GetNameSafe(RootLayout));
}

void ADualFireUIPlayerController::RemoveRootLayout()
{
	if (!IsValid(RootLayout))
	{
		return;
	}

	RootLayout->RemoveFromParent();
	RootLayout = nullptr;
}
