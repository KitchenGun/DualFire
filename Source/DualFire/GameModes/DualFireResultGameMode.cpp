// Copyright DualFire. All Rights Reserved.

#include "GameModes/DualFireResultGameMode.h"

#include "UI/DualFireResultPlayerController.h"

ADualFireResultGameMode::ADualFireResultGameMode()
{
	PlayerControllerClass = ADualFireResultPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
	SpectatorClass = nullptr;
	HUDClass = nullptr;
	bStartPlayersAsSpectators = true;
}
