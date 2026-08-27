// Copyright DualFire. All Rights Reserved.

#include "UI/DualFireMenuScreenWidget.h"

#include "Blueprint/WidgetTree.h"
#include "DualFire.h"
#include "Input/CommonUIInputTypes.h"
#include "InputAction.h"
#include "TimerManager.h"
#include "UI/DualFireMenuButton.h"
#include "UI/DualFireUIPlayerController.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

UDualFireMenuScreenWidget::UDualFireMenuScreenWidget()
{
	bAutoRestoreFocus = true;
	bIsBackHandler = true;
	OverrideBackActionDisplayName = NSLOCTEXT("DualFireUI", "BackAction", "BACK");
}

TOptional<FUIInputConfig> UDualFireMenuScreenWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

void UDualFireMenuScreenWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (bFocusedButtonConfirmEnabled)
	{
		RegisterFocusedButtonConfirmAction();
	}
}

void UDualFireMenuScreenWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	RequestRefreshFocus();

	// Common UI가 복원할 대상이 없는 경우에만 다음 틱에 최초 포커스를 보완한다.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(this, &ThisClass::ApplyFallbackFocus);
	}
}

void UDualFireMenuScreenWidget::SetFocusedButtonConfirmEnabled(const bool bEnabled)
{
	bFocusedButtonConfirmEnabled = bEnabled;
}

UCommonActivatableWidget* UDualFireMenuScreenWidget::PushScreenToLayer(
	const EDualFireUILayer Layer,
	const TSubclassOf<UCommonActivatableWidget> WidgetClass)
{
	ADualFireUIPlayerController* Controller = Cast<ADualFireUIPlayerController>(GetOwningPlayer());
	if (!IsValid(Controller) || !IsValid(WidgetClass))
	{
		UE_LOG(LogDualFire, Warning, TEXT("[UI] Screen class or UI player controller is unavailable on %s."),
			*GetName());
		return nullptr;
	}

	return Controller->PushWidgetToLayer(Layer, WidgetClass);
}

UCommonActivatableWidget* UDualFireMenuScreenWidget::PushUniqueModalScreen(
	const TSubclassOf<UCommonActivatableWidget> WidgetClass)
{
	ADualFireUIPlayerController* Controller = Cast<ADualFireUIPlayerController>(GetOwningPlayer());
	UDualFirePrimaryLayout* RootLayout = IsValid(Controller) ? Controller->GetRootLayout() : nullptr;
	UCommonActivatableWidgetStack* ModalStack = IsValid(RootLayout)
		? RootLayout->GetLayerStack(EDualFireUILayer::Modal)
		: nullptr;
	if (!IsValid(ModalStack) || !IsValid(WidgetClass))
	{
		UE_LOG(LogDualFire, Warning, TEXT("[UI] Modal layer or screen class is unavailable on %s."), *GetName());
		return nullptr;
	}

	// 활성 Modal이 있는 동안에는 같은 입력으로 확인창을 중복 생성하지 않는다.
	if (UCommonActivatableWidget* ActiveWidget = ModalStack->GetActiveWidget())
	{
		return ActiveWidget;
	}

	return Controller->PushWidgetToLayer(EDualFireUILayer::Modal, WidgetClass);
}

void UDualFireMenuScreenWidget::RegisterFocusedButtonConfirmAction()
{
	if (!IsValid(ConfirmInputAction))
	{
		UE_LOG(LogDualFire, Warning, TEXT("[UI] ConfirmInputAction is not configured on %s."), *GetName());
		return;
	}

	FBindUIActionArgs BindArgs(
		ConfirmInputAction,
		true,
		FSimpleDelegate::CreateUObject(this, &ThisClass::HandleFocusedButtonConfirmAction));
	BindArgs.OverrideDisplayName = NSLOCTEXT("DualFireUI", "SelectAction", "SELECT");
	RegisterUIActionBinding(BindArgs);
}

void UDualFireMenuScreenWidget::HandleFocusedButtonConfirmAction()
{
	if (!IsValid(WidgetTree))
	{
		return;
	}

	TArray<UWidget*> Widgets;
	WidgetTree->GetAllWidgets(Widgets);
	for (UWidget* ChildWidget : Widgets)
	{
		UDualFireMenuButton* MenuButton = Cast<UDualFireMenuButton>(ChildWidget);
		if (IsValid(MenuButton) && MenuButton->ExecuteFocusedSelectAction())
		{
			return;
		}
	}

	UE_LOG(LogDualFire, Warning, TEXT("[UI] Select input has no focused menu button on %s."), *GetName());
}

void UDualFireMenuScreenWidget::ApplyFallbackFocus()
{
	APlayerController* Controller = GetOwningPlayer();
	if (!IsValid(Controller) || HasAnyUserFocus() || HasUserFocusedDescendants(Controller))
	{
		return;
	}

	if (UWidget* FocusTarget = GetDesiredFocusTarget())
	{
		FocusTarget->SetUserFocus(Controller);
	}
}
