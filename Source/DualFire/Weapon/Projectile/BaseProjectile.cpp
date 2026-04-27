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
}

void ABaseProjectile::BeginPlay()
{
	Super::BeginPlay();

	// 스폰한 폰과의 겹침 무시 (발사구 오프셋이 작을 경우 즉시 자기 히트박스와 겹치는 것 방지)
	if (AActor* InstigatorPawn = GetInstigator())
	{
		CollisionComp->IgnoreActorWhenMoving(InstigatorPawn, true);
	}

	ProjectileMovement->InitialSpeed    = ProjectileSpeed;
	ProjectileMovement->MaxSpeed        = ProjectileSpeed;
	ProjectileMovement->Velocity        = FVector(ProjectileSpeed, 0.f, 0.f);

	SetLifeSpan(LifeSpan);

	CollisionComp->OnComponentBeginOverlap.AddDynamic(this, &ABaseProjectile::OnProjectileOverlapBegin);
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

	// EnemyBody 채널인지 이중 보험 확인
	if (OtherComp->GetCollisionObjectType() != DualFireChannel::EnemyBody)
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
		Destroy();
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
	// 인터페이스 미구현 적 → 보수적으로 일치 처리 (피격 가능)
	if (!OtherActor || !OtherActor->Implements<UEnemyAttributeInterface>())
	{
		return true;
	}

	const EWeaponAttribute EnemyAttr =
		IEnemyAttributeInterface::Execute_GetEnemyAttribute(OtherActor);

	// 탄환 속성 배열 중 하나라도 적 속성과 일치하면 피격
	return Attributes.Contains(EnemyAttr);
}
