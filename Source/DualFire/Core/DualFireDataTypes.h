// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "DualFireTypes.h"
#include "DualFireDataTypes.generated.h"

class UStaticMesh;
class UTexture2D;
class ABaseProjectile;

namespace DualFireLoadout
{
	constexpr int32 MaxPresetCount = 50;
}

USTRUCT(BlueprintType)
struct DUALFIRE_API FWeaponRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FName WeaponID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	EWeaponCategory Category = EWeaponCategory::Primary;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TSoftClassPtr<ABaseProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TArray<EDualFireAttribute> AttributeArray;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "1"))
	int32 BaseDamage = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "0.001"))
	float FireRate = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	EFireMode FireMode = EFireMode::Auto;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "0.0"))
	float ProjectileSpeed = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	EHitBehavior HitBehavior = EHitBehavior::Destroy;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "0"))
	int32 PenetrationLimit = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FVector MuzzleOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TSoftObjectPtr<UStaticMesh> ProjectileMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FLinearColor ProjectileColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FName CategoryTag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FName UnlockID = NAME_None;
};

USTRUCT(BlueprintType)
struct DUALFIRE_API FEnemyRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	FName EnemyID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	FEnemyAttribute Attribute;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (ClampMin = "1"))
	int32 MaxHealth = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	TSoftObjectPtr<UStaticMesh> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	EEnemyMovementPattern MovementPattern = EEnemyMovementPattern::Linear;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (ClampMin = "0"))
	int32 ContactDamage = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	EEnemyAttackPattern AttackPattern = EEnemyAttackPattern::Single;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (ClampMin = "0"))
	int32 AttackDamage = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (ClampMin = "0.0"))
	float FireInterval = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (ClampMin = "0.0"))
	float EnemyProjectileSpeed = 600.0f;
};

USTRUCT(BlueprintType)
struct DUALFIRE_API FShieldRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shield")
	FName ShieldID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shield")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shield")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shield", meta = (ClampMin = "0"))
	int32 MaxShield = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shield", meta = (ClampMin = "0.0"))
	float RegenInterval = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shield", meta = (ClampMin = "0.0"))
	float BreakInvincibilitySec = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shield")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shield")
	FName CategoryTag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shield")
	FName UnlockID = NAME_None;
};

USTRUCT(BlueprintType)
struct DUALFIRE_API FSuperWeaponEffect
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperWeapon")
	ESuperWeaponEffectType EffectType = ESuperWeaponEffectType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperWeapon")
	TArray<EDualFireAttribute> AttributeArray;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperWeapon")
	float Value = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperWeapon", meta = (ClampMin = "0.0"))
	float Range = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperWeapon", meta = (ClampMin = "0.0"))
	float Duration = 0.0f;
};

USTRUCT(BlueprintType)
struct DUALFIRE_API FSuperWeaponRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperWeapon")
	FName SuperWeaponID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperWeapon")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperWeapon")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperWeapon")
	TArray<FSuperWeaponEffect> Effects;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperWeapon", meta = (ClampMin = "0"))
	int32 ActivationCost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperWeapon", meta = (ClampMin = "1"))
	int32 MaxStock = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperWeapon", meta = (ClampMin = "0.0"))
	float ChargeSpeedMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperWeapon")
	bool bAllowMovementDuringUse = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperWeapon")
	bool bAllowAttackDuringUse = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperWeapon")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperWeapon")
	FName CategoryTag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperWeapon")
	FName UnlockID = NAME_None;
};

USTRUCT(BlueprintType)
struct DUALFIRE_API FShipRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship")
	FName ShipID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship", meta = (ClampMin = "1"))
	int32 MaxHealth = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship")
	TSoftObjectPtr<UStaticMesh> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship", meta = (ClampMin = "0.0"))
	float HitboxRadius = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship")
	FName UnlockID = NAME_None;
};

USTRUCT(BlueprintType)
struct DUALFIRE_API FStageRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage")
	FName StageID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage")
	float ScrollSpeed = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage", meta = (ClampMin = "0.0"))
	float EliteTriggerTime = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage")
	FName EliteEnemyID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage", meta = (ClampMin = "0.0"))
	float EliteTimeLimit = 60.0f;
};

USTRUCT(BlueprintType)
struct DUALFIRE_API FWaveRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	FName StageID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	FName WaveID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave", meta = (ClampMin = "0.0"))
	float TriggerTime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	FName EnemyID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	ESpawnAnchor SpawnAnchor = ESpawnAnchor::TopCenter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	FVector SpawnOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave", meta = (ClampMin = "1"))
	int32 Count = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave", meta = (ClampMin = "0.0"))
	float SpawnInterval = 0.0f;
};

USTRUCT(BlueprintType)
struct DUALFIRE_API FLoadout
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loadout")
	FName ShipID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loadout")
	FName PrimaryWeaponID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loadout")
	FName SpecialWeapon1ID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loadout")
	FName SpecialWeapon2ID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loadout")
	FName SuperWeaponID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loadout")
	FName ShieldID = NAME_None;

	bool IsComplete() const
	{
		return !ShipID.IsNone()
			&& !PrimaryWeaponID.IsNone()
			&& !SpecialWeapon1ID.IsNone()
			&& !SpecialWeapon2ID.IsNone()
			&& !SuperWeaponID.IsNone()
			&& !ShieldID.IsNone();
	}

	FName GetEquipmentID(ELoadoutSlot Slot) const
	{
		switch (Slot)
		{
		case ELoadoutSlot::PrimaryWeapon:
			return PrimaryWeaponID;
		case ELoadoutSlot::SpecialWeapon1:
			return SpecialWeapon1ID;
		case ELoadoutSlot::SpecialWeapon2:
			return SpecialWeapon2ID;
		case ELoadoutSlot::SuperWeapon:
			return SuperWeaponID;
		case ELoadoutSlot::Shield:
			return ShieldID;
		default:
			return NAME_None;
		}
	}
};

USTRUCT(BlueprintType)
struct DUALFIRE_API FLoadoutPreset
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loadout")
	FName PresetID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loadout")
	FText PresetName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loadout")
	FLoadout Loadout;

	bool IsComplete() const
	{
		return !PresetID.IsNone() && Loadout.IsComplete();
	}
};
