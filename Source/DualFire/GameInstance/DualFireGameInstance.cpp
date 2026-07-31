// Copyright DualFire. All Rights Reserved.

#include "GameInstance/DualFireGameInstance.h"
#include "DualFire.h"
#include "Framework/Application/NavigationConfig.h"
#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"

void UDualFireGameInstance::Init()
{
	Super::Init();

	if (FSlateApplication::IsInitialized())
	{
		TSharedRef<FNavigationConfig> NavigationConfig = FSlateApplication::Get().GetNavigationConfig();
		if (const EUINavigation* ExistingW = NavigationConfig->KeyEventRules.Find(EKeys::W))
		{
			bHadWNavigationRule = true;
			PreviousWNavigation = *ExistingW;
		}
		if (const EUINavigation* ExistingS = NavigationConfig->KeyEventRules.Find(EKeys::S))
		{
			bHadSNavigationRule = true;
			PreviousSNavigation = *ExistingS;
		}

		NavigationConfig->KeyEventRules.Add(EKeys::W, EUINavigation::Up);
		NavigationConfig->KeyEventRules.Add(EKeys::S, EUINavigation::Down);
		bMenuNavigationInstalled = true;
	}

	UE_LOG(LogDualFire, Log, TEXT("[DualFireGameInstance] Init"));
}

void UDualFireGameInstance::Shutdown()
{
	if (bMenuNavigationInstalled && FSlateApplication::IsInitialized())
	{
		TSharedRef<FNavigationConfig> NavigationConfig = FSlateApplication::Get().GetNavigationConfig();
		if (bHadWNavigationRule)
		{
			NavigationConfig->KeyEventRules.Add(EKeys::W, PreviousWNavigation);
		}
		else
		{
			NavigationConfig->KeyEventRules.Remove(EKeys::W);
		}

		if (bHadSNavigationRule)
		{
			NavigationConfig->KeyEventRules.Add(EKeys::S, PreviousSNavigation);
		}
		else
		{
			NavigationConfig->KeyEventRules.Remove(EKeys::S);
		}
	}
	bMenuNavigationInstalled = false;

	UE_LOG(LogDualFire, Log, TEXT("[DualFireGameInstance] Shutdown"));
	Super::Shutdown();
}
