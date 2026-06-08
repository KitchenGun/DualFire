// Copyright DualFire. All Rights Reserved.

#include "Enemy/EnemyAIComponent.h"
#include "Core/DualFireCollisionChannels.h"
#include "DualFire.h"

#include "Engine/World.h"
#include "TimerManager.h"

UEnemyAIComponent::UEnemyAIComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UEnemyAIComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AttackPattern != EEnemyAttackPattern::None && IsValid(ProjectileClass))
	{
		FTimerDelegate Del;
		Del.BindUObject(this, &UEnemyAIComponent::FireSingle);

		GetWorld()->GetTimerManager().SetTimer(
			AttackTimerHandle,
			Del,
			AttackInterval,
			true,
			FirstAttackDelay > 0.0f ? FirstAttackDelay : AttackInterval);
	}
}

void UEnemyAIComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	switch (MovementPattern)
	{
	case EEnemyMovementPattern::Linear:
		TickLinearMovement(DeltaTime);
		break;
	default:
		break;
	}
}

void UEnemyAIComponent::TickLinearMovement(float DeltaTime)
{
	AActor* Owner = GetOwner();
	if (!IsValid(Owner))
	{
		return;
	}

	Owner->AddActorWorldOffset(FVector(-MoveSpeed * DeltaTime, 0.0f, 0.0f));

	if (Owner->GetActorLocation().X < DestroyBelowX)
	{
		Owner->Destroy();
	}
}

void UEnemyAIComponent::FireSingle()
{
	if (AttackPattern == EEnemyAttackPattern::None || !IsValid(ProjectileClass))
	{
		return;
	}

	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!IsValid(Owner) || !IsValid(World))
	{
		return;
	}

	const FVector SpawnLocation = Owner->GetActorLocation() + MuzzleOffset;

	FActorSpawnParameters Params;
	Params.Owner     = Owner;
	Params.Instigator = Cast<APawn>(Owner);
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ABaseProjectile* Projectile = World->SpawnActor<ABaseProjectile>(
		ProjectileClass, SpawnLocation, FRotator::ZeroRotator, Params);

	if (!IsValid(Projectile))
	{
		return;
	}

	FProjectileRuntimeConfig Config;
	Config.CollisionProfileName  = DualFireProfile::EnemyBullet;
	Config.TargetChannel         = DualFireChannel::PlayerHitbox;
	Config.bUseAttributeMatching = false;
	Config.VelocityDirection     = FVector(-1.0f, 0.0f, 0.0f);
	Config.Damage                = ProjectileDamage;
	Config.ProjectileSpeed       = ProjectileSpeed;
	Config.HitBehavior           = EHitBehavior::Destroy;

	Projectile->ApplyRuntimeConfig(Config);
}
