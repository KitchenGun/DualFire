// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "UI/DualFirePrimaryLayout.h"
#include "DualFireUIPlayerController.generated.h"

class UCommonActivatableWidget;
class UInputMappingContext;

/** 로컬 플레이어용 Common UI 루트 레이아웃을 하나만 생성하고 소유한다. */
UCLASS(BlueprintType, Blueprintable)
class DUALFIRE_API ADualFireUIPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ADualFireUIPlayerController();

	UFUNCTION(BlueprintPure, Category = "UI")
	UDualFirePrimaryLayout* GetRootLayout() const { return RootLayout; }

	UFUNCTION(BlueprintCallable, Category = "UI", meta = (DeterminesOutputType = "WidgetClass"))
	UCommonActivatableWidget* PushWidgetToLayer(
		EDualFireUILayer Layer,
		TSubclassOf<UCommonActivatableWidget> WidgetClass);

	UFUNCTION(BlueprintCallable, Category = "UI")
	bool PopActiveWidget(EDualFireUILayer Layer);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ClearLayer(EDualFireUILayer Layer);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UDualFirePrimaryLayout> RootLayoutClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UCommonActivatableWidget> InitialWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	EDualFireUILayer InitialWidgetLayer = EDualFireUILayer::Menu;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Input")
	TObjectPtr<UInputMappingContext> UIInputMapping;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Input")
	int32 UIInputMappingPriority = 100;

private:
	void AddUIInputMapping();
	void RemoveUIInputMapping();
	void AddMenuNavigationKeys();
	void RemoveMenuNavigationKeys();
	void InitializeRootLayout();
	void RemoveRootLayout();

	UPROPERTY(Transient)
	TObjectPtr<UDualFirePrimaryLayout> RootLayout;

	bool bUIInputMappingAdded = false;
	bool bMenuNavigationKeysAdded = false;
};
