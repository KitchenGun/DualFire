// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "UI/DualFirePrimaryLayout.h"
#include "DualFireMenuScreenWidget.generated.h"

class UInputAction;

/** 메뉴 계열 화면이 공유하는 입력, 포커스, 화면 전환 동작을 제공한다. */
UCLASS(Abstract, BlueprintType, Blueprintable)
class DUALFIRE_API UDualFireMenuScreenWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UDualFireMenuScreenWidget();

	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnActivated() override;

	void SetFocusedButtonConfirmEnabled(bool bEnabled);

	UCommonActivatableWidget* PushScreenToLayer(
		EDualFireUILayer Layer,
		TSubclassOf<UCommonActivatableWidget> WidgetClass);
	UCommonActivatableWidget* PushUniqueModalScreen(
		TSubclassOf<UCommonActivatableWidget> WidgetClass);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> ConfirmInputAction;

private:
	void RegisterFocusedButtonConfirmAction();
	void HandleFocusedButtonConfirmAction();
	void ApplyFallbackFocus();

	bool bFocusedButtonConfirmEnabled = true;
};
