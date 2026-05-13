// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/DualFireTypes.h"
#include "BaseProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;

/**
 * 플레이어 탄환 베이스 클래스.
 * 서브클래스에서 AttributeArray 배열을 설정해 대지/대공/범용 탄환을 구현.
 * 적과 Overlap 시 속성 비교 → 일치하면 데미지, 불일치하면 관통 진행.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class DUALFIRE_API ABaseProjectile : public AActor
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components",
		meta=(AllowPrivateAccess="true"))
	TObjectPtr<USphereComponent> CollisionComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components",
		meta=(AllowPrivateAccess="true"))
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

public:
	ABaseProjectile();

	virtual void BeginPlay() override;

	// ── 무장 속성 (서브클래스 생성자에서 설정) ───────────────────────────────

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	TArray<EDualFireAttribute> AttributeArray;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Weapon",
		meta=(ClampMin="0.0"))
	float Damage = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Weapon",
		meta=(ClampMin="0.1"))
	float LifeSpan = 5.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Weapon",
		meta=(ClampMin="1.0"))
	float ProjectileSpeed = 1200.f;

protected:
	// ── BP 확장 포인트 ────────────────────────────────────────────────────────

	/** 속성 일치 → 데미지 적용 후 호출 (이펙트/사운드용) */
	UFUNCTION(BlueprintImplementableEvent, Category="Weapon")
	void OnAttributeMatched(AActor* HitActor, UPrimitiveComponent* HitComp);

	/** 속성 불일치 → 관통 시 호출 */
	UFUNCTION(BlueprintImplementableEvent, Category="Weapon")
	void OnAttributeMismatch(AActor* HitActor, UPrimitiveComponent* HitComp);

private:
	UFUNCTION()
	void OnProjectileOverlapBegin(
		UPrimitiveComponent* OverlappedComp,
		AActor*              OtherActor,
		UPrimitiveComponent* OtherComp,
		int32                OtherBodyIndex,
		bool                 bFromSweep,
		const FHitResult&    SweepResult);

	/** 탄환 속성 배열 중 하나라도 적 속성과 일치하는지 확인. 적 시스템 미구현 시 true 반환. */
	bool CheckAttributeMatch(AActor* OtherActor) const;

	/** 관통 탄환이 같은 적에게 반복 피격되지 않도록 캐싱 */
	TSet<TWeakObjectPtr<AActor>> AlreadyHitActors;
};
