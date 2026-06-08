// Copyright DualFire. All Rights Reserved.

#include "Weapon/Projectile/BaseProjectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Core/DualFireCollisionChannels.h"
#include "Enemy/EnemyAttributeInterface.h"
#include "DualFire.h"

ABaseProjectile::ABaseProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComp"));
	CollisionComp->InitSphereRadius(8.f);
	CollisionComp->SetCollisionProfileName(DualFireProfile::PlayerBullet);
	RootComponent = CollisionComp;

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->bRotationFollowsVelocity = false;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 0.f;

	// 기본값: 플레이어 탄 설정 — ApplyRuntimeConfig로 덮어씌울 수 있음
	TargetChannel = DualFireChannel::EnemyBody;
	bUseAttributeMatching = true;
}

void ABaseProjectile::BeginPlay()
{
	Super::BeginPlay();

	// 스폰한 폰과의 겹침 무시 (발사구 오프셋이 작을 경우 즉시 자기 히트박스와 겹치는 것 방지)
	if (AActor* InstigatorPawn = GetInstigator())
	{
		CollisionComp->IgnoreActorWhenMoving(InstigatorPawn, true);
	}

	ProjectileMovement->InitialSpeed = ProjectileSpeed;
	ProjectileMovement->MaxSpeed     = ProjectileSpeed;
	ProjectileMovement->Velocity     = GetActorForwardVector() * ProjectileSpeed;

	SetLifeSpan(LifeSpan);

	CollisionComp->OnComponentBeginOverlap.AddDynamic(this, &ABaseProjectile::OnProjectileOverlapBegin);
}

void ABaseProjectile::ApplyRuntimeConfig(const FProjectileRuntimeConfig& RuntimeConfig)
{
	AttributeArray = RuntimeConfig.AttributeArray;
	Damage = FMath::Max(0.f, RuntimeConfig.Damage);
	ProjectileSpeed = FMath::Max(1.f, RuntimeConfig.ProjectileSpeed);
	HitBehavior = RuntimeConfig.HitBehavior;
	PenetrationLimit = FMath::Max(0, RuntimeConfig.PenetrationLimit);
	bUseAttributeMatching = RuntimeConfig.bUseAttributeMatching;

	// ECC_MAX = 미설정 → 기존 TargetChannel 유지 (플레이어 탄 기본값 보존)
	if (RuntimeConfig.TargetChannel != ECC_MAX)
	{
		TargetChannel = RuntimeConfig.TargetChannel;
	}

	// 콜리전 프로파일 교체 (NAME_None = 미설정 → 기존 프로파일 유지)
	if (!RuntimeConfig.CollisionProfileName.IsNone() && IsValid(CollisionComp))
	{
		CollisionComp->SetCollisionProfileName(RuntimeConfig.CollisionProfileName);
	}

	if (ProjectileMovement)
	{
		ProjectileMovement->InitialSpeed = ProjectileSpeed;
		ProjectileMovement->MaxSpeed     = ProjectileSpeed;

		const FVector Dir = RuntimeConfig.VelocityDirection.IsNearlyZero()
			? GetActorForwardVector()
			: RuntimeConfig.VelocityDirection.GetSafeNormal();
		ProjectileMovement->Velocity = Dir * ProjectileSpeed;
	}
}
void ABaseProjectile::OnProjectileOverlapBegin(
	UPrimitiveComponent* OverlappedComp,
	AActor*              OtherActor,
	UPrimitiveComponent* OtherComp,
	int32                OtherBodyIndex,
	bool                 bFromSweep,
	const FHitResult&    SweepResult)
{
	if (!IsValid(OtherActor) || !IsValid(OtherComp))
	{
		return;
	}

	// 대상 채널이 아니면 무시 (플레이어 탄: EnemyBody, 적 탄: PlayerHitbox)
	if (OtherComp->GetCollisionObjectType() != TargetChannel)
	{
		return;
	}

	// 관통 탄환의 동일 적 중복 피격 방지
	if (AlreadyHitActors.Contains(OtherActor))
	{
		return;
	}
	AlreadyHitActors.Add(OtherActor);

	if (CheckAttributeMatch(OtherActor))
	{
		UGameplayStatics::ApplyDamage(OtherActor, Damage, GetInstigatorController(), this, nullptr);
		OnAttributeMatched(OtherActor, OtherComp);
		switch (HitBehavior)
		{
		case EHitBehavior::Penetrate:
			break;
		case EHitBehavior::LimitedPenetrate:
			++PenetrationCount;
			if (PenetrationLimit <= 0 || PenetrationCount >= PenetrationLimit)
			{
				Destroy();
			}
			break;
		case EHitBehavior::Destroy:
		default:
			Destroy();
			break;
		}
	}
	else
	{
		// 속성 불일치 → 관통 진행
		OnAttributeMismatch(OtherActor, OtherComp);
		UE_LOG(LogDualFire, Verbose, TEXT("[Projectile] %s 관통 — 속성 불일치 (적: %s)"),
			*GetName(), *OtherActor->GetName());
	}
}

bool ABaseProjectile::CheckAttributeMatch(AActor* OtherActor) const
{
	// 속성 매칭 비활성화(적 탄 등) → 무조건 히트
	if (!bUseAttributeMatching)
	{
		return true;
	}

	// Enemies without the interface remain hittable for prototype compatibility.
	if (!OtherActor || !OtherActor->Implements<UEnemyAttributeInterface>())
	{
		return true;
	}
	FEnemyAttribute EnemyAttributes = IEnemyAttributeInterface::Execute_GetEnemyAttributes(OtherActor);
	if (EnemyAttributes.IsNone())
	{
		// Temporary bridge for Blueprint assets that still implement only the legacy enum function.
		EnemyAttributes = FEnemyAttribute::FromAttribute(IEnemyAttributeInterface::Execute_GetEnemyAttribute(OtherActor));
	}
	return FEnemyAttribute::IsMatch(AttributeArray, EnemyAttributes);
}
