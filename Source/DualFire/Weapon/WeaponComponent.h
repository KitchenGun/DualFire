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

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void FireGround();

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void FireAir();

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void FireUniversal();

	UFUNCTION(BlueprintPure, Category="Weapon|Loadout")
	bool CanFireLoadoutSlot(ELoadoutSlot Slot) const;

	UFUNCTION(BlueprintPure, Category="Weapon")
	bool CanFireGround() const;

	UFUNCTION(BlueprintPure, Category="Weapon")
	bool CanFireAir() const;

	UFUNCTION(BlueprintPure, Category="Weapon")
	bool CanFireUniversal() const;

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
