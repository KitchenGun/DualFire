// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/DualFireTypes.h"
#include "Core/PoolableActor.h"
#include "BaseProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/**
 * 무장 데이터(FWeaponRow) 또는 적 AI가 발사 직후 탄환에 주입하는 런타임 설정 묶음.
 * ABaseProjectile::ApplyRuntimeConfig()로 한 번에 적용.
 */
USTRUCT(BlueprintType)
struct DUALFIRE_API FProjectileRuntimeConfig
{
	GENERATED_BODY()

	/** 탄환 속성 목록(대지/대공). 적 속성과 비교해 명중 판정 (bUseAttributeMatching=true일 때) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon")
	TArray<EDualFireAttribute> AttributeArray;

	/** 명중 시 가하는 데미지 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon", meta=(ClampMin="0.0"))
	float Damage = 10.f;

	/** 탄환 이동 속도 (cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon", meta=(ClampMin="1.0"))
	float ProjectileSpeed = 1200.f;

	/** 명중 후 처리: 파괴 / 관통 / 제한 관통 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon")
	EHitBehavior HitBehavior = EHitBehavior::Destroy;

	/** 관통 가능 횟수 (HitBehavior=LimitedPenetrate일 때만 사용). 0이면 무제한 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon", meta=(ClampMin="0"))
	int32 PenetrationLimit = 0;

	/**
	 * 발사체 콜리전 프로파일.
	 * 플레이어 탄 = "PlayerBulletProfile", 적 탄 = "EnemyBulletProfile"
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon")
	FName CollisionProfileName = NAME_None;

	/**
	 * 오버랩 대상으로 허용할 Object Channel.
	 * 플레이어 탄 = EnemyBody(Ch4), 적 탄 = PlayerHitbox(Ch1)
	 * ECC_MAX이면 ApplyRuntimeConfig에서 생성자 기본값을 유지한다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon")
	TEnumAsByte<ECollisionChannel> TargetChannel = ECC_MAX;

	/** false이면 속성(대지/대공) 비교를 건너뛰고 무조건 히트. 적 탄에 사용 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon")
	bool bUseAttributeMatching = true;

	/** 발사 방향 단위벡터. 기본은 X+ (플레이어 탄 진행 방향). 정규화 불필요 — 내부에서 처리 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon")
	FVector VelocityDirection = FVector(1.0f, 0.0f, 0.0f);
};
/**
 * 플레이어 탄환 베이스 클래스.
 * 서브클래스에서 AttributeArray 배열을 설정해 대지/대공/범용 탄환을 구현.
 * 적과 Overlap 시 속성 비교 → 일치하면 데미지, 불일치하면 관통 진행.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class DUALFIRE_API ABaseProjectile : public AActor, public IPoolableActor
{
	GENERATED_BODY()

	/** 충돌·오버랩 감지용 구체 콜리전. 루트 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components",
		meta=(AllowPrivateAccess="true"))
	TObjectPtr<USphereComponent> CollisionComp;

	/** 등속 직선 이동 담당. 중력·바운스 비활성 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components",
		meta=(AllowPrivateAccess="true"))
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	/** 육안 식별용 비주얼(구체). 충돌 없음 — 판정은 CollisionComp 전담 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components",
		meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> VisualMesh;

public:
	ABaseProjectile();

	virtual void BeginPlay() override;
	virtual void LifeSpanExpired() override;

	virtual void OnAcquiredFromPool_Implementation() override;
	virtual void OnReleasedToPool_Implementation() override;

	// ── 무장 속성 (서브클래스 생성자에서 설정, 런타임엔 ApplyRuntimeConfig로 덮어씀) ──

	/** 탄환 속성 목록(대지/대공). 적 속성과 비교해 명중 여부 결정 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Weapon")
	TArray<EDualFireAttribute> AttributeArray;

	/** 명중 시 가하는 데미지 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Weapon",
		meta=(ClampMin="0.0"))
	float Damage = 10.f;

	/** 자동 소멸까지의 수명(초). 화면 밖 탄환 누적 방지 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Weapon",
		meta=(ClampMin="0.1"))
	float LifeSpan = 5.f;

	/** 탄환 이동 속도 (cm/s) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Weapon",
		meta=(ClampMin="1.0"))
	float ProjectileSpeed = 1200.f;

	/** 명중 후 처리: 파괴 / 관통 / 제한 관통 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Weapon")
	EHitBehavior HitBehavior = EHitBehavior::Destroy;

	/** 관통 가능 횟수 (LimitedPenetrate일 때만 사용). 0이면 무제한 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Weapon", meta=(ClampMin="0"))
	int32 PenetrationLimit = 0;

	/** VisualMesh에 적용할 언릿 색상. 서브클래스(BP)에서 덮어써 탄환 종류를 구분(예: 적 탄=빨강) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Weapon|Visual")
	FLinearColor ProjectileColor = FLinearColor::White;

	/** ProjectileColor를 "Color" 벡터 파라미터로 받는 언릿 머티리얼. 기본값은 생성자에서 M_ProjectileUnlit 할당 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Weapon|Visual")
	TObjectPtr<UMaterialInterface> VisualMaterial;

	/** 발사 직후 무장/AI가 런타임 설정을 일괄 주입. 속도·콜리전·타깃 채널을 갱신 */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void ApplyRuntimeConfig(const FProjectileRuntimeConfig& RuntimeConfig);

	/** 활성 적탄이면 효과 없이 풀로 반환하고 true를 반환한다. */
	bool ClearForPlayerRespawn();

protected:
	// ── BP 확장 포인트 ────────────────────────────────────────────────────────

	/** 속성 일치 → 데미지 적용 후 호출 (이펙트/사운드용) */
	UFUNCTION(BlueprintImplementableEvent, Category="Weapon")
	void OnAttributeMatched(AActor* HitActor, UPrimitiveComponent* HitComp);

	/** 속성 불일치 → 관통 시 호출 */
	UFUNCTION(BlueprintImplementableEvent, Category="Weapon")
	void OnAttributeMismatch(AActor* HitActor, UPrimitiveComponent* HitComp);

private:
	/** CollisionComp의 오버랩 콜백. 타깃 채널 확인 → 속성 매칭 → 데미지/관통 처리 */
	UFUNCTION()
	void OnProjectileOverlapBegin(
		UPrimitiveComponent* OverlappedComp,
		AActor*              OtherActor,
		UPrimitiveComponent* OtherComp,
		int32                OtherBodyIndex,
		bool                 bFromSweep,
		const FHitResult&    SweepResult);

	/** 탄환 속성 배열 중 하나라도 적 속성과 일치하는지 확인. 속성 인터페이스가 없으면 false. */
	bool CheckAttributeMatch(AActor* OtherActor) const;

	/** 관통 탄환이 같은 적에게 반복 피격되지 않도록 캐싱 */
	TSet<TWeakObjectPtr<AActor>> AlreadyHitActors;

	/** 현재까지 관통한 횟수. PenetrationLimit과 비교해 소멸 시점 결정 */
	int32 PenetrationCount = 0;

	// ── 런타임 파라미터 (ApplyRuntimeConfig로 주입) ───────────────────────────

	/** 오버랩 필터 채널. 생성자 기본값 = EnemyBody (플레이어 탄 기본) */
	ECollisionChannel TargetChannel = ECC_GameTraceChannel4;

	/** false이면 속성 비교 없이 무조건 히트 */
	bool bUseAttributeMatching = true;

	/** VisualMaterial에서 생성한 다이내믹 인스턴스. ProjectileColor를 "Color" 파라미터로 적용 */
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> VisualMID;

	void ReturnToPoolOrDestroy();
	void ApplyVisualColor();
};
