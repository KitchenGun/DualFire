// Copyright DualFire. All Rights Reserved.

#include "Enemy/EnemyAIComponent.h"
#include "Core/ActorPoolSubsystem.h"
#include "Core/DualFireCollisionChannels.h"
#include "DualFire.h"

#include "Engine/World.h"
#include "TimerManager.h"

UEnemyAIComponent::UEnemyAIComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UEnemyAIComponent::BeginPlay()
{
	Super::BeginPlay();

	ResetRuntimeState();
}

void UEnemyAIComponent::UpdateAI(
	float DeltaTime,
	const FBox2D& PlayableBounds,
	const FVector& PlayerLocation,
	bool bPlayerLocationValid)
{
	if (bPlayerLocationValid)
	{
		CachedPlayerLocation = PlayerLocation;
		bHasCachedPlayerLocation = true;
	}

	switch (MovementPattern)
	{
	case EEnemyMovementPattern::Linear:
		TickLinearMovement(DeltaTime);
		break;
	case EEnemyMovementPattern::EnterStop:
		TickEnterStopMovement(DeltaTime);
		break;
	default:
		break;
	}

	if (AActor* Owner = GetOwner())
	{
		if (Owner->GetActorLocation().X < PlayableBounds.Min.X - DespawnMargin)
		{
			ReleaseOwnerToPool();
		}
	}
}

void UEnemyAIComponent::InitFromEnemyRow(const FEnemyRow& Row)
{
	MovementPattern = Row.MovementPattern;
	AttackPattern = Row.AttackPattern;
	ProjectileDamage = FMath::Max(1.0f, static_cast<float>(Row.AttackDamage));
	ProjectileSpeed = FMath::Max(100.0f, Row.EnemyProjectileSpeed);
	AttackInterval = FMath::Max(0.1f, Row.FireInterval);

	ResetRuntimeState();
	StartAttackTimer();
}

void UEnemyAIComponent::ResetRuntimeState()
{
	if (AActor* Owner = GetOwner())
	{
		SpawnLocation = Owner->GetActorLocation();
	}
	bEnterStopReached = false;
	bHasCachedPlayerLocation = false;
}

void UEnemyAIComponent::StartAttackTimer()
{
	StopAttackTimer();

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	if (AttackPattern != EEnemyAttackPattern::None && IsValid(ProjectileClass))
	{
		FTimerDelegate Del;
		Del.BindUObject(this, &UEnemyAIComponent::FireSingle);

		World->GetTimerManager().SetTimer(
			AttackTimerHandle,
			Del,
			AttackInterval,
			true,
			FirstAttackDelay > 0.0f ? FirstAttackDelay : AttackInterval);
	}
}

void UEnemyAIComponent::StopAttackTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AttackTimerHandle);
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
}

void UEnemyAIComponent::TickEnterStopMovement(float DeltaTime)
{
	if (bEnterStopReached)
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!IsValid(Owner))
	{
		return;
	}

	const FVector CurrentLocation = Owner->GetActorLocation();
	const float TargetX = SpawnLocation.X - EnterDistance;
	const float NextX = FMath::Max(CurrentLocation.X - MoveSpeed * DeltaTime, TargetX);
	Owner->SetActorLocation(FVector(NextX, CurrentLocation.Y, CurrentLocation.Z));

	bEnterStopReached = NextX <= TargetX + KINDA_SMALL_NUMBER;
}

void UEnemyAIComponent::ReleaseOwnerToPool()
{
	AActor* Owner = GetOwner();
	if (!IsValid(Owner))
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (UActorPoolSubsystem* Pool = World->GetSubsystem<UActorPoolSubsystem>())
		{
			Pool->ReleaseActor(Owner);
			return;
		}
	}

	Owner->Destroy();
}

FVector UEnemyAIComponent::GetAimDirection(const FVector& ProjectileSpawnLocation) const
{
	if (!bHasCachedPlayerLocation)
	{
		return FVector(-1.0f, 0.0f, 0.0f);
	}

	FVector Dir = CachedPlayerLocation - ProjectileSpawnLocation;
	Dir.Z = 0.0f;
	return Dir.IsNearlyZero() ? FVector(-1.0f, 0.0f, 0.0f) : Dir.GetSafeNormal();
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

	const FVector ProjectileSpawnLocation = Owner->GetActorLocation() + MuzzleOffset;

	ABaseProjectile* Projectile = nullptr;
	if (UActorPoolSubsystem* Pool = World->GetSubsystem<UActorPoolSubsystem>())
	{
		Projectile = Cast<ABaseProjectile>(
			Pool->AcquireActor(ProjectileClass, FTransform(FRotator::ZeroRotator, ProjectileSpawnLocation)));
	}
	else
	{
		FActorSpawnParameters Params;
		Params.Owner = Owner;
		Params.Instigator = Cast<APawn>(Owner);
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Projectile = World->SpawnActor<ABaseProjectile>(
			ProjectileClass, ProjectileSpawnLocation, FRotator::ZeroRotator, Params);
	}

	if (!IsValid(Projectile))
	{
		return;
	}
	Projectile->SetOwner(Owner);
	Projectile->SetInstigator(Cast<APawn>(Owner));

	FProjectileRuntimeConfig Config;
	Config.CollisionProfileName  = DualFireProfile::EnemyBullet;
	Config.TargetChannel         = DualFireChannel::PlayerHitbox;
	Config.bUseAttributeMatching = false;
	Config.VelocityDirection     = GetAimDirection(ProjectileSpawnLocation);
	Config.Damage                = ProjectileDamage;
	Config.ProjectileSpeed       = ProjectileSpeed;
	Config.HitBehavior           = EHitBehavior::Destroy;

	Projectile->ApplyRuntimeConfig(Config);
}
