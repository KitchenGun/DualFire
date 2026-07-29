// Copyright DualFire. All Rights Reserved.

#include "Loadout/LoadoutManagerSubsystem.h"
#include "Player/DualFirePlayerPawn.h"
#include "Health/HealthComponent.h"
#include "Weapon/WeaponComponent.h"
#include "Core/LoadoutDataLibrary.h"
#include "DualFire.h"
#include "PaperFlipbook.h"

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

void ULoadoutManagerSubsystem::SetActiveLoadout(const FLoadout& Loadout)
{
	ActiveLoadout = Loadout;
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

	SetActiveLoadout(Loadout);
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
	if (!ULoadoutDataLibrary::FindAircraftRow(AircraftDataTable, Loadout.AircraftID, AircraftRow))
	{
		return Fail(TEXT("Aircraft"), NSLOCTEXT("DualFireLoadout", "AircraftInvalid", "THE SELECTED AIRCRAFT IS UNAVAILABLE."));
	}

	auto ValidateWeapon = [this, &Fail](const FName Field, const FName WeaponID, const ELoadoutSlot Slot)
	{
		FWeaponRow WeaponRow;
		if (!ULoadoutDataLibrary::FindWeaponRow(WeaponDataTable, WeaponID, WeaponRow) ||
			!ULoadoutDataLibrary::IsWeaponCategoryCompatible(Slot, WeaponRow.Category))
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
	if (!ULoadoutDataLibrary::FindShieldRow(ShieldDataTable, Loadout.ShieldID, ShieldRow))
	{
		return Fail(TEXT("Shield"), NSLOCTEXT("DualFireLoadout", "ShieldInvalid", "THE SELECTED SHIELD IS UNAVAILABLE."));
	}

	return true;
}

bool ULoadoutManagerSubsystem::ApplyToPlayer(ADualFirePlayerPawn* Pawn)
{
	if (!IsValid(Pawn))
	{
		UE_LOG(LogDualFire, Warning, TEXT("[LoadoutManagerSubsystem] ApplyToPlayer: Pawn이 유효하지 않음"));
		return false;
	}

	// ── 무장 적용 ─────────────────────────────────────────────────────────────
	UWeaponComponent* WeaponComp = Pawn->GetWeaponComp();
	if (IsValid(WeaponComp))
	{
		WeaponComp->ApplyLoadout(ActiveLoadout);
	}

	// ── HP / Shield 적용 ──────────────────────────────────────────────────────
	UHealthComponent* HealthComp = Pawn->GetHealthComp();
	if (!IsValid(HealthComp))
	{
		UE_LOG(LogDualFire, Warning, TEXT("[LoadoutManagerSubsystem] HealthComponent 없음"));
		return false;
	}

	// AircraftRow에서 외형과 MaxHealth 조회 (없으면 현재 값 유지)
	int32 MaxHealth = HealthComp->MaxHealth;
	FAircraftRow AircraftRow;
	if (ULoadoutDataLibrary::FindAircraftRow(AircraftDataTable, ActiveLoadout.AircraftID, AircraftRow))
	{
		Pawn->ApplyAircraftVisual(AircraftRow.BankFlipbook.LoadSynchronous());
		MaxHealth = FMath::Max(AircraftRow.MaxHealth, 1);
	}
	else
	{
		UE_LOG(LogDualFire, Warning,
			TEXT("[LoadoutManagerSubsystem] 기체 행을 찾지 못함 — Aircraft:%s"),
			*ActiveLoadout.AircraftID.ToString());
	}

	// ShieldRow에서 Shield 파라미터 조회 (없으면 기본값)
	int32 MaxShield = 0;
	float ShieldRecoveryDuration = 5.0f;
	float BreakInvincibilityDuration = 0.5f;

	FShieldRow ShieldRow;
	if (ULoadoutDataLibrary::FindShieldRow(ShieldDataTable, ActiveLoadout.ShieldID, ShieldRow))
	{
		MaxShield                  = FMath::Max(ShieldRow.MaxShield, 0);
		ShieldRecoveryDuration     = FMath::Max(ShieldRow.ShieldRecoveryDuration, 0.1f);
		BreakInvincibilityDuration = FMath::Max(ShieldRow.BreakInvincibilityDuration, 0.0f);
	}

	HealthComp->bUseShield = (MaxShield > 0);
	HealthComp->InitFromData(MaxHealth, MaxShield, ShieldRecoveryDuration, BreakInvincibilityDuration);

	UE_LOG(LogDualFire, Log, TEXT("[LoadoutManagerSubsystem] 적용 완료 — HP:%d, Shield:%d"), MaxHealth, MaxShield);
	return true;
}

TSubclassOf<ADualFirePlayerPawn> ULoadoutManagerSubsystem::ResolveAircraftClass() const
{
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
