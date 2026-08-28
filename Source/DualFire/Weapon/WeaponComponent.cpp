// Copyright DualFire. All Rights Reserved.

#include "Weapon/WeaponComponent.h"

#include "Core/ActorPoolSubsystem.h"
#include "Core/LoadoutDataLibrary.h"
#include "DualFire.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "TimerManager.h"

UWeaponComponent::UWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	PrimaryWeaponSlot.Reset();
	SpecialWeapon1Slot.Reset();
	SpecialWeapon2Slot.Reset();
}

bool UWeaponComponent::TryApplyResolvedLoadout(
	const FWeaponRow& PrimaryWeaponRow,
	const FWeaponRow& SpecialWeapon1Row,
	const FWeaponRow& SpecialWeapon2Row,
	FText& OutError,
	FName& OutInvalidField)
{
	FWeaponSlotState NewPrimaryWeaponSlot;
	FWeaponSlotState NewSpecialWeapon1Slot;
	FWeaponSlotState NewSpecialWeapon2Slot;

	if (!BuildWeaponSlotState(
		ELoadoutSlot::PrimaryWeapon,
		TEXT("PrimaryWeapon"),
		PrimaryWeaponRow.WeaponID,
		PrimaryWeaponRow,
		NewPrimaryWeaponSlot,
		OutError,
		OutInvalidField) ||
		!BuildWeaponSlotState(
		ELoadoutSlot::SpecialWeapon1,
		TEXT("SpecialWeapon1"),
		SpecialWeapon1Row.WeaponID,
		SpecialWeapon1Row,
		NewSpecialWeapon1Slot,
		OutError,
		OutInvalidField) ||
		!BuildWeaponSlotState(
		ELoadoutSlot::SpecialWeapon2,
		TEXT("SpecialWeapon2"),
		SpecialWeapon2Row.WeaponID,
		SpecialWeapon2Row,
		NewSpecialWeapon2Slot,
		OutError,
		OutInvalidField))
	{
		return false;
	}

	ResetCooldowns();
	PrimaryWeaponSlot = MoveTemp(NewPrimaryWeaponSlot);
	SpecialWeapon1Slot = MoveTemp(NewSpecialWeapon1Slot);
	SpecialWeapon2Slot = MoveTemp(NewSpecialWeapon2Slot);
	return true;
}

void UWeaponComponent::FirePrimary()
{
	FireLoadoutSlot(ELoadoutSlot::PrimaryWeapon);
}

void UWeaponComponent::FireSpecial1()
{
	FireLoadoutSlot(ELoadoutSlot::SpecialWeapon1);
}

void UWeaponComponent::FireSpecial2()
{
	FireLoadoutSlot(ELoadoutSlot::SpecialWeapon2);
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

	ABaseProjectile* Projectile = nullptr;
	if (UActorPoolSubsystem* Pool = World->GetSubsystem<UActorPoolSubsystem>())
	{
		Projectile = Cast<ABaseProjectile>(
			Pool->AcquireActor(SlotState->ProjectileClass, FTransform(SpawnRotation, SpawnLocation)));
	}
	else
	{
		Projectile = World->SpawnActor<ABaseProjectile>(
			SlotState->ProjectileClass,
			SpawnLocation,
			SpawnRotation,
			SpawnParams);
	}

	if (!IsValid(Projectile))
	{
		return;
	}
	Projectile->SetOwner(Owner);
	Projectile->SetInstigator(Cast<APawn>(Owner));

	Projectile->ApplyRuntimeConfig(ULoadoutDataLibrary::MakeProjectileRuntimeConfig(SlotState->WeaponData));

	SlotState->bCooldownActive = true;

	FTimerDelegate CooldownDelegate;
	CooldownDelegate.BindUObject(this, &UWeaponComponent::OnLoadoutSlotCooldownExpired, Slot);

	World->GetTimerManager().SetTimer(
		SlotState->CooldownHandle,
		CooldownDelegate,
		1.0f / SlotState->WeaponData.FireRate,
		false);

	UE_LOG(LogDualFire, Verbose, TEXT("[WeaponComp] Fired %s"), *SlotState->WeaponID.ToString());

#if !UE_BUILD_SHIPPING
	// 발사 중인 무장을 로그가 아닌 화면에 표시 — 어떤 슬롯이 무엇을 쐈는지 즉시 육안 식별 가능하도록.
	// 슬롯별 고정 키(연사 중 줄 쌓임 방지) + 슬롯별 색상으로 구분.
	if (GEngine)
	{
		static const int32 OnScreenMessageKeyBase = 5000;
		FColor MessageColor = FColor::White;
		switch (Slot)
		{
		case ELoadoutSlot::PrimaryWeapon:  MessageColor = FColor::Cyan;   break;
		case ELoadoutSlot::SpecialWeapon1: MessageColor = FColor::Yellow; break;
		case ELoadoutSlot::SpecialWeapon2: MessageColor = FColor::Orange; break;
		default: break;
		}

		GEngine->AddOnScreenDebugMessage(
			OnScreenMessageKeyBase + static_cast<int32>(Slot),
			1.0f,
			MessageColor,
			FString::Printf(TEXT("[%s] Fired %s"),
				*UEnum::GetDisplayValueAsText(Slot).ToString(),
				*SlotState->WeaponID.ToString()));
	}
#endif
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

bool UWeaponComponent::BuildWeaponSlotState(
	ELoadoutSlot Slot,
	FName Field,
	FName WeaponID,
	const FWeaponRow& WeaponRow,
	FWeaponSlotState& OutState,
	FText& OutError,
	FName& OutInvalidField) const
{
	auto Fail = [&OutError, &OutInvalidField, Field](const FText& Message)
	{
		OutError = Message;
		OutInvalidField = Field;
		return false;
	};

	if (!ULoadoutDataLibrary::IsWeaponCategoryCompatible(Slot, WeaponRow.Category))
	{
		return Fail(NSLOCTEXT("DualFireLoadout", "WeaponCategoryInvalid", "THE WEAPON CATEGORY DOES NOT MATCH ITS SLOT."));
	}

	if (WeaponRow.FireRate <= 0.f || WeaponRow.ProjectileSpeed <= 0.f || WeaponRow.AttributeArray.IsEmpty())
	{
		return Fail(NSLOCTEXT("DualFireLoadout", "WeaponRuntimeDataInvalid", "THE WEAPON RUNTIME DATA IS INVALID."));
	}

	UClass* ProjectileClass = WeaponRow.ProjectileClass.IsNull()
		? nullptr
		: WeaponRow.ProjectileClass.LoadSynchronous();
	if (!IsValid(ProjectileClass) || !ProjectileClass->IsChildOf(ABaseProjectile::StaticClass()))
	{
		return Fail(NSLOCTEXT("DualFireLoadout", "WeaponProjectileInvalid", "THE WEAPON PROJECTILE CLASS IS INVALID."));
	}

	OutState.Reset();
	OutState.WeaponID = WeaponID;
	OutState.WeaponData = WeaponRow;
	OutState.ProjectileClass = ProjectileClass;
	return true;
}

void UWeaponComponent::ResetCooldowns()
{
	UWorld* World = GetWorld();
	if (IsValid(World))
	{
		World->GetTimerManager().ClearTimer(PrimaryWeaponSlot.CooldownHandle);
		World->GetTimerManager().ClearTimer(SpecialWeapon1Slot.CooldownHandle);
		World->GetTimerManager().ClearTimer(SpecialWeapon2Slot.CooldownHandle);
	}
	PrimaryWeaponSlot.bCooldownActive = false;
	SpecialWeapon1Slot.bCooldownActive = false;
	SpecialWeapon2Slot.bCooldownActive = false;
}

void UWeaponComponent::OnLoadoutSlotCooldownExpired(ELoadoutSlot Slot)
{
	if (FWeaponSlotState* SlotState = GetWeaponSlotState(Slot))
	{
		SlotState->bCooldownActive = false;
	}
}
