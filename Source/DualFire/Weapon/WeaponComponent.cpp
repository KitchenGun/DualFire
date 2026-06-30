// Copyright DualFire. All Rights Reserved.

#include "Weapon/WeaponComponent.h"

#include "Core/LoadoutDataLibrary.h"
#include "DualFire.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FName TestAircraftID(TEXT("TEST_SHIP"));
	const FName TestSuperWeaponID(TEXT("TEST_SUPER"));
	const FName TestShieldID(TEXT("TEST_SHIELD"));
	const FName TestGroundWeaponID(TEXT("TEST_AG"));
	const FName TestAirWeaponID(TEXT("TEST_AA"));
	const FName TestUniversalWeaponID(TEXT("TEST_AM"));

	float FireRateFromCooldown(float Cooldown)
	{
		return 1.0f / FMath::Max(0.001f, Cooldown);
	}
}

UWeaponComponent::UWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	PrimaryWeaponSlot.Reset(ELoadoutSlot::PrimaryWeapon);
	SpecialWeapon1Slot.Reset(ELoadoutSlot::SpecialWeapon1);
	SpecialWeapon2Slot.Reset(ELoadoutSlot::SpecialWeapon2);

	static ConstructorHelpers::FObjectFinder<UDataTable> WeaponDataTableFinder(
		TEXT("/Game/Data/Loadout/DT_LoadoutWeapons.DT_LoadoutWeapons"));
	if (WeaponDataTableFinder.Succeeded())
	{
		WeaponDataTable = WeaponDataTableFinder.Object;
	}

	DefaultLoadout.AircraftID = TestAircraftID;
	DefaultLoadout.PrimaryWeaponID = TestGroundWeaponID;
	DefaultLoadout.SpecialWeapon1ID = TestAirWeaponID;
	DefaultLoadout.SpecialWeapon2ID = TestUniversalWeaponID;
	DefaultLoadout.SuperWeaponID = TestSuperWeaponID;
	DefaultLoadout.ShieldID = TestShieldID;
}

void UWeaponComponent::BeginPlay()
{
	Super::BeginPlay();

	ApplyLoadout(DefaultLoadout);
}

bool UWeaponComponent::ApplyLoadout(const FLoadout& Loadout)
{
	FLoadout LoadoutToApply = Loadout;
	if (LoadoutToApply.PrimaryWeaponID.IsNone())
	{
		LoadoutToApply.PrimaryWeaponID = TestGroundWeaponID;
	}
	if (LoadoutToApply.SpecialWeapon1ID.IsNone())
	{
		LoadoutToApply.SpecialWeapon1ID = TestAirWeaponID;
	}
	if (LoadoutToApply.SpecialWeapon2ID.IsNone())
	{
		LoadoutToApply.SpecialWeapon2ID = TestUniversalWeaponID;
	}

	ActiveLoadout = LoadoutToApply;

	const bool bPrimaryEquipped = EquipWeaponSlot(
		ELoadoutSlot::PrimaryWeapon,
		LoadoutToApply.PrimaryWeaponID,
		GroundProjectileClass);

	const bool bSpecial1Equipped = EquipWeaponSlot(
		ELoadoutSlot::SpecialWeapon1,
		LoadoutToApply.SpecialWeapon1ID,
		AirProjectileClass);

	const bool bSpecial2Equipped = EquipWeaponSlot(
		ELoadoutSlot::SpecialWeapon2,
		LoadoutToApply.SpecialWeapon2ID,
		UniversalProjectileClass);

	return bPrimaryEquipped && bSpecial1Equipped && bSpecial2Equipped;
}

void UWeaponComponent::FireGround()
{
	FireLoadoutSlot(ELoadoutSlot::PrimaryWeapon);
}

void UWeaponComponent::FireAir()
{
	FireLoadoutSlot(ELoadoutSlot::SpecialWeapon1);
}

void UWeaponComponent::FireUniversal()
{
	FireLoadoutSlot(ELoadoutSlot::SpecialWeapon2);
}

bool UWeaponComponent::CanFireGround() const
{
	return CanFireLoadoutSlot(ELoadoutSlot::PrimaryWeapon);
}

bool UWeaponComponent::CanFireAir() const
{
	return CanFireLoadoutSlot(ELoadoutSlot::SpecialWeapon1);
}

bool UWeaponComponent::CanFireUniversal() const
{
	return CanFireLoadoutSlot(ELoadoutSlot::SpecialWeapon2);
}

void UWeaponComponent::FireLoadoutSlot(ELoadoutSlot Slot)
{
	FWeaponSlotState* SlotState = GetWeaponSlotState(Slot);
	if (!SlotState || SlotState->bCooldownActive)
	{
		return;
	}

	if (!IsValid(SlotState->ProjectileClass))
	{
		UE_LOG(LogDualFire, Warning, TEXT("[WeaponComp] No projectile class for weapon %s"), *SlotState->WeaponID.ToString());
		return;
	}

	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!IsValid(Owner) || !IsValid(World))
	{
		return;
	}

	const FVector SpawnLocation = Owner->GetActorLocation() + MuzzleOffset + SlotState->WeaponData.MuzzleOffset;
	const FRotator SpawnRotation = FRotator::ZeroRotator;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Instigator = Cast<APawn>(Owner);
	SpawnParams.Owner = Owner;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ABaseProjectile* Projectile = World->SpawnActor<ABaseProjectile>(
		SlotState->ProjectileClass,
		SpawnLocation,
		SpawnRotation,
		SpawnParams);

	if (!IsValid(Projectile))
	{
		return;
	}

	Projectile->ApplyRuntimeConfig(ULoadoutDataLibrary::MakeProjectileRuntimeConfig(SlotState->WeaponData));

	SlotState->bCooldownActive = true;

	FTimerDelegate CooldownDelegate;
	CooldownDelegate.BindUObject(this, &UWeaponComponent::OnLoadoutSlotCooldownExpired, Slot);

	World->GetTimerManager().SetTimer(
		SlotState->CooldownHandle,
		CooldownDelegate,
		GetCooldownFromFireRate(SlotState->WeaponData.FireRate),
		false);

	UE_LOG(LogDualFire, Verbose, TEXT("[WeaponComp] Fired %s"), *SlotState->WeaponID.ToString());
}

bool UWeaponComponent::CanFireLoadoutSlot(ELoadoutSlot Slot) const
{
	const FWeaponSlotState* SlotState = GetWeaponSlotState(Slot);
	return SlotState && !SlotState->bCooldownActive && IsValid(SlotState->ProjectileClass);
}

FWeaponSlotState* UWeaponComponent::GetWeaponSlotState(ELoadoutSlot Slot)
{
	switch (Slot)
	{
	case ELoadoutSlot::PrimaryWeapon:
		return &PrimaryWeaponSlot;
	case ELoadoutSlot::SpecialWeapon1:
		return &SpecialWeapon1Slot;
	case ELoadoutSlot::SpecialWeapon2:
		return &SpecialWeapon2Slot;
	default:
		return nullptr;
	}
}

const FWeaponSlotState* UWeaponComponent::GetWeaponSlotState(ELoadoutSlot Slot) const
{
	switch (Slot)
	{
	case ELoadoutSlot::PrimaryWeapon:
		return &PrimaryWeaponSlot;
	case ELoadoutSlot::SpecialWeapon1:
		return &SpecialWeapon1Slot;
	case ELoadoutSlot::SpecialWeapon2:
		return &SpecialWeapon2Slot;
	default:
		return nullptr;
	}
}

bool UWeaponComponent::EquipWeaponSlot(ELoadoutSlot Slot, FName WeaponID, TSubclassOf<ABaseProjectile> LegacyProjectileClass)
{
	FWeaponSlotState* SlotState = GetWeaponSlotState(Slot);
	if (!SlotState)
	{
		return false;
	}

	SlotState->Reset(Slot);

	FWeaponRow WeaponRow;
	if (!ResolveWeaponRow(WeaponID, WeaponRow))
	{
		UE_LOG(LogDualFire, Warning, TEXT("[WeaponComp] Weapon row not found: %s"), *WeaponID.ToString());
		return false;
	}

	if (!ULoadoutDataLibrary::IsWeaponCategoryCompatible(Slot, WeaponRow.Category))
	{
		UE_LOG(LogDualFire, Warning, TEXT("[WeaponComp] Weapon category mismatch: %s"), *WeaponID.ToString());
		return false;
	}

	const TSubclassOf<ABaseProjectile> ResolvedProjectileClass = ResolveProjectileClass(WeaponRow, LegacyProjectileClass);
	if (!IsValid(ResolvedProjectileClass))
	{
		UE_LOG(LogDualFire, Warning, TEXT("[WeaponComp] Projectile class not found: %s"), *WeaponID.ToString());
		return false;
	}

	SlotState->WeaponID = WeaponID;
	SlotState->WeaponData = WeaponRow;
	SlotState->ProjectileClass = ResolvedProjectileClass;
	return true;
}

bool UWeaponComponent::ResolveWeaponRow(FName WeaponID, FWeaponRow& OutWeaponRow) const
{
	if (ULoadoutDataLibrary::FindWeaponRow(WeaponDataTable, WeaponID, OutWeaponRow))
	{
		return true;
	}

	return BuildDefaultTestWeaponRow(WeaponID, OutWeaponRow);
}

bool UWeaponComponent::BuildDefaultTestWeaponRow(FName WeaponID, FWeaponRow& OutWeaponRow) const
{
	OutWeaponRow = FWeaponRow();
	OutWeaponRow.WeaponID = WeaponID;
	OutWeaponRow.BaseDamage = 10;
	OutWeaponRow.ProjectileSpeed = 1200.0f;
	OutWeaponRow.HitBehavior = EHitBehavior::Destroy;
	OutWeaponRow.PenetrationLimit = 0;
	OutWeaponRow.UnlockID = TEXT("DEFAULT");

	if (WeaponID == TestGroundWeaponID)
	{
		OutWeaponRow.DisplayName = FText::FromString(TEXT("Test AG Weapon"));
		OutWeaponRow.Category = EWeaponCategory::Primary;
		OutWeaponRow.AttributeArray = { EDualFireAttribute::Ground };
		OutWeaponRow.FireRate = FireRateFromCooldown(GroundFireCooldown);
		OutWeaponRow.CategoryTag = TEXT("Ground");
		return true;
	}

	if (WeaponID == TestAirWeaponID)
	{
		OutWeaponRow.DisplayName = FText::FromString(TEXT("Test AA Weapon"));
		OutWeaponRow.Category = EWeaponCategory::Special;
		OutWeaponRow.AttributeArray = { EDualFireAttribute::Air };
		OutWeaponRow.FireRate = FireRateFromCooldown(AirFireCooldown);
		OutWeaponRow.CategoryTag = TEXT("Air");
		return true;
	}

	if (WeaponID == TestUniversalWeaponID)
	{
		OutWeaponRow.DisplayName = FText::FromString(TEXT("Test AM Weapon"));
		OutWeaponRow.Category = EWeaponCategory::Special;
		OutWeaponRow.AttributeArray = { EDualFireAttribute::Ground, EDualFireAttribute::Air };
		OutWeaponRow.FireRate = FireRateFromCooldown(UniversalFireCooldown);
		OutWeaponRow.CategoryTag = TEXT("Universal");
		return true;
	}

	return false;
}

TSubclassOf<ABaseProjectile> UWeaponComponent::ResolveProjectileClass(
	const FWeaponRow& WeaponRow,
	TSubclassOf<ABaseProjectile> LegacyProjectileClass) const
{
	if (!WeaponRow.ProjectileClass.IsNull())
	{
		if (UClass* LoadedClass = WeaponRow.ProjectileClass.LoadSynchronous())
		{
			if (LoadedClass->IsChildOf(ABaseProjectile::StaticClass()))
			{
				return LoadedClass;
			}
		}
	}

	return LegacyProjectileClass;
}

float UWeaponComponent::GetCooldownFromFireRate(float FireRate) const
{
	return 1.0f / FMath::Max(0.001f, FireRate);
}

void UWeaponComponent::OnLoadoutSlotCooldownExpired(ELoadoutSlot Slot)
{
	if (FWeaponSlotState* SlotState = GetWeaponSlotState(Slot))
	{
		SlotState->bCooldownActive = false;
	}
}
