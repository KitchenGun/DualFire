// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "DualFireMenuButton.generated.h"

class UCommonTextBlock;
class UCommonUserWidget;
class UInputAction;

DUALFIRE_API void RegisterDualFireConfirmPrompt(
	UCommonUserWidget& Widget,
	const UInputAction* ConfirmInputAction);

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

	/** 포커스와 선택 상태를 연동할지 설정한다. 탭 버튼은 클릭 또는 액션 입력으로만 선택해야 하므로 끌 수 있다. */
	void SetSelectUponFocusEnabled(bool bEnabled);

	/** Select 입력을 현재 포커스된 버튼의 일반 클릭 경로로 전달한다. */
	bool ExecuteFocusedSelectAction();

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

	bool bSelectUponFocusEnabled = true;
};
