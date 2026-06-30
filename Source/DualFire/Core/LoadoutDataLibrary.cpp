// Copyright DualFire. All Rights Reserved.

#include "Core/LoadoutDataLibrary.h"

#include "Engine/DataTable.h"

namespace
{
	template <typename RowType>
	FName GetRowID(const RowType& Row)
	{
		return NAME_None;
	}

	template <>
	FName GetRowID<FWeaponRow>(const FWeaponRow& Row)
	{
		return Row.WeaponID;
	}

	template <>
	FName GetRowID<FAircraftRow>(const FAircraftRow& Row)
	{
		return Row.AircraftID;
	}

	template <>
	FName GetRowID<FShieldRow>(const FShieldRow& Row)
	{
		return Row.ShieldID;
	}

	template <>
	FName GetRowID<FSuperWeaponRow>(const FSuperWeaponRow& Row)
	{
		return Row.SuperWeaponID;
	}

	template <typename RowType>
	void EnsureRowID(RowType& Row, FName RowID)
	{
	}

	template <>
	void EnsureRowID<FWeaponRow>(FWeaponRow& Row, FName RowID)
	{
		if (Row.WeaponID.IsNone())
		{
			Row.WeaponID = RowID;
		}
	}

	template <>
	void EnsureRowID<FAircraftRow>(FAircraftRow& Row, FName RowID)
	{
		if (Row.AircraftID.IsNone())
		{
			Row.AircraftID = RowID;
		}
	}

	template <>
	void EnsureRowID<FShieldRow>(FShieldRow& Row, FName RowID)
	{
		if (Row.ShieldID.IsNone())
		{
			Row.ShieldID = RowID;
		}
	}

	template <>
	void EnsureRowID<FSuperWeaponRow>(FSuperWeaponRow& Row, FName RowID)
	{
		if (Row.SuperWeaponID.IsNone())
		{
			Row.SuperWeaponID = RowID;
		}
	}
}

template <typename RowType>
bool ULoadoutDataLibrary::FindRowByID(UDataTable* DataTable, FName RowID, RowType& OutRow, const TCHAR* Context)
{
	if (!IsValid(DataTable) || RowID.IsNone())
	{
		return false;
	}

	if (const RowType* DirectRow = DataTable->FindRow<RowType>(RowID, Context, false))
	{
		OutRow = *DirectRow;
		EnsureRowID(OutRow, RowID);
		return true;
	}

	TArray<RowType*> Rows;
	DataTable->GetAllRows(Context, Rows);
	for (const RowType* Row : Rows)
	{
		if (Row && GetRowID(*Row) == RowID)
		{
			OutRow = *Row;
			return true;
		}
	}

	return false;
}

bool ULoadoutDataLibrary::FindWeaponRow(UDataTable* WeaponTable, FName WeaponID, FWeaponRow& OutWeaponRow)
{
	return FindRowByID(WeaponTable, WeaponID, OutWeaponRow, TEXT("FindWeaponRow"));
}

bool ULoadoutDataLibrary::FindAircraftRow(UDataTable* AircraftTable, FName AircraftID, FAircraftRow& OutAircraftRow)
{
	return FindRowByID(AircraftTable, AircraftID, OutAircraftRow, TEXT("FindAircraftRow"));
}

bool ULoadoutDataLibrary::FindShieldRow(UDataTable* ShieldTable, FName ShieldID, FShieldRow& OutShieldRow)
{
	return FindRowByID(ShieldTable, ShieldID, OutShieldRow, TEXT("FindShieldRow"));
}

bool ULoadoutDataLibrary::FindSuperWeaponRow(UDataTable* SuperWeaponTable, FName SuperWeaponID, FSuperWeaponRow& OutSuperWeaponRow)
{
	return FindRowByID(SuperWeaponTable, SuperWeaponID, OutSuperWeaponRow, TEXT("FindSuperWeaponRow"));
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
