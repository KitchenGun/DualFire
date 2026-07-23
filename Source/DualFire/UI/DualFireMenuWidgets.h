// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "GenericPlatform/GenericWindow.h"
#include "DualFireMenuWidgets.generated.h"

class UTextBlock;
class UDualFireMenuButton;

/** Menu 레이어에 표시되는 시작 화면이다. */
UCLASS(BlueprintType, Blueprintable)
class DUALFIRE_API UDualFireStartMenuWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UDualFireStartMenuWidget();

	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void StartMission();

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void OpenSettings();

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void ExitGame();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnActivated() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu")
	TSubclassOf<UCommonActivatableWidget> SettingsWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu")
	FName MissionLevelName = TEXT("/Game/Level/LV_Test");

	UPROPERTY(BlueprintReadOnly, Category = "Menu|Widgets", meta = (BindWidget))
	TObjectPtr<UDualFireMenuButton> StartMissionButton;

	UPROPERTY(BlueprintReadOnly, Category = "Menu|Widgets", meta = (BindWidget))
	TObjectPtr<UDualFireMenuButton> SettingsButton;

	UPROPERTY(BlueprintReadOnly, Category = "Menu|Widgets", meta = (BindWidget))
	TObjectPtr<UDualFireMenuButton> ExitButton;

private:
	void ApplyFallbackFocus();
};

/** 시작 메뉴 위에 추가되는 기본 화면 설정 메뉴다. */
UCLASS(BlueprintType, Blueprintable)
class DUALFIRE_API UDualFireSettingsWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UDualFireSettingsWidget();

	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

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
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

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
	void ApplyFallbackFocus();
	void LoadCurrentSettings();
	void UpdateSettingLabels();

	EWindowMode::Type PendingWindowMode = EWindowMode::WindowedFullscreen;
	bool bPendingVSync = false;
};
