// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Projectile/BaseProjectile.h"
#include "GroundProjectile.generated.h"

/** 대지 무장 탄환. AttributeArray=[Ground]. 지상 적에게만 데미지, 공중 적은 관통. */
UCLASS(BlueprintType, Blueprintable)
class DUALFIRE_API AGroundProjectile : public ABaseProjectile
{
	GENERATED_BODY()

public:
	AGroundProjectile();
};
