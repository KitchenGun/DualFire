// Copyright DualFire. All Rights Reserved.

#include "UI/DualFireMenuButton.h"

#include "Blueprint/WidgetTree.h"
#include "DualFire.h"
#include "Input/CommonUIInputTypes.h"
#include "InputAction.h"
#include "UI/DualFireMenuButtonStyle.h"

#include "CommonTextBlock.h"

void RegisterDualFireConfirmPrompt(
	UCommonUserWidget& Widget,
	const UInputAction* ConfirmInputAction)
{
	if (!IsValid(ConfirmInputAction))
	{
		UE_LOG(LogDualFire, Warning, TEXT("[UI] ConfirmInputAction is not configured on %s."), *Widget.GetName());
		return;
	}

	const TWeakObjectPtr<UCommonUserWidget> WeakWidget(&Widget);
	FBindUIActionArgs BindArgs(ConfirmInputAction, true, FSimpleDelegate::CreateLambda([WeakWidget]()
	{
		UCommonUserWidget* BoundWidget = WeakWidget.Get();
		if (!IsValid(BoundWidget) || !IsValid(BoundWidget->WidgetTree))
		{
			return;
		}

		TArray<UWidget*> Widgets;
		BoundWidget->WidgetTree->GetAllWidgets(Widgets);
		for (UWidget* ChildWidget : Widgets)
		{
			UDualFireMenuButton* MenuButton = Cast<UDualFireMenuButton>(ChildWidget);
			if (IsValid(MenuButton) && MenuButton->ExecuteFocusedSelectAction())
			{
				return;
			}
		}

		UE_LOG(LogDualFire, Warning, TEXT("[UI] Select input has no focused menu button on %s."),
			*BoundWidget->GetName());
	}));
	BindArgs.OverrideDisplayName = NSLOCTEXT("DualFireUI", "SelectAction", "SELECT");
	Widget.RegisterUIActionBinding(BindArgs);
}

UDualFireMenuButton::UDualFireMenuButton()
{
	Style = UDualFireMenuButtonStyle::StaticClass();
	SetIsSelectable(true);
	SetSelectUponFocusEnabled(true);
	SetIsInteractableWhenSelected(true);
}

void UDualFireMenuButton::SetSelectUponFocusEnabled(const bool bEnabled)
{
	bSelectUponFocusEnabled = bEnabled;
	SetShouldSelectUponReceivingFocus(bEnabled);
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
	if (bSelectUponFocusEnabled && GetIsEnabled())
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
	if (bSelectUponFocusEnabled)
	{
		ClearSelection();
	}
}
