// Copyright DualFire. All Rights Reserved.

#include "GameInstance/DualFireGameUserSettings.h"

void UDualFireGameUserSettings::SetSlowInputMode(const ESlowInputMode InSlowInputMode)
{
	if (SlowInputMode == InSlowInputMode)
	{
		return;
	}

	SlowInputMode = InSlowInputMode;
	SaveSettings();
}
