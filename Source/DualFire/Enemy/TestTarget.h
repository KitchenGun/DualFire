// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Enemy/EnemyAttributeInterface.h"
#include "Weapon/DualFireWeaponTypes.h"
#include "TestTarget.generated.h"

class USphereComponent;
class USkeletalMeshComponent;

/**
 * 발사 시스템 검증용 정적 표적.
 * 인스턴스마다 Attribute(Ground/Air)를 설정해 대지/대공 무장 판정을 시각적으로 확인.
 * 단발 격파 — TakeDamage가 호출되면 즉시 Destroy.
 */
UCLASS(BlueprintType, Blueprintable)
class DUALFIRE_API ATestTarget : public AActor, public IEnemyAttributeInterface
{
	GENERATED_BODY()

public:
	ATestTarget();

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Components")
	TObjectPtr<USphereComponent> CollisionComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Components")
	TObjectPtr<USkeletalMeshComponent> Mesh;

	/** 이 표적의 속성. 인스턴스별로 에디터에서 설정 (Ground/Air) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon")
	EWeaponAttribute Attribute = EWeaponAttribute::Ground;

	/** 격파에 필요한 누적 피해량. 0 이하가 되면 Destroy */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon",
		meta=(ClampMin="1.0"))
	float HitPoints = 10.f;

	// ── IEnemyAttributeInterface ───────────────────────────────────────────────
	virtual EWeaponAttribute GetEnemyAttribute_Implementation() const override
	{
		return Attribute;
	}

	// ── AActor ────────────────────────────────────────────────────────────────
	virtual float TakeDamage(
		float                DamageAmount,
		const FDamageEvent&  DamageEvent,
		AController*         EventInstigator,
		AActor*              DamageCauser) override;
};
