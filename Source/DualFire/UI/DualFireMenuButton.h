// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "DualFireMenuButton.generated.h"

class UBorder;
class UTextBlock;

/** 시각적 계층을 WBP_MenuButton에서 편집하는 재사용 가능한 Common UI 메뉴 버튼이다. */
UCLASS(BlueprintType, Blueprintable)
class DUALFIRE_API UDualFireMenuButton : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Menu Button")
	void SetLabelText(const FText& InLabelText);

	UFUNCTION(BlueprintPure, Category = "Menu Button")
	FText GetLabelText() const { return LabelText; }

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeOnHovered() override;
	virtual void NativeOnUnhovered() override;
	virtual void NativeOnSelected(bool bBroadcast) override;
	virtual void NativeOnDeselected(bool bBroadcast) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu Button", meta = (ExposeOnSpawn))
	FText LabelText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu Button|Color")
	FLinearColor NormalColor = FLinearColor(0.045f, 0.065f, 0.08f, 0.94f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu Button|Color")
	FLinearColor FocusedColor = FLinearColor(0.06f, 0.48f, 0.56f, 1.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Menu Button|Widgets", meta = (BindWidget))
	TObjectPtr<UBorder> ButtonBackground;

	UPROPERTY(BlueprintReadOnly, Category = "Menu Button|Widgets", meta = (BindWidget))
	TObjectPtr<UTextBlock> ButtonLabel;

private:
	void RefreshVisualState();
};
