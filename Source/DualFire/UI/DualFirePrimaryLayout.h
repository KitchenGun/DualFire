// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "DualFirePrimaryLayout.generated.h"

class UCommonActivatableWidget;
class UCommonActivatableWidgetStack;
class UUserWidget;

UENUM(BlueprintType)
enum class EDualFireUILayer : uint8
{
	Game,
	Menu,
	Modal,
	System
};

/** 메뉴와 게임 플레이 컨트롤러가 공유하는 전체 화면 Common UI 레이어 루트다. */
UCLASS(BlueprintType, Blueprintable)
class DUALFIRE_API UDualFirePrimaryLayout : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "UI|Layout")
	UCommonActivatableWidgetStack* GetLayerStack(EDualFireUILayer Layer) const;

	UFUNCTION(BlueprintCallable, Category = "UI|Layout", meta = (DeterminesOutputType = "WidgetClass"))
	UCommonActivatableWidget* PushWidgetToLayer(
		EDualFireUILayer Layer,
		TSubclassOf<UCommonActivatableWidget> WidgetClass);

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Layout")
	TSubclassOf<UUserWidget> ActionBarWidgetClass;

private:
	void BuildDefaultWidgetTree();
	bool ResolveLayerStacks();

	UPROPERTY(Transient)
	TObjectPtr<UCommonActivatableWidgetStack> GameStack;

	UPROPERTY(Transient)
	TObjectPtr<UCommonActivatableWidgetStack> MenuStack;

	UPROPERTY(Transient)
	TObjectPtr<UCommonActivatableWidgetStack> ModalStack;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> ActionBar;

	UPROPERTY(Transient)
	TObjectPtr<UCommonActivatableWidgetStack> SystemStack;
};
