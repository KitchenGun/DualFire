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
	// SingleShot 미구현 폴백 경고를 슬롯당 1회로 제한 (Triggered 연사로 인한 로그 스팸 방지)
	bool bSingleShotWarningLogged = false;

	void Reset(ELoadoutSlot InSlot)
	{
		Slot = InSlot;
		WeaponID = NAME_None;
		WeaponData = FWeaponRow();
		ProjectileClass = nullptr;
		CooldownHandle.Invalidate();
		bCooldownActive = false;
		bSingleShotWarningLogged = false;
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

	UFUNCTION(BlueprintPure, Category="Weapon|Loadout")
	bool CanFireLoadoutSlot(ELoadoutSlot Slot) const;

	UFUNCTION(BlueprintPure, Category="Weapon")
	bool CanFirePrimary() const;

	UFUNCTION(BlueprintPure, Category="Weapon")
	bool CanFireSpecial1() const;

	UFUNCTION(BlueprintPure, Category="Weapon")
	bool CanFireSpecial2() const;

private:
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
	void ClearActiveCooldowns();
	float GetCooldownFromFireRate(float FireRate) const;
	void OnLoadoutSlotCooldownExpired(ELoadoutSlot Slot);
};
