// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/DualFireDataTypes.h"
#include "Weapon/Projectile/BaseProjectile.h"
#include "WeaponComponent.generated.h"

struct FWeaponSlotState
{
	ELoadoutSlot Slot = ELoadoutSlot::PrimaryWeapon;
	FName WeaponID = NAME_None;
	FWeaponRow WeaponData;
	TSubclassOf<ABaseProjectile> ProjectileClass;
	FTimerHandle CooldownHandle;
	bool bCooldownActive = false;

	void Reset(ELoadoutSlot InSlot)
	{
		Slot = InSlot;
		WeaponID = NAME_None;
		WeaponData = FWeaponRow();
		ProjectileClass = nullptr;
		CooldownHandle.Invalidate();
		bCooldownActive = false;
	}
};

UCLASS(ClassGroup=Weapon, meta=(BlueprintSpawnableComponent))
class DUALFIRE_API UWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWeaponComponent();

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Weapon|Loadout")
	FLoadout ActiveLoadout;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Weapon")
	FVector MuzzleOffset = FVector(50.f, 0.f, 0.f);

	/** LoadoutManager가 해석한 세 행을 전부 검증한 뒤 슬롯 상태를 원자적으로 교체한다. */
	bool TryApplyResolvedLoadout(
		const FLoadout& Loadout,
		const FWeaponRow& PrimaryWeaponRow,
		const FWeaponRow& SpecialWeapon1Row,
		const FWeaponRow& SpecialWeapon2Row,
		FText& OutError,
		FName& OutInvalidField);

	UFUNCTION(BlueprintCallable, Category="Weapon|Loadout")
	void FireLoadoutSlot(ELoadoutSlot Slot);

	/** 기본 무기 슬롯(PrimaryWeapon) 발사 */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void FirePrimary();

	/** 특수무장 슬롯 1 발사 */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void FireSpecial1();

	/** 특수무장 슬롯 2 발사 */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void FireSpecial2();

private:
	FWeaponSlotState PrimaryWeaponSlot;
	FWeaponSlotState SpecialWeapon1Slot;
	FWeaponSlotState SpecialWeapon2Slot;

	FWeaponSlotState* GetWeaponSlotState(ELoadoutSlot Slot);

	bool BuildWeaponSlotState(
		ELoadoutSlot Slot,
		FName Field,
		FName WeaponID,
		const FWeaponRow& WeaponRow,
		FWeaponSlotState& OutState,
		FText& OutError,
		FName& OutInvalidField) const;
	void ClearActiveCooldowns();
	void OnLoadoutSlotCooldownExpired(ELoadoutSlot Slot);
};
