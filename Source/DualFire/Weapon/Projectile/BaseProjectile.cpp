// Copyright DualFire. All Rights Reserved.

#include "Weapon/Projectile/BaseProjectile.h"
#include "Core/ActorPoolSubsystem.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"
#include "Core/DualFireCollisionChannels.h"
#include "Enemy/EnemyAttributeInterface.h"
#include "DualFire.h"
#include "UObject/ConstructorHelpers.h"

ABaseProjectile::ABaseProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComp"));
	CollisionComp->InitSphereRadius(8.f);
	CollisionComp->SetCollisionProfileName(DualFireProfile::PlayerBullet);
	CollisionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CollisionComp->SetGenerateOverlapEvents(false);
	RootComponent = CollisionComp;

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->bRotationFollowsVelocity = false;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 0.f;

	// 육안 식별용 구체 — 콜리전은 CollisionComp가 전담하므로 여기서는 비활성
	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(CollisionComp);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetCastShadow(false);
	// 엔진 기본 구체(반지름 50) — CollisionComp 반지름(8)에 맞춰 축소
	VisualMesh->SetRelativeScale3D(FVector(0.16f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshFinder(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMeshFinder.Succeeded())
	{
		VisualMesh->SetStaticMesh(SphereMeshFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> VisualMaterialFinder(
		TEXT("/Game/Blueprint/Weapon/M_ProjectileUnlit.M_ProjectileUnlit"));
	if (VisualMaterialFinder.Succeeded())
	{
		VisualMaterial = VisualMaterialFinder.Object;
	}

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

	ApplyVisualColor();

	SetLifeSpan(LifeSpan);

	CollisionComp->OnComponentBeginOverlap.AddDynamic(this, &ABaseProjectile::OnProjectileOverlapBegin);
}

void ABaseProjectile::LifeSpanExpired()
{
	ReturnToPoolOrDestroy();
}

void ABaseProjectile::OnAcquiredFromPool_Implementation()
{
	AlreadyHitActors.Reset();
	PenetrationCount = 0;

	if (CollisionComp)
	{
		CollisionComp->SetGenerateOverlapEvents(false);
		CollisionComp->ClearMoveIgnoreActors();
	}

	if (ProjectileMovement)
	{
		ProjectileMovement->Activate(true);
		ProjectileMovement->StopMovementImmediately();
	}

	SetLifeSpan(LifeSpan);
}

void ABaseProjectile::OnReleasedToPool_Implementation()
{
	AlreadyHitActors.Reset();
	PenetrationCount = 0;
	SetLifeSpan(0.0f);

	if (CollisionComp)
	{
		CollisionComp->SetGenerateOverlapEvents(false);
		CollisionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CollisionComp->ClearMoveIgnoreActors();
	}

	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->Deactivate();
	}
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

	if (CollisionComp)
	{
		CollisionComp->ClearMoveIgnoreActors();
		if (AActor* InstigatorPawn = GetInstigator())
		{
			CollisionComp->IgnoreActorWhenMoving(InstigatorPawn, true);
		}
		CollisionComp->SetGenerateOverlapEvents(true);
		CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
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

bool ABaseProjectile::ClearForPlayerRespawn()
{
	if (TargetChannel != DualFireChannel::PlayerHitbox || IsHidden() ||
		!IsValid(CollisionComp) ||
		CollisionComp->GetCollisionEnabled() == ECollisionEnabled::NoCollision)
	{
		return false;
	}

	ReturnToPoolOrDestroy();
	return true;
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
				ReturnToPoolOrDestroy();
			}
			break;
		case EHitBehavior::Destroy:
		default:
			ReturnToPoolOrDestroy();
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

	if (!OtherActor || !OtherActor->Implements<UEnemyAttributeInterface>())
	{
		return false;
	}
	return IEnemyAttributeInterface::Execute_GetEnemyAttributes(OtherActor).MatchesAny(AttributeArray);
}

void ABaseProjectile::ReturnToPoolOrDestroy()
{
	if (UWorld* World = GetWorld())
	{
		if (UActorPoolSubsystem* Pool = World->GetSubsystem<UActorPoolSubsystem>())
		{
			Pool->ReleaseActor(this);
			return;
		}
	}

	Destroy();
}

void ABaseProjectile::ApplyVisualColor()
{
	if (!VisualMesh || !VisualMaterial)
	{
		return;
	}

	// 풀 재사용 액터의 첫 BeginPlay(=Prewarm 스폰)에서 1회만 생성 — ProjectileColor는 클래스(BP)별 고정값이라
	// 매 발사마다 다시 만들 필요가 없다.
	if (!VisualMID)
	{
		VisualMID = UMaterialInstanceDynamic::Create(VisualMaterial, this);
		VisualMesh->SetMaterial(0, VisualMID);
	}

	if (VisualMID)
	{
		VisualMID->SetVectorParameterValue(TEXT("Color"), ProjectileColor);
	}
}
