// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ActorPoolSubsystem.generated.h"

USTRUCT()
struct DUALFIRE_API FPoolBucket
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<AActor>> FreeActors;
};

UCLASS()
class DUALFIRE_API UActorPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Pool")
	AActor* AcquireActor(TSubclassOf<AActor> Class, const FTransform& SpawnTransform);

	UFUNCTION(BlueprintCallable, Category="Pool")
	void ReleaseActor(AActor* Actor);

	UFUNCTION(BlueprintCallable, Category="Pool")
	void Prewarm(TSubclassOf<AActor> Class, int32 Count);

private:
	UPROPERTY()
	TMap<TObjectPtr<UClass>, FPoolBucket> Pools;

	void ActivateActor(AActor* Actor, const FTransform& SpawnTransform);
	void DeactivateActor(AActor* Actor);
};
