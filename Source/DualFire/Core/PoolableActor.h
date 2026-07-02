// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PoolableActor.generated.h"

UINTERFACE(BlueprintType)
class DUALFIRE_API UPoolableActor : public UInterface
{
	GENERATED_BODY()
};

class DUALFIRE_API IPoolableActor
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Pool")
	void OnAcquiredFromPool();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Pool")
	void OnReleasedToPool();
};
