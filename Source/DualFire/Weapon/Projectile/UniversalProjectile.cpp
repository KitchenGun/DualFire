// Copyright DualFire. All Rights Reserved.

#include "Weapon/Projectile/UniversalProjectile.h"

AUniversalProjectile::AUniversalProjectile()
{
	FProjectileRuntimeConfig RuntimeConfig;
	RuntimeConfig.AttributeArray.Add(EDualFireAttribute::Ground);
	RuntimeConfig.AttributeArray.Add(EDualFireAttribute::Air);
	ApplyRuntimeConfig(RuntimeConfig);
}
