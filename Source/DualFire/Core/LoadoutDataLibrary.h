// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Core/DualFireDataTypes.h"
#include "Weapon/Projectile/BaseProjectile.h"
#include "LoadoutDataLibrary.generated.h"

class UDataTable;

UCLASS()
class DUALFIRE_API ULoadoutDataLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Loadout|Data")
	static bool FindWeaponRow(UDataTable* WeaponTable, FName WeaponID, FWeaponRow& OutWeaponRow);

	UFUNCTION(BlueprintCallable, Category = "Loadout|Data")
	static bool FindAircraftRow(UDataTable* AircraftTable, FName AircraftID, FAircraftRow& OutAircraftRow);

	UFUNCTION(BlueprintCallable, Category = "Loadout|Data")
	static bool FindShieldRow(UDataTable* ShieldTable, FName ShieldID, FShieldRow& OutShieldRow);

	UFUNCTION(BlueprintCallable, Category = "Loadout|Data")
	static bool FindSuperWeaponRow(UDataTable* SuperWeaponTable, FName SuperWeaponID, FSuperWeaponRow& OutSuperWeaponRow);

	UFUNCTION(BlueprintPure, Category = "Loadout|Data")
	static bool IsWeaponCategoryCompatible(ELoadoutSlot Slot, EWeaponCategory Category);

	UFUNCTION(BlueprintPure, Category = "Loadout|Data")
	static FProjectileRuntimeConfig MakeProjectileRuntimeConfig(const FWeaponRow& WeaponRow);

	/** BP 디테일 패널에서 드롭다운으로 고른 FLoadoutRowHandles를 런타임용 FLoadout(FName 집합)으로 변환 */
	UFUNCTION(BlueprintPure, Category = "Loadout|Data")
	static FLoadout MakeLoadoutFromRowHandles(const FLoadoutRowHandles& RowHandles);

private:
	template <typename RowType>
	static bool FindRowByID(UDataTable* DataTable, FName RowID, RowType& OutRow, const TCHAR* Context);
};
