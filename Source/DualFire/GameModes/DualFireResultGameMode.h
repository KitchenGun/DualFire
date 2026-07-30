// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DualFireResultGameMode.generated.h"

/** Pawn과 전투 시스템 없이 결과 UI만 실행하는 게임모드다. */
UCLASS(BlueprintType, Blueprintable)
class DUALFIRE_API ADualFireResultGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADualFireResultGameMode();
};
