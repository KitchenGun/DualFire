// Copyright DualFire. All Rights Reserved.

#include "Enemy/EnemyBase.h"
#include "Camera/StageCameraActor.h"
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
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

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
	Mesh->SetReceivesDecals(false);
	Mesh->SetCastShadow(false);
	Mesh->bCastDynamicShadow = false;

	GroundShadow = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("GroundShadow"));
	GroundShadow->SetupAttachment(HitboxComp);
	GroundShadow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GroundShadow->SetReceivesDecals(false);
	GroundShadow->SetCastShadow(false);
	GroundShadow->bCastDynamicShadow = false;
	GroundShadow->SetVisibility(false);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GroundShadowMaterialFinder(
		TEXT("/Game/Material/Unit/M_AirUnitShadow.M_AirUnitShadow"));
	GroundShadowMaterial = GroundShadowMaterialFinder.Succeeded() ? GroundShadowMaterialFinder.Object : nullptr;

	HealthComp = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComp"));
	HealthComp->bUseShield       = false;
	HealthComp->bUseInvincibility = false;

	AIComp = CreateDefaultSubobject<UEnemyAIComponent>(TEXT("AIComp"));
}

void AEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	// 적은 잔여 기체 없음(bUseLife=false) → HP 0 시 OnDeath 즉시 발생 → 격파
	HealthComp->OnDeath.AddDynamic(this, &AEnemyBase::OnEnemyDeath);

	// 컴포넌트 BeginPlay가 Actor BeginPlay보다 먼저 실행되므로 InitFromData로 재초기화
	HealthComp->InitFromData(MaxHealth, 0, 0.0f, 1.0f, 0.0f);
}

void AEnemyBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterFromStageController();
	Super::EndPlay(EndPlayReason);
}

void AEnemyBase::OnAcquiredFromPool_Implementation()
{
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	if (HitboxComp)
	{
		HitboxComp->SetGenerateOverlapEvents(false);
		HitboxComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (AIComp)
	{
		AIComp->ResetRuntimeState();
	}
	if (GroundShadow)
	{
		GroundShadow->SetVisibility(false);
	}
	ApplyGroundShadowOpacity();
}

void AEnemyBase::OnReleasedToPool_Implementation()
{
	UnregisterFromStageController();
	RuntimeEnemyID = NAME_None;
	AirShadowOffsetPerHeight = FVector2D::ZeroVector;
	SetAirShadowOpacity(0.35f);
	VisualWorldOffset = FVector::ZeroVector;
	if (GroundShadow)
	{
		GroundShadow->SetVisibility(false);
		GroundShadow->SetLeaderPoseComponent(nullptr, false, false);
		GroundShadow->SetSkeletalMesh(nullptr);
	}
	if (Mesh)
	{
		Mesh->SetCastShadow(false);
		Mesh->bCastDynamicShadow = false;
	}
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

bool AEnemyBase::InitFromEnemyRow(const FEnemyRow& Row)
{
	if (!IsValid(Mesh) || !IsValid(HealthComp) || !IsValid(AIComp) ||
		Row.MaxHealth < 1 || Row.Mesh.IsNull())
	{
		return false;
	}

	USkeletalMesh* LoadedMesh = Row.Mesh.LoadSynchronous();
	if (!IsValid(LoadedMesh))
	{
		return false;
	}

	MaxHealth = Row.MaxHealth;
	RuntimeEnemyID = Row.EnemyID;
	EnemyAttribute = Row.Attribute;
	Mesh->SetSkeletalMesh(LoadedMesh);
	const bool bUsesNativeGroundShadow = EnemyAttribute.HasGround() && !EnemyAttribute.HasAir();
	Mesh->SetCastShadow(bUsesNativeGroundShadow);
	Mesh->bCastDynamicShadow = bUsesNativeGroundShadow;
	Mesh->SetRelativeLocation(FVector::ZeroVector);
	VisualWorldOffset = FVector::ZeroVector;
	if (const ADualFireGameModeBase* GameMode = Cast<ADualFireGameModeBase>(UGameplayStatics::GetGameMode(this)))
	{
		if (const AStageCameraActor* StageCamera = GameMode->GetStageCamera())
		{
			VisualWorldOffset = StageCamera->GetRenderHeightOffset(Row.RenderHeightRatio);
			Mesh->SetRelativeLocation(HitboxComp->GetComponentTransform().InverseTransformVectorNoScale(VisualWorldOffset));
		}
	}
	ApplyGroundShadow();
	HealthComp->InitFromData(MaxHealth, 0, 0.0f, 1.0f, 0.0f);
	AIComp->InitFromEnemyRow(Row);
	HitboxComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HitboxComp->SetGenerateOverlapEvents(true);
	return true;
}

void AEnemyBase::SetAirShadowOffsetPerHeight(const FVector2D& InOffsetPerHeight)
{
	AirShadowOffsetPerHeight = FMath::IsFinite(InOffsetPerHeight.X) && FMath::IsFinite(InOffsetPerHeight.Y)
		? InOffsetPerHeight
		: FVector2D::ZeroVector;
	ApplyGroundShadow();
}

void AEnemyBase::SetAirShadowOpacity(const float InAirShadowOpacity)
{
	AirShadowOpacity = FMath::IsFinite(InAirShadowOpacity)
		? FMath::Clamp(InAirShadowOpacity, 0.0f, 1.0f)
		: 0.35f;
	ApplyGroundShadowOpacity();
}

void AEnemyBase::ApplyGroundShadow()
{
	if (!IsValid(GroundShadow) || !IsValid(HitboxComp) || !IsValid(Mesh))
	{
		return;
	}

	const bool bShowGroundShadow = EnemyAttribute.HasAir();
	GroundShadow->SetVisibility(bShowGroundShadow);
	if (!bShowGroundShadow)
	{
		return;
	}

	const FRotator MeshRotation = Mesh->GetRelativeRotation();
	ensureMsgf(FMath::IsNearlyZero(MeshRotation.Pitch) && FMath::IsNearlyZero(MeshRotation.Roll),
		TEXT("[Enemy] GroundShadow supports yaw-only Mesh rotation."));
	GroundShadow->SetSkeletalMesh(Mesh->GetSkeletalMeshAsset());
	GroundShadow->SetLeaderPoseComponent(Mesh, true, false);
	GroundShadow->SetRelativeRotation(FRotator(0.0f, MeshRotation.Yaw, 0.0f));
	const FVector MeshScale = Mesh->GetRelativeScale3D();
	GroundShadow->SetRelativeScale3D(FVector(MeshScale.X, MeshScale.Y, 0.01f));

	const FVector WorldOffset = AStageCameraActor::CalculateGroundShadowOffset(
		VisualWorldOffset, AirShadowOffsetPerHeight);
	GroundShadow->SetRelativeLocation(HitboxComp->GetComponentTransform().InverseTransformVectorNoScale(WorldOffset));
	ApplyGroundShadowOpacity();
}

void AEnemyBase::ApplyGroundShadowOpacity()
{
	if (!IsValid(GroundShadow))
	{
		return;
	}

	if (!IsValid(GroundShadowMaterialInstance))
	{
		if (IsValid(GroundShadowMaterial))
		{
			GroundShadowMaterialInstance = GroundShadow->CreateDynamicMaterialInstance(0, GroundShadowMaterial);
		}
	}
	if (IsValid(GroundShadowMaterialInstance))
	{
		GroundShadowMaterialInstance->SetScalarParameterValue(TEXT("ShadowOpacity"), AirShadowOpacity);
		for (int32 MaterialIndex = 0; MaterialIndex < GroundShadow->GetNumMaterials(); ++MaterialIndex)
		{
			GroundShadow->SetMaterial(MaterialIndex, GroundShadowMaterialInstance);
		}
	}
}

void AEnemyBase::OnEnemyDeath()
{
	UE_LOG(LogDualFire, Log, TEXT("[Enemy] %s 격파"), *GetName());
	if (ADualFireGameModeBase* GM = Cast<ADualFireGameModeBase>(UGameplayStatics::GetGameMode(this)))
	{
		if (AStageController* StageController = GM->GetStageController())
		{
			StageController->NotifyEnemyDefeated(this);
		}
	}
	ReturnToPoolOrDestroy();
}

void AEnemyBase::UnregisterFromStageController()
{
	ADualFireGameModeBase* GM = Cast<ADualFireGameModeBase>(UGameplayStatics::GetGameMode(this));
	if (IsValid(GM) && IsValid(GM->GetStageController()))
	{
		GM->GetStageController()->UnregisterEnemy(this);
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
