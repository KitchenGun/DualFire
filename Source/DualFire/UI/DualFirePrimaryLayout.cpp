// Copyright DualFire. All Rights Reserved.

#include "UI/DualFirePrimaryLayout.h"

#include "DualFire.h"
#include "Blueprint/WidgetTree.h"
#include "CommonActivatableWidget.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

namespace
{
const FName GameStackName(TEXT("Stack_Game"));
const FName MenuStackName(TEXT("Stack_Menu"));
const FName ModalStackName(TEXT("Stack_Modal"));
const FName SystemStackName(TEXT("Stack_System"));
}

void UDualFirePrimaryLayout::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// WBP에 디자이너 트리가 없을 때만 네이티브 기본 레이어 구조를 생성한다.
	if (!IsValid(WidgetTree->RootWidget))
	{
		BuildDefaultWidgetTree();
	}

	if (!ResolveLayerStacks())
	{
		UE_LOG(LogDualFire, Error,
			TEXT("[UI] Primary layout requires Stack_Game, Stack_Menu, Stack_Modal, and Stack_System."));
	}
}

UCommonActivatableWidgetStack* UDualFirePrimaryLayout::GetLayerStack(const EDualFireUILayer Layer) const
{
	switch (Layer)
	{
	case EDualFireUILayer::Game:
		return GameStack;
	case EDualFireUILayer::Menu:
		return MenuStack;
	case EDualFireUILayer::Modal:
		return ModalStack;
	case EDualFireUILayer::System:
		return SystemStack;
	default:
		return nullptr;
	}
}

UCommonActivatableWidget* UDualFirePrimaryLayout::PushWidgetToLayer(
	const EDualFireUILayer Layer,
	const TSubclassOf<UCommonActivatableWidget> WidgetClass)
{
	UCommonActivatableWidgetStack* Stack = GetLayerStack(Layer);
	if (!IsValid(Stack) || !IsValid(WidgetClass))
	{
		UE_LOG(LogDualFire, Warning,
			TEXT("[UI] Cannot push widget. Layer=%d WidgetClass=%s"),
			static_cast<int32>(Layer),
			*GetNameSafe(WidgetClass));
		return nullptr;
	}

	return Stack->AddWidget(WidgetClass);
}

bool UDualFirePrimaryLayout::PopActiveWidget(const EDualFireUILayer Layer)
{
	UCommonActivatableWidgetStack* Stack = GetLayerStack(Layer);
	if (!IsValid(Stack))
	{
		UE_LOG(LogDualFire, Warning, TEXT("[UI] Cannot pop invalid layer %d."), static_cast<int32>(Layer));
		return false;
	}

	UCommonActivatableWidget* ActiveWidget = Stack->GetActiveWidget();
	if (!IsValid(ActiveWidget))
	{
		return false;
	}

	ActiveWidget->DeactivateWidget();
	return true;
}

void UDualFirePrimaryLayout::ClearLayer(const EDualFireUILayer Layer)
{
	UCommonActivatableWidgetStack* Stack = GetLayerStack(Layer);
	if (!IsValid(Stack))
	{
		UE_LOG(LogDualFire, Warning, TEXT("[UI] Cannot clear invalid layer %d."), static_cast<int32>(Layer));
		return;
	}

	Stack->ClearWidgets();
}

void UDualFirePrimaryLayout::BuildDefaultWidgetTree()
{
	check(WidgetTree);

	UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("RootOverlay"));
	WidgetTree->RootWidget = RootOverlay;

	auto AddFullScreenStack = [this, RootOverlay](const FName StackName)
	{
		UCommonActivatableWidgetStack* Stack =
			WidgetTree->ConstructWidget<UCommonActivatableWidgetStack>(
				UCommonActivatableWidgetStack::StaticClass(), StackName);

		UOverlaySlot* Slot = RootOverlay->AddChildToOverlay(Stack);
		check(Slot);
		Slot->SetHorizontalAlignment(HAlign_Fill);
		Slot->SetVerticalAlignment(VAlign_Fill);
		return Stack;
	};

	// Overlay에 나중에 추가한 스택일수록 화면 앞쪽에 배치된다.
	GameStack = AddFullScreenStack(GameStackName);
	MenuStack = AddFullScreenStack(MenuStackName);
	ModalStack = AddFullScreenStack(ModalStackName);
	SystemStack = AddFullScreenStack(SystemStackName);
}

bool UDualFirePrimaryLayout::ResolveLayerStacks()
{
	check(WidgetTree);

	GameStack = Cast<UCommonActivatableWidgetStack>(WidgetTree->FindWidget(GameStackName));
	MenuStack = Cast<UCommonActivatableWidgetStack>(WidgetTree->FindWidget(MenuStackName));
	ModalStack = Cast<UCommonActivatableWidgetStack>(WidgetTree->FindWidget(ModalStackName));
	SystemStack = Cast<UCommonActivatableWidgetStack>(WidgetTree->FindWidget(SystemStackName));

	return IsValid(GameStack) && IsValid(MenuStack) && IsValid(ModalStack) && IsValid(SystemStack);
}
