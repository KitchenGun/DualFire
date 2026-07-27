// Copyright DualFire. All Rights Reserved.

#include "UI/DualFireMenuButtonStyle.h"

#include "Brushes/SlateColorBrush.h"

namespace
{
FSlateBrush MakeColorBrush(const FLinearColor& Color)
{
	return FSlateColorBrush(Color);
}
}

UDualFireMenuButtonStyle::UDualFireMenuButtonStyle()
{
	bSingleMaterial = false;

	const FLinearColor NormalColor(0.045f, 0.065f, 0.08f, 0.94f);
	const FLinearColor FocusedColor(0.06f, 0.48f, 0.56f, 1.0f);
	const FLinearColor FocusedHoveredColor(0.08f, 0.58f, 0.67f, 1.0f);
	const FLinearColor PressedColor(0.04f, 0.30f, 0.36f, 1.0f);
	const FLinearColor DisabledColor(0.045f, 0.055f, 0.065f, 0.45f);

	NormalBase = MakeColorBrush(NormalColor);
	NormalHovered = MakeColorBrush(FocusedColor);
	NormalPressed = MakeColorBrush(PressedColor);
	SelectedBase = MakeColorBrush(FocusedColor);
	SelectedHovered = MakeColorBrush(FocusedHoveredColor);
	SelectedPressed = MakeColorBrush(PressedColor);
	Disabled = MakeColorBrush(DisabledColor);

	ButtonPadding = FMargin(16.0f, 8.0f);
	CustomPadding = FMargin(0.0f);
	MinHeight = 40;
}
