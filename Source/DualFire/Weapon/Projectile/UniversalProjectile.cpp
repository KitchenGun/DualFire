// Copyright DualFire. All Rights Reserved.

#include "Weapon/Projectile/UniversalProjectile.h"

AUniversalProjectile::AUniversalProjectile()
{
	Attributes.Add(EWeaponAttribute::Ground);
	Attributes.Add(EWeaponAttribute::Air);
}
