// Copyright DualFire. All Rights Reserved.

#include "Core/LoadoutDataLibrary.h"

#include "Core/DualFireCollisionChannels.h"
#include "Engine/DataTable.h"

template <typename RowType>
bool ULoadoutDataLibrary::FindRowByID(
	UDataTable* DataTable,
	FName RowID,
	FName RowType::* IDMember,
	RowType& OutRow,
	const TCHAR* Context)
{
	if (!IsValid(DataTable) || RowID.IsNone())
	{
		return false;
	}

	if (const RowType* DirectRow = DataTable->FindRow<RowType>(RowID, Context, false))
	{
		OutRow = *DirectRow;
		if ((OutRow.*IDMember).IsNone())
		{
			OutRow.*IDMember = RowID;
		}
		return true;
	}

	TArray<RowType*> Rows;
	DataTable->GetAllRows(Context, Rows);
	for (const RowType* Row : Rows)
	{
		if (Row && Row->*IDMember == RowID)
		{
			OutRow = *Row;
			return true;
		}
	}

	return false;
}

bool ULoadoutDataLibrary::FindWeaponRow(UDataTable* WeaponTable, FName WeaponID, FWeaponRow& OutWeaponRow)
{
	return FindRowByID(WeaponTable, WeaponID, &FWeaponRow::WeaponID, OutWeaponRow, TEXT("FindWeaponRow"));
}

bool ULoadoutDataLibrary::FindAircraftRow(UDataTable* AircraftTable, FName AircraftID, FAircraftRow& OutAircraftRow)
{
	return FindRowByID(AircraftTable, AircraftID, &FAircraftRow::AircraftID, OutAircraftRow, TEXT("FindAircraftRow"));
}

bool ULoadoutDataLibrary::FindShieldRow(UDataTable* ShieldTable, FName ShieldID, FShieldRow& OutShieldRow)
{
	return FindRowByID(ShieldTable, ShieldID, &FShieldRow::ShieldID, OutShieldRow, TEXT("FindShieldRow"));
}

bool ULoadoutDataLibrary::FindSuperWeaponRow(UDataTable* SuperWeaponTable, FName SuperWeaponID, FSuperWeaponRow& OutSuperWeaponRow)
{
	return FindRowByID(
		SuperWeaponTable,
		SuperWeaponID,
		&FSuperWeaponRow::SuperWeaponID,
		OutSuperWeaponRow,
		TEXT("FindSuperWeaponRow"));
}

bool ULoadoutDataLibrary::IsWeaponCategoryCompatible(ELoadoutSlot Slot, EWeaponCategory Category)
{
	switch (Slot)
	{
	case ELoadoutSlot::PrimaryWeapon:
		return Category == EWeaponCategory::Primary;
	case ELoadoutSlot::SpecialWeapon1:
	case ELoadoutSlot::SpecialWeapon2:
		return Category == EWeaponCategory::Special;
	default:
		return false;
	}
}

FProjectileRuntimeConfig ULoadoutDataLibrary::MakeProjectileRuntimeConfig(const FWeaponRow& WeaponRow)
{
	FProjectileRuntimeConfig RuntimeConfig;
	RuntimeConfig.AttributeArray = WeaponRow.AttributeArray;
	RuntimeConfig.Damage = static_cast<float>(FMath::Max(0, WeaponRow.BaseDamage));
	RuntimeConfig.ProjectileSpeed = FMath::Max(1.0f, WeaponRow.ProjectileSpeed);
	RuntimeConfig.HitBehavior = WeaponRow.HitBehavior;
	RuntimeConfig.PenetrationLimit = FMath::Max(0, WeaponRow.PenetrationLimit);
	RuntimeConfig.CollisionProfileName = DualFireProfile::PlayerBullet;
	RuntimeConfig.TargetChannel = DualFireChannel::EnemyBody;
	return RuntimeConfig;
}

FLoadout ULoadoutDataLibrary::MakeLoadoutFromRowHandles(const FLoadoutRowHandles& RowHandles)
{
	FLoadout Loadout;
	Loadout.AircraftID = RowHandles.AircraftRow.RowName;
	Loadout.PrimaryWeaponID = RowHandles.PrimaryWeaponRow.RowName;
	Loadout.SpecialWeapon1ID = RowHandles.SpecialWeapon1Row.RowName;
	Loadout.SpecialWeapon2ID = RowHandles.SpecialWeapon2Row.RowName;
	Loadout.SuperWeaponID = RowHandles.SuperWeaponRow.RowName;
	Loadout.ShieldID = RowHandles.ShieldRow.RowName;
	return Loadout;
}
