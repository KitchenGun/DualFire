// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Weapon/Projectile/BaseProjectile.h"
#include "WeaponComponent.generated.h"

/**
 * 플레이어 무장 컴포넌트.
 * 대지(Ground) / 대공(Air) / 범용(Universal) 3개 슬롯을 독립적으로 관리.
 * 각 슬롯은 별도의 쿨다운을 가지므로 한 슬롯이 쿨다운 중에도 다른 슬롯 발사 가능.
 */
UCLASS(ClassGroup=Weapon, meta=(BlueprintSpawnableComponent))
class DUALFIRE_API UWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWeaponComponent();

	// ── 슬롯별 투사체 클래스 (BP_PlayerPawn에서 할당) ──────────────────────────

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Ground")
	TSubclassOf<ABaseProjectile> GroundProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Air")
	TSubclassOf<ABaseProjectile> AirProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Universal")
	TSubclassOf<ABaseProjectile> UniversalProjectileClass;

	// ── 슬롯별 발사 쿨다운 (초) ────────────────────────────────────────────────

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Weapon|Ground",
		meta=(ClampMin="0.05"))
	float GroundFireCooldown = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Weapon|Air",
		meta=(ClampMin="0.05"))
	float AirFireCooldown = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Weapon|Universal",
		meta=(ClampMin="0.05"))
	float UniversalFireCooldown = 0.15f;

	/** 발사구 오프셋. Owner 위치 기준 +X 방향으로 탄환이 스폰되는 위치 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Weapon")
	FVector MuzzleOffset = FVector(50.f, 0.f, 0.f);

	// ── 발사 메서드 ────────────────────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void FireGround();

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void FireAir();

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void FireUniversal();

	// ── 쿨다운 쿼리 ────────────────────────────────────────────────────────────

	UFUNCTION(BlueprintPure, Category="Weapon")
	bool CanFireGround() const { return !bGroundCooldown && IsValid(GroundProjectileClass); }

	UFUNCTION(BlueprintPure, Category="Weapon")
	bool CanFireAir() const { return !bAirCooldown && IsValid(AirProjectileClass); }

	UFUNCTION(BlueprintPure, Category="Weapon")
	bool CanFireUniversal() const { return !bUniversalCooldown && IsValid(UniversalProjectileClass); }

private:
	bool bGroundCooldown    = false;
	bool bAirCooldown       = false;
	bool bUniversalCooldown = false;

	FTimerHandle GroundCooldownHandle;
	FTimerHandle AirCooldownHandle;
	FTimerHandle UniversalCooldownHandle;

	/** 공통 발사 로직. 슬롯별 ProjectileClass/쿨다운 상태/쿨다운 시간을 받아 처리 */
	void FireSlot(
		TSubclassOf<ABaseProjectile> ProjectileClass,
		FTimerHandle&                CooldownHandle,
		bool&                        bCooldownActive,
		float                        Cooldown);

	void OnGroundCooldownExpired();
	void OnAirCooldownExpired();
	void OnUniversalCooldownExpired();
};
