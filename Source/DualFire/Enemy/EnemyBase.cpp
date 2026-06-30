// Copyright DualFire. All Rights Reserved.

#include "Enemy/EnemyBase.h"
#include "Enemy/EnemyAIComponent.h"
#include "Health/HealthComponent.h"
#include "Core/DualFireCollisionChannels.h"
#include "DualFire.h"

#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"

AEnemyBase::AEnemyBase()
{
	PrimaryActorTick.bCanEverTick = false;

	HitboxComp = CreateDefaultSubobject<USphereComponent>(TEXT("HitboxComp"));
	HitboxComp->InitSphereRadius(30.0f);
	HitboxComp->SetCollisionProfileName(DualFireProfile::EnemyBody);
	SetRootComponent(HitboxComp);

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(HitboxComp);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	HealthComp = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComp"));
	HealthComp->bUseShield       = false;
	HealthComp->bUseInvincibility = false;
	HealthComp->bBindToActorDamage = true;

	AIComp = CreateDefaultSubobject<UEnemyAIComponent>(TEXT("AIComp"));
}

void AEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	// 적은 잔여 기체 없음(bUseLife=false) → HP 0 시 OnDeath 즉시 발생 → 격파
	HealthComp->OnDeath.AddDynamic(this, &AEnemyBase::OnEnemyDeath);

	// 컴포넌트 BeginPlay가 Actor BeginPlay보다 먼저 실행되므로 InitFromData로 재초기화
	HealthComp->InitFromData(MaxHealth, 0, 1.0f, 0.0f);
}

void AEnemyBase::OnEnemyDeath()
{
	UE_LOG(LogDualFire, Log, TEXT("[Enemy] %s 격파"), *GetName());
	Destroy();
}
