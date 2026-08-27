// Copyright DualFire. All Rights Reserved.

#include "UI/DualFireUIPlayerController.h"

#include "DualFire.h"
#include "CommonActivatableWidget.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "GameInstance/DualFireMissionFlowSubsystem.h"

ADualFireUIPlayerController::ADualFireUIPlayerController()
{
	RootLayoutClass = UDualFirePrimaryLayout::StaticClass();
}

void ADualFireUIPlayerController::BeginPlay()
{
	Super::BeginPlay();
	AddUIInputMapping();
	InitializeRootLayout();
}

void ADualFireUIPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveRootLayout();
	RemoveUIInputMapping();
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

void ADualFireUIPlayerController::AddUIInputMapping()
{
	if (!IsLocalPlayerController() || !IsValid(UIInputMapping) || bUIInputMappingAdded)
	{
		return;
	}

	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = IsValid(LocalPlayer)
		? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()
		: nullptr;
	if (!IsValid(InputSubsystem))
	{
		UE_LOG(LogDualFire, Warning, TEXT("[UI] Enhanced Input subsystem is unavailable."));
		return;
	}

	// 화면마다 같은 Mapping Context를 제거하지 않도록 PlayerController가 수명 전체를 소유한다.
	InputSubsystem->AddMappingContext(UIInputMapping, UIInputMappingPriority);
	bUIInputMappingAdded = true;
}

void ADualFireUIPlayerController::RemoveUIInputMapping()
{
	if (!bUIInputMappingAdded)
	{
		return;
	}

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
			LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			InputSubsystem->RemoveMappingContext(UIInputMapping);
		}
	}
	bUIInputMappingAdded = false;
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

	ApplyPendingStartRoute();

	UE_LOG(LogDualFire, Log, TEXT("[UI] Root layout initialized: %s"), *GetNameSafe(RootLayout));
}

void ADualFireUIPlayerController::ApplyPendingStartRoute()
{
	UDualFireMissionFlowSubsystem* Flow = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDualFireMissionFlowSubsystem>()
		: nullptr;
	if (!IsValid(Flow))
	{
		return;
	}

	const EDualFireStartRoute Route = Flow->ConsumeStartRoute();
	if (Route == EDualFireStartRoute::None)
	{
		return;
	}
	if (!IsValid(CampaignMapWidgetClass) ||
		!IsValid(PushWidgetToLayer(EDualFireUILayer::Menu, CampaignMapWidgetClass)))
	{
		UE_LOG(LogDualFire, Error, TEXT("[UI] 시작 경로 Campaign 화면 열기 실패"));
		return;
	}
	if (Route == EDualFireStartRoute::Briefing &&
		(!IsValid(MissionBriefingWidgetClass) ||
		 !IsValid(PushWidgetToLayer(EDualFireUILayer::Menu, MissionBriefingWidgetClass))))
	{
		UE_LOG(LogDualFire, Error, TEXT("[UI] 시작 경로 Briefing 화면 열기 실패"));
	}
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
