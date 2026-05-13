// Copyright DualFire. All Rights Reserved.

#include "Weapon/Projectile/UniversalProjectile.h"

AUniversalProjectile::AUniversalProjectile()
{
	AttributeArray.Add(EDualFireAttribute::Ground);
	AttributeArray.Add(EDualFireAttribute::Air);
}
