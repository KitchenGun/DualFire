// Copyright DualFire. All Rights Reserved.

#include "Weapon/WeaponComponent.h"
#include "Weapon/Projectile/BaseProjectile.h"
#include "DualFire.h"

UWeaponComponent::UWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// ── 공개 발사 메서드 ─────────────────────────────────────────────────────────

void UWeaponComponent::FireGround()
{
	FireSlot(GroundProjectileClass, GroundCooldownHandle, bGroundCooldown, GroundFireCooldown);
}

void UWeaponComponent::FireAir()
{
	FireSlot(AirProjectileClass, AirCooldownHandle, bAirCooldown, AirFireCooldown);
}

void UWeaponComponent::FireUniversal()
{
	FireSlot(UniversalProjectileClass, UniversalCooldownHandle, bUniversalCooldown, UniversalFireCooldown);
}

// ── 공통 발사 로직 ───────────────────────────────────────────────────────────

void UWeaponComponent::FireSlot(
	TSubclassOf<ABaseProjectile> ProjectileClass,
	FTimerHandle&                CooldownHandle,
	bool&                        bCooldownActive,
	float                        Cooldown)
{
	if (bCooldownActive)
	{
		return;
	}

	if (!IsValid(ProjectileClass))
	{
		UE_LOG(LogDualFire, Warning, TEXT("[WeaponComp] ProjectileClass가 설정되지 않아 발사 불가"));
		return;
	}

	AActor* Owner = GetOwner();
	if (!IsValid(Owner))
	{
		return;
	}

	const FVector  SpawnLocation = Owner->GetActorLocation() + MuzzleOffset;
	const FRotator SpawnRotation = FRotator::ZeroRotator;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Instigator             = Cast<APawn>(Owner);
	SpawnParams.Owner                  = Owner;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ABaseProjectile* Projectile = GetWorld()->SpawnActor<ABaseProjectile>(
		ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);

	if (IsValid(Projectile))
	{
		UE_LOG(LogDualFire, Verbose, TEXT("[WeaponComp] 발사 — %s"), *ProjectileClass->GetName());
	}

	bCooldownActive = true;

	// 람다 대신 명시적 콜백 사용 (UObject 안전)
	if (&CooldownHandle == &GroundCooldownHandle)
	{
		GetWorld()->GetTimerManager().SetTimer(
			CooldownHandle, this, &UWeaponComponent::OnGroundCooldownExpired, Cooldown, false);
	}
	else if (&CooldownHandle == &AirCooldownHandle)
	{
		GetWorld()->GetTimerManager().SetTimer(
			CooldownHandle, this, &UWeaponComponent::OnAirCooldownExpired, Cooldown, false);
	}
	else
	{
		GetWorld()->GetTimerManager().SetTimer(
			CooldownHandle, this, &UWeaponComponent::OnUniversalCooldownExpired, Cooldown, false);
	}
}

void UWeaponComponent::OnGroundCooldownExpired()    { bGroundCooldown    = false; }
void UWeaponComponent::OnAirCooldownExpired()       { bAirCooldown       = false; }
void UWeaponComponent::OnUniversalCooldownExpired() { bUniversalCooldown = false; }
