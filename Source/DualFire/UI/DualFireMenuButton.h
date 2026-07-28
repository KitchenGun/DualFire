// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "DualFireMenuButton.generated.h"

class UCommonTextBlock;

/** 시각적 계층을 WBP_MenuButton에서 편집하는 재사용 가능한 Common UI 메뉴 버튼이다. */
UCLASS(BlueprintType, Blueprintable)
class DUALFIRE_API UDualFireMenuButton : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	UDualFireMenuButton();

	UFUNCTION(BlueprintCallable, Category = "Menu Button")
	void SetLabelText(const FText& InLabelText);

	UFUNCTION(BlueprintPure, Category = "Menu Button")
	FText GetLabelText() const { return LabelText; }

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativePreConstruct() override;
	virtual void NativeOnHovered() override;
	virtual void NativeOnCurrentTextStyleChanged() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu Button", meta = (ExposeOnSpawn))
	FText LabelText;

	UPROPERTY(BlueprintReadOnly, Category = "Menu Button|Widgets", meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> ButtonLabel;

private:
	void HandleFocusLost();
};
