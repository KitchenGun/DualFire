// Copyright DualFire. All Rights Reserved.

#include "Stage/DualFirePrototypeBossCube.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ADualFirePrototypeBossCube::ADualFirePrototypeBossCube()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	CubeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CubeMesh"));
	CubeMesh->SetupAttachment(SceneRoot);
	CubeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CubeMesh->SetGenerateOverlapEvents(false);
	CubeMesh->SetCastShadow(false);
	CubeMesh->SetRelativeScale3D(FVector(3.0f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshFinder.Succeeded())
	{
		CubeMesh->SetStaticMesh(CubeMeshFinder.Object);
	}

	VisualMaterial = TSoftObjectPtr<UMaterialInterface>(
		FSoftObjectPath(TEXT("/Game/Materials/M_PrototypeBossCube.M_PrototypeBossCube")));
}

void ADualFirePrototypeBossCube::BeginPlay()
{
	Super::BeginPlay();

	if (UMaterialInterface* Material = VisualMaterial.LoadSynchronous())
	{
		CubeMesh->SetMaterial(0, Material);
	}
}

void ADualFirePrototypeBossCube::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsDescending)
	{
		return;
	}

	DescentElapsed += DeltaTime;
	const float Alpha = FMath::Clamp(DescentElapsed / DescentDuration, 0.0f, 1.0f);
	const float EaseOutAlpha = 1.0f - FMath::Pow(1.0f - Alpha, 3.0f);
	SetActorLocation(FMath::Lerp(DescentStart, DescentDestination, EaseOutAlpha));
	AddActorLocalRotation(FRotator(0.0f, RotationSpeedDegrees * DeltaTime, 0.0f));

	if (Alpha >= 1.0f)
	{
		bIsDescending = false;
		SetActorTickEnabled(false);

		if (!bDestinationEventSent)
		{
			bDestinationEventSent = true;
			OnDestinationReached.Broadcast();
		}
	}
}

void ADualFirePrototypeBossCube::StartDescent(
	const FVector& StartLocation,
	const FVector& Destination,
	float Duration)
{
	DescentStart = StartLocation;
	DescentDestination = Destination;
	DescentDuration = FMath::Max(Duration, KINDA_SMALL_NUMBER);
	DescentElapsed = 0.0f;
	bDestinationEventSent = false;
	bIsDescending = true;

	SetActorLocation(DescentStart);
	SetActorTickEnabled(true);
}
