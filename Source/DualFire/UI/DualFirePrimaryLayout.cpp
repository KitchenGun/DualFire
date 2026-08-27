// Copyright DualFire. All Rights Reserved.

#include "UI/DualFirePrimaryLayout.h"

#include "DualFire.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "CommonActivatableWidget.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SafeZone.h"
#include "Components/SafeZoneSlot.h"
#include "Input/CommonBoundActionBar.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

namespace
{
const FName GameStackName(TEXT("Stack_Game"));
const FName MenuStackName(TEXT("Stack_Menu"));
const FName ModalStackName(TEXT("Stack_Modal"));
const FName ActionBarName(TEXT("ActionBar"));
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
			TEXT("[UI] Primary layout requires Stack_Game, Stack_Menu, Stack_Modal, ActionBar, and Stack_System."));
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

	if (IsValid(ActionBarWidgetClass))
	{
		ActionBar = WidgetTree->ConstructWidget<UUserWidget>(ActionBarWidgetClass, ActionBarName);
	}
	else
	{
		ActionBar = WidgetTree->ConstructWidget<UCommonBoundActionBar>(
			UCommonBoundActionBar::StaticClass(), ActionBarName);
	}
	USafeZone* ActionBarSafeZone = WidgetTree->ConstructWidget<USafeZone>(
		USafeZone::StaticClass(), TEXT("ActionBarSafeZone"));
	ActionBarSafeZone->SetSidesToPad(true, true, true, true);
	UOverlaySlot* ActionBarSafeZoneSlot = RootOverlay->AddChildToOverlay(ActionBarSafeZone);
	check(ActionBarSafeZoneSlot);
	ActionBarSafeZoneSlot->SetHorizontalAlignment(HAlign_Fill);
	ActionBarSafeZoneSlot->SetVerticalAlignment(VAlign_Fill);

	ActionBarSafeZone->SetContent(ActionBar);
	USafeZoneSlot* ActionBarSlot = CastChecked<USafeZoneSlot>(ActionBar->Slot);
	ActionBarSlot->SetHorizontalAlignment(HAlign_Right);
	ActionBarSlot->SetVerticalAlignment(VAlign_Bottom);
	ActionBarSlot->SetPadding(FMargin(0.0f, 0.0f, 64.0f, 48.0f));
	ActionBar->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UCommonBoundActionBar* NativeActionBar = Cast<UCommonBoundActionBar>(ActionBar))
	{
		NativeActionBar->SetDisplayOwningPlayerActionsOnly(true);
	}

	SystemStack = AddFullScreenStack(SystemStackName);
}

bool UDualFirePrimaryLayout::ResolveLayerStacks()
{
	check(WidgetTree);

	GameStack = Cast<UCommonActivatableWidgetStack>(WidgetTree->FindWidget(GameStackName));
	MenuStack = Cast<UCommonActivatableWidgetStack>(WidgetTree->FindWidget(MenuStackName));
	ModalStack = Cast<UCommonActivatableWidgetStack>(WidgetTree->FindWidget(ModalStackName));
	ActionBar = WidgetTree->FindWidget(ActionBarName);
	SystemStack = Cast<UCommonActivatableWidgetStack>(WidgetTree->FindWidget(SystemStackName));

	return IsValid(GameStack) && IsValid(MenuStack) && IsValid(ModalStack) &&
		IsValid(ActionBar) && IsValid(SystemStack);
}
