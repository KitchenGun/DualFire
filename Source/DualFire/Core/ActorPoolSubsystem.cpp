// Copyright DualFire. All Rights Reserved.

#include "Core/ActorPoolSubsystem.h"

#include "Core/PoolableActor.h"
#include "DualFire.h"
#include "Engine/World.h"

AActor* UActorPoolSubsystem::AcquireActor(TSubclassOf<AActor> Class, const FTransform& SpawnTransform)
{
	if (!IsValid(Class))
	{
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return nullptr;
	}

	FPoolBucket& Bucket = Pools.FindOrAdd(Class.Get());
	while (Bucket.FreeActors.Num() > 0)
	{
		AActor* Actor = Bucket.FreeActors.Pop(EAllowShrinking::No);
		if (IsValid(Actor) && !Actor->IsActorBeingDestroyed())
		{
			ActivateActor(Actor, SpawnTransform);
			return Actor;
		}
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AActor* Actor = World->SpawnActor<AActor>(Class, SpawnTransform, Params);
	if (!IsValid(Actor))
	{
		return nullptr;
	}

	UE_LOG(LogDualFire, Warning, TEXT("[Pool] %s pool exhausted; spawned runtime actor"),
		*Class->GetName());

	ActivateActor(Actor, SpawnTransform);
	return Actor;
}

void UActorPoolSubsystem::ReleaseActor(AActor* Actor)
{
	if (!IsValid(Actor) || Actor->IsActorBeingDestroyed())
	{
		return;
	}

	UClass* Class = Actor->GetClass();
	if (!IsValid(Class))
	{
		return;
	}

	FPoolBucket& Bucket = Pools.FindOrAdd(Class);
	if (Bucket.FreeActors.Contains(Actor))
	{
		return;
	}

	DeactivateActor(Actor);
	Bucket.FreeActors.Add(Actor);
}

void UActorPoolSubsystem::Prewarm(TSubclassOf<AActor> Class, int32 Count)
{
	if (!IsValid(Class) || Count <= 0)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	for (int32 Index = 0; Index < Count; ++Index)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AActor* Actor = World->SpawnActor<AActor>(Class, FTransform::Identity, Params);
		if (IsValid(Actor))
		{
			ReleaseActor(Actor);
		}
	}
}

void UActorPoolSubsystem::ActivateActor(AActor* Actor, const FTransform& SpawnTransform)
{
	if (!IsValid(Actor))
	{
		return;
	}

	Actor->SetActorTransform(SpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);
	Actor->SetActorHiddenInGame(false);
	Actor->SetActorEnableCollision(true);
	Actor->SetActorTickEnabled(true);

	if (Actor->GetClass()->ImplementsInterface(UPoolableActor::StaticClass()))
	{
		IPoolableActor::Execute_OnAcquiredFromPool(Actor);
	}
}

void UActorPoolSubsystem::DeactivateActor(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return;
	}

	if (Actor->GetClass()->ImplementsInterface(UPoolableActor::StaticClass()))
	{
		IPoolableActor::Execute_OnReleasedToPool(Actor);
	}

	Actor->SetLifeSpan(0.0f);
	Actor->SetActorHiddenInGame(true);
	Actor->SetActorEnableCollision(false);
	Actor->SetActorTickEnabled(false);
}
