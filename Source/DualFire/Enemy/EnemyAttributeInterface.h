// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Core/DualFireTypes.h"
#include "EnemyAttributeInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UEnemyAttributeInterface : public UInterface
{
	GENERATED_BODY()
};

/** 적과 보스 파츠의 Ground/Air 속성을 탄환 판정에 제공한다. */
class DUALFIRE_API IEnemyAttributeInterface
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Weapon")
	FEnemyAttribute GetEnemyAttributes() const;
	virtual FEnemyAttribute GetEnemyAttributes_Implementation() const
	{
		return {};
	}
};
