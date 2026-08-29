// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/DualFireDataTypes.h"
#include "Weapon/Projectile/BaseProjectile.h"
#include "WeaponComponent.generated.h"

struct FWeaponSlotState
{
	FName WeaponID = NAME_None;
	FWeaponRow WeaponData;
	TSubclassOf<ABaseProjectile> ProjectileClass;
	FTimerHandle CooldownHandle;
	bool bCooldownActive = false;

	void Reset()
	{
		WeaponID = NAME_None;
		WeaponData = FWeaponRow();
		ProjectileClass = nullptr;
		CooldownHandle.Invalidate();
		bCooldownActive = false;
	}
};

DECLARE_MULTICAST_DELEGATE(FOnResolvedLoadoutApplied);

UCLASS(ClassGroup=Weapon, meta=(BlueprintSpawnableComponent))
class DUALFIRE_API UWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWeaponComponent();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Weapon")
	FVector MuzzleOffset = FVector(50.f, 0.f, 0.f);

	/** LoadoutManager가 해석한 세 행을 전부 검증한 뒤 슬롯 상태를 원자적으로 교체한다. */
	bool TryApplyResolvedLoadout(
		const FWeaponRow& PrimaryWeaponRow,
		const FWeaponRow& SpecialWeapon1Row,
		const FWeaponRow& SpecialWeapon2Row,
		FText& OutError,
		FName& OutInvalidField);

	/** 기본 무기 슬롯(PrimaryWeapon) 발사 */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void FirePrimary();

	/** 특수무장 슬롯 1 발사 */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void FireSpecial1();

	/** 특수무장 슬롯 2 발사 */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void FireSpecial2();

	/** 모든 무장 슬롯의 진행 중 쿨다운을 취소하고 즉시 발사 가능 상태로 복구 */
	void ResetCooldowns();

	/** 로드아웃 적용 후 HUD 등이 사용할 수 있는 슬롯 데이터 사본을 반환한다. */
	UFUNCTION(BlueprintPure, Category="Weapon|HUD")
	bool GetResolvedSlotData(ELoadoutSlot Slot, FWeaponRow& OutWeaponData) const;

	/** 진행 중인 쿨다운의 남은 비율. 준비 완료 또는 미해결 슬롯은 0이다. */
	UFUNCTION(BlueprintPure, Category="Weapon|HUD")
	float GetCooldownRemainingPercent(ELoadoutSlot Slot) const;

	/** 원자적 슬롯 교체가 끝난 뒤 HUD처럼 표시 데이터를 캐시하는 관찰자에게 알린다. */
	FOnResolvedLoadoutApplied OnResolvedLoadoutApplied;

private:
	void FireLoadoutSlot(ELoadoutSlot Slot);

	FWeaponSlotState PrimaryWeaponSlot;
	FWeaponSlotState SpecialWeapon1Slot;
	FWeaponSlotState SpecialWeapon2Slot;

	FWeaponSlotState* GetWeaponSlotState(ELoadoutSlot Slot);
	const FWeaponSlotState* GetWeaponSlotState(ELoadoutSlot Slot) const;

	bool BuildWeaponSlotState(
		ELoadoutSlot Slot,
		FName Field,
		FName WeaponID,
		const FWeaponRow& WeaponRow,
		FWeaponSlotState& OutState,
		FText& OutError,
		FName& OutInvalidField) const;
	void OnLoadoutSlotCooldownExpired(ELoadoutSlot Slot);
};
