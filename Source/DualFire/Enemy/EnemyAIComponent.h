// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/DualFireDataTypes.h"
#include "Core/DualFireTypes.h"
#include "Weapon/Projectile/BaseProjectile.h"
#include "EnemyAIComponent.generated.h"

/**
 * 적 이동 + 공격을 담당하는 컴포넌트.
 * EnemyBase에 부착해서 사용. 다른 AActor에 붙여도 동작한다.
 *
 * 구현된 패턴:
 *   이동: Linear (X- 방향 등속 이동)
 *   공격: None / Single (일정 간격 단발 발사)
 *
 * 미구현: EnterStop, Hover, Spread3, Rotate3
 */
UCLASS(ClassGroup=Enemy, meta=(BlueprintSpawnableComponent))
class DUALFIRE_API UEnemyAIComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEnemyAIComponent();

	virtual void BeginPlay() override;

	void UpdateAI(float DeltaTime, const FBox2D& PlayableBounds, const FVector& PlayerLocation);
	void InitFromEnemyRow(const FEnemyRow& Row);
	void ResetRuntimeState();
	void StopAttackTimer();

	// ── 이동 ─────────────────────────────────────────────────────────────────

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="AI|Movement")
	EEnemyMovementPattern MovementPattern = EEnemyMovementPattern::Linear;

	/** Linear 이동 속도 (cm/s). X- 방향 (스크롤 진행 방향) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="AI|Movement", meta=(ClampMin="0.0"))
	float MoveSpeed = 300.0f;

	/** EnterStop: 스폰 지점에서 -X 방향으로 이 거리만큼 진입 후 정지 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="AI|Movement", meta=(ClampMin="0.0"))
	float EnterDistance = 400.0f;

	/** 화면 아래쪽 이탈 판정 여유 거리 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="AI|Movement", meta=(ClampMin="0.0"))
	float DespawnMargin = 300.0f;

	// ── 공격 ─────────────────────────────────────────────────────────────────

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="AI|Attack")
	EEnemyAttackPattern AttackPattern = EEnemyAttackPattern::Single;

	/** 공격 간격(초). Single 패턴에서 사용 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="AI|Attack", meta=(ClampMin="0.1"))
	float AttackInterval = 2.0f;

	/** 최초 공격까지 대기 시간. 0이면 즉시 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="AI|Attack", meta=(ClampMin="0.0"))
	float FirstAttackDelay = 0.5f;

	/** 발사할 투사체 클래스. BP에서 설정 필요 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AI|Attack")
	TSubclassOf<ABaseProjectile> ProjectileClass;

	/** 발사 위치 오프셋 (Owner 로컬) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="AI|Attack")
	FVector MuzzleOffset = FVector(0.0f, 0.0f, 0.0f);

	/** 적 탄 데미지 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="AI|Attack", meta=(ClampMin="1.0"))
	float ProjectileDamage = 1.0f;

	/** 적 탄 속도 (cm/s) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="AI|Attack", meta=(ClampMin="100.0"))
	float ProjectileSpeed = 800.0f;

private:
	/** Single 공격 주기 타이머 핸들 */
	FTimerHandle AttackTimerHandle;

	FVector SpawnLocation = FVector::ZeroVector;
	FVector CachedPlayerLocation = FVector::ZeroVector;
	bool bHasCachedPlayerLocation = false;
	bool bEnterStopReached = false;

	void StartAttackTimer();
	void TickLinearMovement(float DeltaTime);
	void TickEnterStopMovement(float DeltaTime);
	void ReleaseOwnerToPool();
	FVector GetAimDirection(const FVector& SpawnLocation) const;

	/** 단발 발사 — 적 탄 스폰 후 ApplyRuntimeConfig로 적 탄 설정 주입 */
	void FireSingle();
};
