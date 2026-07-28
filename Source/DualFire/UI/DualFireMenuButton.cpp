// Copyright DualFire. All Rights Reserved.

#include "UI/DualFireMenuButton.h"

#include "UI/DualFireMenuButtonStyle.h"

#include "CommonTextBlock.h"

UDualFireMenuButton::UDualFireMenuButton()
{
	Style = UDualFireMenuButtonStyle::StaticClass();
	SetIsSelectable(true);
	SetShouldSelectUponReceivingFocus(true);
	SetIsInteractableWhenSelected(true);
}

void UDualFireMenuButton::SetLabelText(const FText& InLabelText)
{
	LabelText = InLabelText;
	if (IsValid(ButtonLabel))
	{
		ButtonLabel->SetText(LabelText);
	}
}

bool UDualFireMenuButton::ExecuteFocusedSelectAction()
{
	APlayerController* Controller = GetOwningPlayer();
	const bool bHasFocus = IsValid(Controller) &&
		(HasUserFocus(Controller) || HasUserFocusedDescendants(Controller));
	if (!bHasFocus || !GetIsEnabled() || !IsInteractionEnabled())
	{
		return false;
	}

	HandleTriggeringActionCommited();
	return true;
}

void UDualFireMenuButton::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	OnFocusLost().AddUObject(this, &ThisClass::HandleFocusLost);
}

void UDualFireMenuButton::NativePreConstruct()
{
	Super::NativePreConstruct();
	SetLabelText(LabelText);
	NativeOnCurrentTextStyleChanged();
}

void UDualFireMenuButton::NativeOnHovered()
{
	Super::NativeOnHovered();

	// 마우스 탐색도 Common UI의 단일 선택 상태를 사용하도록 포커스를 동기화한다.
	if (GetIsEnabled())
	{
		if (APlayerController* Controller = GetOwningPlayer())
		{
			SetUserFocus(Controller);
		}
	}
}

void UDualFireMenuButton::NativeOnCurrentTextStyleChanged()
{
	Super::NativeOnCurrentTextStyleChanged();

	if (IsValid(ButtonLabel))
	{
		if (const TSubclassOf<UCommonTextStyle> CurrentTextStyle = GetCurrentTextStyleClass())
		{
			ButtonLabel->SetStyle(CurrentTextStyle);
		}
	}
}

void UDualFireMenuButton::HandleFocusLost()
{
	// 포커스 표현에 Selected 상태를 사용하므로 이탈 시 선택 상태를 남기지 않는다.
	ClearSelection();
}
