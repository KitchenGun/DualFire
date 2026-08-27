// Copyright DualFire. All Rights Reserved.

#include "UI/DualFireScreenFrameWidget.h"

#include "Components/Image.h"

void UDualFireScreenFrameWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (IsValid(BackgroundImage))
	{
		BackgroundImage->SetBrushFromTexture(BackgroundTexture, true);
	}
	if (IsValid(TopAccentContainer))
	{
		TopAccentContainer->SetVisibility(
			bShowTopAccent ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}
