// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/DualFireDataTypes.h"
#include "Weapon/Projectile/BaseProjectile.h"
#include "WeaponComponent.generated.h"

class UDataTable;

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

	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Data")
	TObjectPtr<UDataTable> WeaponDataTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Weapon|Loadout")
	FLoadout DefaultLoadout;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Weapon|Loadout")
	FLoadout ActiveLoadout;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Legacy")
	TSubclassOf<ABaseProjectile> GroundProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Legacy")
	TSubclassOf<ABaseProjectile> AirProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Legacy")
	TSubclassOf<ABaseProjectile> UniversalProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Weapon|Legacy", meta=(ClampMin="0.05"))
	float GroundFireCooldown = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Weapon|Legacy", meta=(ClampMin="0.05"))
	float AirFireCooldown = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Weapon|Legacy", meta=(ClampMin="0.05"))
	float UniversalFireCooldown = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Weapon")
	FVector MuzzleOffset = FVector(50.f, 0.f, 0.f);

	UFUNCTION(BlueprintCallable, Category="Weapon|Loadout")
	bool ApplyLoadout(const FLoadout& Loadout);

	UFUNCTION(BlueprintCallable, Category="Weapon|Loadout")
	void FireLoadoutSlot(ELoadoutSlot Slot);

	/** 기본 무기 슬롯(PrimaryWeapon) 발사 */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void FirePrimary();

	/** 특수무장 슬롯(SpecialWeapon1 + SpecialWeapon2) 발사. 각 슬롯은 쿨타임을 공유하지 않고 준비된 슬롯만 발사된다 */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void FireSpecial();

	UFUNCTION(BlueprintPure, Category="Weapon|Loadout")
	bool CanFireLoadoutSlot(ELoadoutSlot Slot) const;

	UFUNCTION(BlueprintPure, Category="Weapon")
	bool CanFirePrimary() const;

	/** 특수무장 슬롯 중 하나라도 발사 가능하면 true */
	UFUNCTION(BlueprintPure, Category="Weapon")
	bool CanFireSpecial() const;

private:
	FWeaponSlotState PrimaryWeaponSlot;
	FWeaponSlotState SpecialWeapon1Slot;
	FWeaponSlotState SpecialWeapon2Slot;

	FWeaponSlotState* GetWeaponSlotState(ELoadoutSlot Slot);
	const FWeaponSlotState* GetWeaponSlotState(ELoadoutSlot Slot) const;

	bool EquipWeaponSlot(ELoadoutSlot Slot, FName WeaponID, TSubclassOf<ABaseProjectile> LegacyProjectileClass);
	bool ResolveWeaponRow(FName WeaponID, FWeaponRow& OutWeaponRow) const;
	bool BuildDefaultTestWeaponRow(FName WeaponID, FWeaponRow& OutWeaponRow) const;
	TSubclassOf<ABaseProjectile> ResolveProjectileClass(const FWeaponRow& WeaponRow, TSubclassOf<ABaseProjectile> LegacyProjectileClass) const;
	float GetCooldownFromFireRate(float FireRate) const;
	void OnLoadoutSlotCooldownExpired(ELoadoutSlot Slot);
};
