// Copyright DualFire. All Rights Reserved.

#include "Loadout/LoadoutManagerSubsystem.h"
#include "Player/DualFirePlayerPawn.h"
#include "Health/HealthComponent.h"
#include "Weapon/WeaponComponent.h"
#include "Core/LoadoutDataLibrary.h"
#include "DualFire.h"
#include "PaperFlipbook.h"

namespace
{
	bool IsValidProjectileClass(const FWeaponRow& WeaponRow)
	{
		if (WeaponRow.ProjectileClass.IsNull())
		{
			return false;
		}

		UClass* ProjectileClass = WeaponRow.ProjectileClass.LoadSynchronous();
		return IsValid(ProjectileClass) && ProjectileClass->IsChildOf(ABaseProjectile::StaticClass());
	}
}

void ULoadoutManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (!AircraftDataTable)
	{
		AircraftDataTable = LoadObject<UDataTable>(nullptr,
			TEXT("/Game/Data/Loadout/DT_LoadoutAircrafts.DT_LoadoutAircrafts"));
	}
	if (!WeaponDataTable)
	{
		WeaponDataTable = LoadObject<UDataTable>(nullptr,
			TEXT("/Game/Data/Loadout/DT_LoadoutWeapons.DT_LoadoutWeapons"));
	}
	if (!SuperWeaponDataTable)
	{
		SuperWeaponDataTable = LoadObject<UDataTable>(nullptr,
			TEXT("/Game/Data/Loadout/DT_LoadoutSuperWeapons.DT_LoadoutSuperWeapons"));
	}
	if (!ShieldDataTable)
	{
		ShieldDataTable = LoadObject<UDataTable>(nullptr,
			TEXT("/Game/Data/Loadout/DT_LoadoutShields.DT_LoadoutShields"));
	}
}

void ULoadoutManagerSubsystem::CommitActiveLoadout(const FLoadout& Loadout)
{
	ActiveLoadout = Loadout;
	bHasActiveLoadout = true;
	UE_LOG(LogDualFire, Log, TEXT("[LoadoutManagerSubsystem] 로드아웃 설정 — Aircraft:%s"), *Loadout.AircraftID.ToString());
}

bool ULoadoutManagerSubsystem::TrySetActiveLoadout(
	const FLoadout& Loadout,
	FText& OutError,
	FName& OutInvalidField)
{
	if (!ValidateLoadout(Loadout, OutError, OutInvalidField))
	{
		return false;
	}

	CommitActiveLoadout(Loadout);
	return true;
}

bool ULoadoutManagerSubsystem::ValidateLoadout(
	const FLoadout& Loadout,
	FText& OutError,
	FName& OutInvalidField) const
{
	OutError = FText::GetEmpty();
	OutInvalidField = NAME_None;

	auto Fail = [&OutError, &OutInvalidField](const FName Field, const FText& Message)
	{
		OutInvalidField = Field;
		OutError = Message;
		return false;
	};

	if (Loadout.AircraftID.IsNone())
	{
		return Fail(TEXT("Aircraft"), NSLOCTEXT("DualFireLoadout", "AircraftRequired", "SELECT AN AIRCRAFT."));
	}
	if (Loadout.PrimaryWeaponID.IsNone())
	{
		return Fail(TEXT("PrimaryWeapon"), NSLOCTEXT("DualFireLoadout", "PrimaryRequired", "SELECT A PRIMARY WEAPON."));
	}
	if (Loadout.SpecialWeapon1ID.IsNone())
	{
		return Fail(TEXT("SpecialWeapon1"), NSLOCTEXT("DualFireLoadout", "Special1Required", "SELECT SPECIAL WEAPON 1."));
	}
	if (Loadout.SpecialWeapon2ID.IsNone())
	{
		return Fail(TEXT("SpecialWeapon2"), NSLOCTEXT("DualFireLoadout", "Special2Required", "SELECT SPECIAL WEAPON 2."));
	}
	if (Loadout.SuperWeaponID.IsNone())
	{
		return Fail(TEXT("SuperWeapon"), NSLOCTEXT("DualFireLoadout", "SuperRequired", "SELECT A SUPER WEAPON."));
	}
	if (Loadout.ShieldID.IsNone())
	{
		return Fail(TEXT("Shield"), NSLOCTEXT("DualFireLoadout", "ShieldRequired", "SELECT A SHIELD."));
	}

	FAircraftRow AircraftRow;
	if (!ULoadoutDataLibrary::FindAircraftRow(AircraftDataTable, Loadout.AircraftID, AircraftRow) ||
		AircraftRow.MaxHealth < 1 ||
		AircraftRow.AircraftClass.IsNull() ||
		!IsValid(AircraftRow.AircraftClass.LoadSynchronous()) ||
		AircraftRow.BankFlipbook.IsNull() ||
		!IsValid(AircraftRow.BankFlipbook.LoadSynchronous()))
	{
		return Fail(TEXT("Aircraft"), NSLOCTEXT("DualFireLoadout", "AircraftInvalid", "THE SELECTED AIRCRAFT IS UNAVAILABLE."));
	}

	auto ValidateWeapon = [this, &Fail](const FName Field, const FName WeaponID, const ELoadoutSlot Slot)
	{
		FWeaponRow WeaponRow;
		if (!ULoadoutDataLibrary::FindWeaponRow(WeaponDataTable, WeaponID, WeaponRow) ||
			!ULoadoutDataLibrary::IsWeaponCategoryCompatible(Slot, WeaponRow.Category) ||
			WeaponRow.FireRate <= 0.f ||
			WeaponRow.ProjectileSpeed <= 0.f ||
			WeaponRow.AttributeArray.IsEmpty() ||
			!IsValidProjectileClass(WeaponRow))
		{
			return Fail(Field, NSLOCTEXT("DualFireLoadout", "WeaponInvalid", "THE SELECTED WEAPON IS UNAVAILABLE FOR THIS SLOT."));
		}
		return true;
	};

	if (!ValidateWeapon(TEXT("PrimaryWeapon"), Loadout.PrimaryWeaponID, ELoadoutSlot::PrimaryWeapon) ||
		!ValidateWeapon(TEXT("SpecialWeapon1"), Loadout.SpecialWeapon1ID, ELoadoutSlot::SpecialWeapon1) ||
		!ValidateWeapon(TEXT("SpecialWeapon2"), Loadout.SpecialWeapon2ID, ELoadoutSlot::SpecialWeapon2))
	{
		return false;
	}

	FSuperWeaponRow SuperWeaponRow;
	if (!ULoadoutDataLibrary::FindSuperWeaponRow(SuperWeaponDataTable, Loadout.SuperWeaponID, SuperWeaponRow))
	{
		return Fail(TEXT("SuperWeapon"), NSLOCTEXT("DualFireLoadout", "SuperInvalid", "THE SELECTED SUPER WEAPON IS UNAVAILABLE."));
	}

	FShieldRow ShieldRow;
	if (!ULoadoutDataLibrary::FindShieldRow(ShieldDataTable, Loadout.ShieldID, ShieldRow) ||
		ShieldRow.MaxShield < 0 ||
		(ShieldRow.MaxShield > 0 &&
			(ShieldRow.ShieldRecoveryDelay < 0.f ||
			 ShieldRow.ShieldRecoveryDuration <= 0.f ||
			 ShieldRow.BreakInvincibilityDuration < 0.f)))
	{
		return Fail(TEXT("Shield"), NSLOCTEXT("DualFireLoadout", "ShieldInvalid", "THE SELECTED SHIELD IS UNAVAILABLE."));
	}

	return true;
}

bool ULoadoutManagerSubsystem::TryApplyActiveLoadout(
	ADualFirePlayerPawn* Pawn,
	FText& OutError,
	FName& OutInvalidField)
{
	OutError = FText::GetEmpty();
	OutInvalidField = NAME_None;

	auto Fail = [&OutError, &OutInvalidField](const FName Field, const FText& Message)
	{
		OutInvalidField = Field;
		OutError = Message;
		return false;
	};

	if (!bHasActiveLoadout)
	{
		return Fail(TEXT("Loadout"), NSLOCTEXT("DualFireLoadout", "LoadoutMissing", "NO ACTIVE LOADOUT IS AVAILABLE."));
	}

	if (!ValidateLoadout(ActiveLoadout, OutError, OutInvalidField))
	{
		return false;
	}

	if (!IsValid(Pawn))
	{
		return Fail(TEXT("Pawn"), NSLOCTEXT("DualFireLoadout", "PawnInvalid", "THE PLAYER PAWN IS UNAVAILABLE."));
	}

	UWeaponComponent* WeaponComp = Pawn->GetWeaponComp();
	UHealthComponent* HealthComp = Pawn->GetHealthComp();
	if (!IsValid(WeaponComp) || !IsValid(HealthComp))
	{
		return Fail(TEXT("Pawn"), NSLOCTEXT("DualFireLoadout", "PawnComponentsInvalid", "THE PLAYER LOADOUT COMPONENTS ARE UNAVAILABLE."));
	}

	FAircraftRow AircraftRow;
	FShieldRow ShieldRow;
	FWeaponRow PrimaryWeaponRow;
	FWeaponRow SpecialWeapon1Row;
	FWeaponRow SpecialWeapon2Row;
	if (!ULoadoutDataLibrary::FindAircraftRow(AircraftDataTable, ActiveLoadout.AircraftID, AircraftRow) ||
		!ULoadoutDataLibrary::FindShieldRow(ShieldDataTable, ActiveLoadout.ShieldID, ShieldRow) ||
		!ULoadoutDataLibrary::FindWeaponRow(WeaponDataTable, ActiveLoadout.PrimaryWeaponID, PrimaryWeaponRow) ||
		!ULoadoutDataLibrary::FindWeaponRow(WeaponDataTable, ActiveLoadout.SpecialWeapon1ID, SpecialWeapon1Row) ||
		!ULoadoutDataLibrary::FindWeaponRow(WeaponDataTable, ActiveLoadout.SpecialWeapon2ID, SpecialWeapon2Row))
	{
		return Fail(TEXT("Loadout"), NSLOCTEXT("DualFireLoadout", "LoadoutResolveFailed", "THE ACTIVE LOADOUT COULD NOT BE RESOLVED."));
	}

	UPaperFlipbook* BankFlipbook = AircraftRow.BankFlipbook.LoadSynchronous();
	if (!IsValid(BankFlipbook))
	{
		return Fail(TEXT("Aircraft"), NSLOCTEXT("DualFireLoadout", "AircraftVisualInvalid", "THE AIRCRAFT VISUAL IS UNAVAILABLE."));
	}

	if (!WeaponComp->TryApplyResolvedLoadout(
		PrimaryWeaponRow,
		SpecialWeapon1Row,
		SpecialWeapon2Row,
		OutError,
		OutInvalidField))
	{
		return false;
	}

	const int32 MaxHealth = AircraftRow.MaxHealth;
	const int32 MaxShield = ShieldRow.MaxShield;
	Pawn->ApplyAircraftVisual(BankFlipbook);
	HealthComp->bUseShield = (MaxShield > 0);
	HealthComp->InitFromData(
		MaxHealth,
		MaxShield,
		ShieldRow.ShieldRecoveryDelay,
		ShieldRow.ShieldRecoveryDuration,
		ShieldRow.BreakInvincibilityDuration);

	UE_LOG(LogDualFire, Log, TEXT("[LoadoutManagerSubsystem] 적용 완료 — HP:%d, Shield:%d"), MaxHealth, MaxShield);
	return true;
}

TSubclassOf<ADualFirePlayerPawn> ULoadoutManagerSubsystem::ResolveAircraftClass() const
{
	if (!bHasActiveLoadout)
	{
		return nullptr;
	}

	FAircraftRow AircraftRow;
	if (!ULoadoutDataLibrary::FindAircraftRow(AircraftDataTable, ActiveLoadout.AircraftID, AircraftRow))
	{
		return nullptr;
	}

	if (AircraftRow.AircraftClass.IsNull())
	{
		return nullptr;
	}

	return AircraftRow.AircraftClass.LoadSynchronous();
}
