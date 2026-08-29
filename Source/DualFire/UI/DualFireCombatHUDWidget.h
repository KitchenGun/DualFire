// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "DualFireCombatHUDWidget.generated.h"

class UCommonActionWidget;
class UCommonTextBlock;
class UHealthComponent;
class UHorizontalBox;
class UImage;
class UInputAction;
class UProgressBar;
class UTexture2D;
class UWeaponComponent;

/** Game 레이어에서 플레이어 생존 자원과 특수무장 두 슬롯만 표시한다. */
UCLASS(BlueprintType, Blueprintable)
class DUALFIRE_API UDualFireCombatHUDWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	/** 표시 전용 HUD가 활성화되어도 Common UI가 gameplay input을 Menu 모드로 바꾸지 않게 한다. */
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

	/** Pawn 재점유 때 HUD 인스턴스는 유지하고 관찰할 컴포넌트만 바꾼다. */
	UFUNCTION(BlueprintCallable, Category="Combat HUD")
	void SetObservedPawn(APawn* InPawn);

	UFUNCTION(BlueprintPure, Category="Combat HUD")
	APawn* GetObservedPawn() const { return ObservedPawn.Get(); }

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(BlueprintReadOnly, Category="Combat HUD|Widgets", meta=(BindWidget))
	TObjectPtr<UHorizontalBox> HealthSegmentContainer;
	UPROPERTY(BlueprintReadOnly, Category="Combat HUD|Widgets", meta=(BindWidget))
	TObjectPtr<UHorizontalBox> ShieldSegmentContainer;
	UPROPERTY(BlueprintReadOnly, Category="Combat HUD|Widgets", meta=(BindWidget))
	TObjectPtr<UCommonTextBlock> LivesText;

	UPROPERTY(BlueprintReadOnly, Category="Combat HUD|Widgets", meta=(BindWidget))
	TObjectPtr<UImage> Special1Icon;
	UPROPERTY(BlueprintReadOnly, Category="Combat HUD|Widgets", meta=(BindWidget))
	TObjectPtr<UCommonTextBlock> Special1AttributeText;
	UPROPERTY(BlueprintReadOnly, Category="Combat HUD|Widgets", meta=(BindWidget))
	TObjectPtr<UCommonActionWidget> Special1ActionWidget;
	UPROPERTY(BlueprintReadOnly, Category="Combat HUD|Widgets", meta=(BindWidget))
	TObjectPtr<UProgressBar> Special1CooldownBar;

	UPROPERTY(BlueprintReadOnly, Category="Combat HUD|Widgets", meta=(BindWidget))
	TObjectPtr<UImage> Special2Icon;
	UPROPERTY(BlueprintReadOnly, Category="Combat HUD|Widgets", meta=(BindWidget))
	TObjectPtr<UCommonTextBlock> Special2AttributeText;
	UPROPERTY(BlueprintReadOnly, Category="Combat HUD|Widgets", meta=(BindWidget))
	TObjectPtr<UCommonActionWidget> Special2ActionWidget;
	UPROPERTY(BlueprintReadOnly, Category="Combat HUD|Widgets", meta=(BindWidget))
	TObjectPtr<UProgressBar> Special2CooldownBar;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat HUD|Input")
	TObjectPtr<UInputAction> Special1InputAction;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat HUD|Input")
	TObjectPtr<UInputAction> Special2InputAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat HUD|Presentation")
	TObjectPtr<UTexture2D> HealthSegmentTexture;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat HUD|Presentation")
	TObjectPtr<UTexture2D> ShieldSegmentTexture;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat HUD|Presentation")
	FVector2D SegmentDesiredSize = FVector2D(28.0f, 12.0f);

private:
	UFUNCTION()
	void HandleHealthChanged(int32 CurrentHealth, int32 MaxHealth);
	UFUNCTION()
	void HandleShieldChanged(int32 CurrentShield, int32 MaxShield);
	UFUNCTION()
	void HandleLifeChanged(int32 CurrentLife);
	void HandleResolvedLoadoutApplied();

	void BindObservedHealth(UHealthComponent* InHealthComponent);
	void UnbindObservedHealth();
	void BindObservedWeapon(UWeaponComponent* InWeaponComponent);
	void UnbindObservedWeapon();
	void RefreshSpecialPresentation();
	void RefreshCooldowns();
	void SetSegments(
		UHorizontalBox* Container,
		int32 Current,
		int32 Maximum,
		const FLinearColor& FilledColor,
		UTexture2D* SegmentTexture);
	void SetSpecialPresentation(int32 SpecialIndex);

	TWeakObjectPtr<APawn> ObservedPawn;
	TWeakObjectPtr<UHealthComponent> ObservedHealthComponent;
	TWeakObjectPtr<UWeaponComponent> ObservedWeaponComponent;
};
