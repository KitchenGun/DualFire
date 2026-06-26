// Copyright DualFire. All Rights Reserved.

#include "GameInstance/DualFireGameInstance.h"
#include "DualFire.h"

void UDualFireGameInstance::Init()
{
	Super::Init();
	UE_LOG(LogDualFire, Log, TEXT("[DualFireGameInstance] Init"));
}

void UDualFireGameInstance::Shutdown()
{
	UE_LOG(LogDualFire, Log, TEXT("[DualFireGameInstance] Shutdown"));
	Super::Shutdown();
}
