// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Projectile/BaseProjectile.h"
#include "UniversalProjectile.generated.h"

/** 범용 무장 탄환. AttributeArray=[Ground, Air]. 지상/공중 모든 적에게 데미지. */
UCLASS(BlueprintType, Blueprintable)
class DUALFIRE_API AUniversalProjectile : public ABaseProjectile
{
	GENERATED_BODY()

public:
	AUniversalProjectile();
};
