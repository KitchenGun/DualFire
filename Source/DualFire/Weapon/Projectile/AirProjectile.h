// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Projectile/BaseProjectile.h"
#include "AirProjectile.generated.h"

/** 대공 무장 탄환. Attributes=[Air]. 공중 적에게만 데미지, 지상 적은 관통. */
UCLASS(BlueprintType, Blueprintable)
class DUALFIRE_API AAirProjectile : public ABaseProjectile
{
	GENERATED_BODY()

public:
	AAirProjectile();
};
