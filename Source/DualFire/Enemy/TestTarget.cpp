// Copyright DualFire. All Rights Reserved.

#include "Enemy/TestTarget.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/DualFireCollisionChannels.h"
#include "DualFire.h"

ATestTarget::ATestTarget()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComp"));
	CollisionComp->InitSphereRadius(48.f);
	CollisionComp->SetCollisionProfileName(DualFireProfile::EnemyBody);
	RootComponent = CollisionComp;

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(CollisionComp);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

float ATestTarget::TakeDamage(
	float                DamageAmount,
	const FDamageEvent&  DamageEvent,
	AController*         EventInstigator,
	AActor*              DamageCauser)
{
	const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	Health -= DamageAmount;
	UE_LOG(LogDualFire, Log, TEXT("[TestTarget %s] 피격 -%.1f → 잔여 HP %.1f"),
		*GetName(), DamageAmount, Health);

	if (Health <= 0.f)
	{
		UE_LOG(LogDualFire, Log, TEXT("[TestTarget %s] 격파"), *GetName());
		Destroy();
	}

	return Applied;
}
