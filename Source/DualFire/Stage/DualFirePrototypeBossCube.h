// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DualFirePrototypeBossCube.generated.h"

class UMaterialInterface;
class USceneComponent;
class UStaticMeshComponent;

DECLARE_MULTICAST_DELEGATE(FOnPrototypeBossDestinationReached);

UCLASS()
class DUALFIRE_API ADualFirePrototypeBossCube : public AActor
{
	GENERATED_BODY()

public:
	ADualFirePrototypeBossCube();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	void StartDescent(const FVector& StartLocation, const FVector& Destination, float Duration);

	FOnPrototypeBossDestinationReached OnDestinationReached;

private:
	UPROPERTY(VisibleAnywhere, Category="Prototype Boss")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category="Prototype Boss")
	TObjectPtr<UStaticMeshComponent> CubeMesh;

	UPROPERTY(EditDefaultsOnly, Category="Prototype Boss")
	TSoftObjectPtr<UMaterialInterface> VisualMaterial;

	UPROPERTY(EditDefaultsOnly, Category="Prototype Boss", meta=(ClampMin="0.0"))
	float RotationSpeedDegrees = 20.0f;

	FVector DescentStart = FVector::ZeroVector;
	FVector DescentDestination = FVector::ZeroVector;
	float DescentDuration = 3.0f;
	float DescentElapsed = 0.0f;
	bool bIsDescending = false;
	bool bDestinationEventSent = false;
};
