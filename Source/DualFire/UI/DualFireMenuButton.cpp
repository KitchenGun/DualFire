// Copyright DualFire. All Rights Reserved.

#include "UI/DualFireMenuButton.h"

#include "Components/Border.h"
#include "Components/TextBlock.h"

void UDualFireMenuButton::SetLabelText(const FText& InLabelText)
{
	LabelText = InLabelText;
	if (IsValid(ButtonLabel))
	{
		ButtonLabel->SetText(LabelText);
	}
}

void UDualFireMenuButton::NativePreConstruct()
{
	Super::NativePreConstruct();
	SetLabelText(LabelText);
	RefreshVisualState();
}

void UDualFireMenuButton::NativeOnHovered()
{
	Super::NativeOnHovered();
	RefreshVisualState();
}

void UDualFireMenuButton::NativeOnUnhovered()
{
	Super::NativeOnUnhovered();
	RefreshVisualState();
}

void UDualFireMenuButton::NativeOnSelected(const bool bBroadcast)
{
	Super::NativeOnSelected(bBroadcast);
	RefreshVisualState();
}

void UDualFireMenuButton::NativeOnDeselected(const bool bBroadcast)
{
	Super::NativeOnDeselected(bBroadcast);
	RefreshVisualState();
}

void UDualFireMenuButton::RefreshVisualState()
{
	if (IsValid(ButtonBackground))
	{
		ButtonBackground->SetBrushColor(IsHovered() || GetSelected() ? FocusedColor : NormalColor);
	}
}
