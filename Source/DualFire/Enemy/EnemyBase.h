// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Enemy/EnemyAttributeInterface.h"
#include "Core/PoolableActor.h"
#include "Core/DualFireTypes.h"
#include "EnemyBase.generated.h"

class USphereComponent;
class USkeletalMeshComponent;
class UHealthComponent;
class UEnemyAIComponent;
struct FEnemyRow;

/**
 * 최소 적 액터.
 * HealthComponent로 HP를 관리하고, EnemyAIComponent가 이동/공격을 담당한다.
 * IEnemyAttributeInterface를 구현해 플레이어 탄 속성 매칭에 참여한다.
 *
 * 현재 구현: Linear 이동 + Single 공격.
 * 잔여 기체/GameMode 연결은 다음 청크.
 */
UCLASS(BlueprintType, Blueprintable)
class DUALFIRE_API AEnemyBase : public AActor, public IEnemyAttributeInterface, public IPoolableActor
{
	GENERATED_BODY()

	/** 피격 판정용 구체 콜리전. Profile=EnemyBody, 루트 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USphereComponent> HitboxComp;

	/** 비주얼 전담 스켈레탈 메시. 충돌 없음 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USkeletalMeshComponent> Mesh;

	/** HP 관리. 적은 Shield/무적 끄고 OnTakeAnyDamage 자동 구독 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UHealthComponent> HealthComp;

	/** 이동·공격 패턴 담당 AI 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UEnemyAIComponent> AIComp;

public:
	AEnemyBase();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnAcquiredFromPool_Implementation() override;
	virtual void OnReleasedToPool_Implementation() override;

	/** HP 초기값. HealthComponent의 MaxHealth로 주입 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy", meta=(ClampMin="1"))
	int32 MaxHealth = 3;

	/** 이 적의 Ground/Air 속성. 플레이어 탄 매칭에 사용 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy")
	FEnemyAttribute EnemyAttribute;

	/** false인 진단용 적은 공통 사망 경로를 사용하되 미션 통계에서는 제외한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Mission Result")
	bool bCountsTowardMissionMetrics = true;

	UFUNCTION(BlueprintCallable, Category="Enemy")
	bool InitFromEnemyRow(const FEnemyRow& Row);

	UEnemyAIComponent* GetAIComponent() const { return AIComp; }
	bool CountsTowardMissionMetrics() const { return bCountsTowardMissionMetrics; }

	// ── IEnemyAttributeInterface ──────────────────────────────────────────────

	virtual FEnemyAttribute GetEnemyAttributes_Implementation() const override
	{
		return EnemyAttribute;
	}

	virtual EDualFireAttribute GetEnemyAttribute_Implementation() const override
	{
		if (EnemyAttribute.bGround) return EDualFireAttribute::Ground;
		if (EnemyAttribute.bAir)    return EDualFireAttribute::Air;
		return EDualFireAttribute::None;
	}

private:
	/** HealthComp.OnDeath 콜백. 적은 잔여 기체 없이 즉시 격파 → Destroy */
	UFUNCTION()
	void OnEnemyDeath();

	void UnregisterFromStageController();
	void ReturnToPoolOrDestroy();
};
