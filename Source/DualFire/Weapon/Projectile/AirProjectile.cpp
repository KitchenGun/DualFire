// Copyright DualFire. All Rights Reserved.

#include "Weapon/Projectile/AirProjectile.h"

AAirProjectile::AAirProjectile()
{
	FProjectileRuntimeConfig RuntimeConfig;
	RuntimeConfig.AttributeArray.Add(EDualFireAttribute::Air);
	ApplyRuntimeConfig(RuntimeConfig);
}
