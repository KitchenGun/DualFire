// Copyright DualFire. All Rights Reserved.

#include "Weapon/Projectile/GroundProjectile.h"

AGroundProjectile::AGroundProjectile()
{
	FProjectileRuntimeConfig RuntimeConfig;
	RuntimeConfig.AttributeArray.Add(EDualFireAttribute::Ground);
	ApplyRuntimeConfig(RuntimeConfig);
}
