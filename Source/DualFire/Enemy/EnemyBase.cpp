// Copyright DualFire. All Rights Reserved.

#include "Enemy/EnemyBase.h"
#include "Core/ActorPoolSubsystem.h"
#include "Core/DualFireDataTypes.h"
#include "Enemy/EnemyAIComponent.h"
#include "GameModes/DualFireGameModeBase.h"
#include "Health/HealthComponent.h"
#include "Stage/StageController.h"
#include "Core/DualFireCollisionChannels.h"
#include "DualFire.h"

#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Kismet/GameplayStatics.h"

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

void AEnemyBase::OnAcquiredFromPool_Implementation()
{
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	if (HitboxComp)
	{
		HitboxComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		HitboxComp->SetGenerateOverlapEvents(true);
	}
	if (AIComp)
	{
		AIComp->ResetRuntimeState();
	}
	RegisterWithStageController();
}

void AEnemyBase::OnReleasedToPool_Implementation()
{
	UnregisterFromStageController();
	if (AIComp)
	{
		AIComp->StopAttackTimer();
	}
	if (HitboxComp)
	{
		HitboxComp->SetGenerateOverlapEvents(false);
		HitboxComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AEnemyBase::InitFromEnemyRow(const FEnemyRow& Row)
{
	MaxHealth = FMath::Max(1, Row.MaxHealth);
	EnemyAttribute = Row.Attribute;

	if (USkeletalMesh* LoadedMesh = Row.Mesh.LoadSynchronous())
	{
		Mesh->SetSkeletalMesh(LoadedMesh);
	}

	if (HealthComp)
	{
		HealthComp->InitFromData(MaxHealth, 0, 1.0f, 0.0f);
	}

	if (AIComp)
	{
		AIComp->InitFromEnemyRow(Row);
	}
}

void AEnemyBase::OnEnemyDeath()
{
	UE_LOG(LogDualFire, Log, TEXT("[Enemy] %s 격파"), *GetName());
	if (ADualFireGameModeBase* GM = Cast<ADualFireGameModeBase>(UGameplayStatics::GetGameMode(this)))
	{
		if (AStageController* StageController = GM->GetStageController())
		{
			StageController->NotifyEnemyDefeated(EnemyAttribute);
		}
	}
	ReturnToPoolOrDestroy();
}

void AEnemyBase::RegisterWithStageController()
{
	ADualFireGameModeBase* GM = Cast<ADualFireGameModeBase>(UGameplayStatics::GetGameMode(this));
	if (IsValid(GM) && IsValid(GM->GetStageController()) && IsValid(AIComp))
	{
		GM->GetStageController()->RegisterEnemyAI(AIComp);
	}
}

void AEnemyBase::UnregisterFromStageController()
{
	ADualFireGameModeBase* GM = Cast<ADualFireGameModeBase>(UGameplayStatics::GetGameMode(this));
	if (IsValid(GM) && IsValid(GM->GetStageController()) && IsValid(AIComp))
	{
		GM->GetStageController()->UnregisterEnemyAI(AIComp);
	}
}

void AEnemyBase::ReturnToPoolOrDestroy()
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
