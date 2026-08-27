// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GenericPlatform/GenericWindow.h"
#include "UI/DualFireMenuScreenWidget.h"
#include "DualFireMenuWidgets.generated.h"

class UTextBlock;
class UDualFireMenuButton;

/** Menu 레이어에 표시되는 시작 화면이다. */
UCLASS(BlueprintType, Blueprintable)
class DUALFIRE_API UDualFireStartMenuWidget : public UDualFireMenuScreenWidget
{
	GENERATED_BODY()

public:
	UDualFireStartMenuWidget();

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void StartMission();

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void OpenSettings();

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void ExitGame();

protected:
	virtual void NativeOnInitialized() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual bool NativeOnHandleBackAction() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu")
	TSubclassOf<UCommonActivatableWidget> SettingsWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu")
	TSubclassOf<UCommonActivatableWidget> ExitConfirmWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu")
	TSubclassOf<UCommonActivatableWidget> CampaignWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category = "Menu|Widgets", meta = (BindWidget))
	TObjectPtr<UDualFireMenuButton> StartMissionButton;

	UPROPERTY(BlueprintReadOnly, Category = "Menu|Widgets", meta = (BindWidget))
	TObjectPtr<UDualFireMenuButton> SettingsButton;

	UPROPERTY(BlueprintReadOnly, Category = "Menu|Widgets", meta = (BindWidget))
	TObjectPtr<UDualFireMenuButton> ExitButton;

};

/** 시작 메뉴 위에 추가되는 기본 화면 설정 메뉴다. */
UCLASS(BlueprintType, Blueprintable)
class DUALFIRE_API UDualFireSettingsWidget : public UDualFireMenuScreenWidget
{
	GENERATED_BODY()

public:
	UDualFireSettingsWidget();

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void CycleWindowMode();

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void ToggleVSync();

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void ApplySettings();

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void CloseSettings();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnActivated() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual bool NativeOnHandleBackAction() override;

	UPROPERTY(BlueprintReadOnly, Category = "Settings|Widgets", meta = (BindWidget))
	TObjectPtr<UDualFireMenuButton> WindowModeButton;

	UPROPERTY(BlueprintReadOnly, Category = "Settings|Widgets", meta = (BindWidget))
	TObjectPtr<UDualFireMenuButton> VSyncButton;

	UPROPERTY(BlueprintReadOnly, Category = "Settings|Widgets", meta = (BindWidget))
	TObjectPtr<UDualFireMenuButton> ApplyButton;

	UPROPERTY(BlueprintReadOnly, Category = "Settings|Widgets", meta = (BindWidget))
	TObjectPtr<UDualFireMenuButton> BackButton;

	UPROPERTY(BlueprintReadOnly, Category = "Settings|Widgets", meta = (BindWidget))
	TObjectPtr<UTextBlock> StatusLabel;

private:
	void LoadCurrentSettings();
	void UpdateSettingLabels();

	EWindowMode::Type PendingWindowMode = EWindowMode::WindowedFullscreen;
	bool bPendingVSync = false;
};

/** 종료 요청을 확인하고 이전 메뉴 포커스를 복원하는 Modal 화면이다. */
UCLASS(BlueprintType, Blueprintable)
class DUALFIRE_API UDualFireExitConfirmWidget : public UDualFireMenuScreenWidget
{
	GENERATED_BODY()

public:
	UDualFireExitConfirmWidget();

	UFUNCTION(BlueprintCallable, Category = "Exit")
	void ConfirmExit();

	UFUNCTION(BlueprintCallable, Category = "Exit")
	void CancelExit();

protected:
	virtual void NativeOnInitialized() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual bool NativeOnHandleBackAction() override;

	UPROPERTY(BlueprintReadOnly, Category = "Exit|Widgets", meta = (BindWidget))
	TObjectPtr<UDualFireMenuButton> ConfirmButton;

	UPROPERTY(BlueprintReadOnly, Category = "Exit|Widgets", meta = (BindWidget))
	TObjectPtr<UDualFireMenuButton> CancelButton;
};
