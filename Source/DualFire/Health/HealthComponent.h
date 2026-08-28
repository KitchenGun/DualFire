// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

UENUM(BlueprintType)
enum class EInvincibilitySource : uint8
{
	Hit,
	ShieldBreak,
	Respawn,
};

// ── 델리게이트 ────────────────────────────────────────────────────────────────

/** HP 변화 시 브로드캐스트. Current / Max */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, int32, CurrentHealth, int32, MaxHealth);

/** Shield 변화 시 브로드캐스트. Current / Max */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnShieldChanged, int32, CurrentShield, int32, MaxShield);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDamageReceived);

/** Shield가 0이 된 순간 1회 브로드캐스트 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShieldBroken);

/** 무적 상태 전환 시 브로드캐스트. bActive=true → 무적 시작, false → 해제 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInvincibilityChanged, bool, bActive);

/** 잔여 기체 수 변화 시 브로드캐스트. HUD 잔여 기체 표시 갱신용 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLifeChanged, int32, CurrentLife);

/** 잔여 기체를 소비해 리스폰 연출을 시작해야 할 때 브로드캐스트 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRespawnRequested);

/**
 * 최종 사망(잔여 기체 소진 후 HP 0) 시 1회 브로드캐스트.
 * 플레이어 → GameMode.OnMissionFail() 연결, 적 → Destroy() 연결.
 * 이 컴포넌트는 구독자(GameMode 등)를 직접 알지 못한다 (의존성 역전).
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeath);

/**
 * HP / Shield / 무적(피격 후 / 보호막 파괴 후) / Shield 자동 재생을 관리하는 컴포넌트.
 *
 * 플레이어·적 모두 재사용 가능하도록 기능을 플래그로 opt-in:
 *   bUseShield          — 보호막 흡수 레이어 사용 여부
 *   bUseInvincibility   — 피격 후 무적 / 보호막 파괴 무적 사용 여부
 */
UCLASS(ClassGroup=Health, meta=(BlueprintSpawnableComponent))
class DUALFIRE_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHealthComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ── 기능 플래그 ───────────────────────────────────────────────────────────

	/** true: Shield가 HP 앞단에서 데미지를 흡수 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Health|Flags")
	bool bUseShield = true;

	/** true: 피격 / Shield 파괴 후 무적 시간 부여 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Health|Flags")
	bool bUseInvincibility = true;

	// ── HP ───────────────────────────────────────────────────────────────────

	/** 최대 체력. BeginPlay/InitFromData에서 CurrentHealth의 초기값이 된다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Health", meta=(ClampMin="1"))
	int32 MaxHealth = 5;

	/** 현재 체력. 0에 도달하면 잔여 기체 또는 최종 사망을 처리한다. 런타임 전용 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Health")
	int32 CurrentHealth = 5;

	// ── Shield ────────────────────────────────────────────────────────────────

	/** 최대 보호막 칸 수. 0이면 보호막 없음 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Health|Shield", meta=(ClampMin="0"))
	int32 MaxShield = 2;

	/** 현재 보호막 칸 수. HP 앞단에서 데미지를 흡수. 런타임 전용 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Health|Shield")
	int32 CurrentShield = 0;

	/** Shield 1칸 재생에 걸리는 초 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Health|Shield", meta=(ClampMin="0.1"))
	float ShieldRecoveryDuration = 5.0f;

	/** 유효 피해 후 Shield 재생을 시작하기까지의 대기 시간 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Health|Shield", meta=(ClampMin="0.0"))
	float ShieldRecoveryDelay = 2.0f;

	// ── 무적 ─────────────────────────────────────────────────────────────────

	/** Shield 파괴 직후 부여되는 무적 시간(초). 0이면 미적용 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Health|Invincibility", meta=(ClampMin="0.0"))
	float BreakInvincibilityDuration = 0.5f;

	/** HP 직접 피격 시 부여되는 무적 시간(초). 0이면 미적용 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Health|Invincibility", meta=(ClampMin="0.0"))
	float HitInvincibilityDuration = 2.5f;

	/** 리스폰(부활) 직후 부여되는 무적 시간(초). 사양 §5.3.6 기본 3.0초 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Health|Invincibility", meta=(ClampMin="0.0"))
	float RespawnInvincibilityDuration = 3.0f;

	/** 하나 이상의 무적 출처가 활성화되어 있는지 여부 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Health|Invincibility")
	bool bIsInvincible = false;

	// ── 잔여 기체 / 리스폰 ────────────────────────────────────────────────

	/** true: 사망 시 잔여 기체가 남아있으면 리스폰. 플레이어 true, 적 false */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Health|Life")
	bool bUseLife = false;

	/** 최대 잔여 기체 수. 사양 §5.3.2 기본 1 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Health|Life", meta=(ClampMin="0"))
	int32 MaxLife = 1;

	/** 현재 남은 잔여 기체 수. 0에서 사망하면 최종 사망(OnDeath). 런타임 전용 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Health|Life")
	int32 CurrentLife = 1;

	/** 사망 후 최종 사망 또는 리스폰 진입이 끝나기 전인 상태 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Health|Life")
	bool bIsDead = false;

	// ── 델리게이트 (HUD·이펙트 구독용) ──────────────────────────────────────

	/** HP가 변할 때마다 발생. HUD 체력바 갱신용 */
	UPROPERTY(BlueprintAssignable, Category="Health|Events")
	FOnHealthChanged OnHealthChanged;

	/** Shield가 변할 때마다 발생. HUD 보호막 표시 갱신용 */
	UPROPERTY(BlueprintAssignable, Category="Health|Events")
	FOnShieldChanged OnShieldChanged;

	/** 사망·무적으로 무시되지 않은 피해 요청마다 한 번 발생 */
	UPROPERTY(BlueprintAssignable, Category="Health|Events")
	FOnDamageReceived OnDamageReceived;

	/** Shield가 0이 되는 순간 발생. 파괴 이펙트/사운드용 */
	UPROPERTY(BlueprintAssignable, Category="Health|Events")
	FOnShieldBroken OnShieldBroken;

	/** 무적 시작/해제 시 발생. 무적 중 메시 점멸 연출용 */
	UPROPERTY(BlueprintAssignable, Category="Health|Events")
	FOnInvincibilityChanged OnInvincibilityChanged;

	/** 잔여 기체 수 변동 시 발생. HUD 잔여 기체 표시 갱신용 */
	UPROPERTY(BlueprintAssignable, Category="Health|Events")
	FOnLifeChanged OnLifeChanged;

	/** 잔여 기체 차감 후 Pawn이 사망 대기/진입 연출을 시작하는 지점 */
	UPROPERTY(BlueprintAssignable, Category="Health|Events")
	FOnRespawnRequested OnRespawnRequested;

	/** 최종 사망(잔여 기체 소진) 시 발생. 플레이어→미션 실패, 적→파괴 연결 지점 */
	UPROPERTY(BlueprintAssignable, Category="Health|Events")
	FOnDeath OnDeath;

	// ── 공개 API ──────────────────────────────────────────────────────────────

	/**
	 * 데미지 적용 진입점.
	 * Shield → HP 순으로 흡수. 무적 중이면 무시. 최소 1 데미지 보장.
	 * HP가 0에 도달하면 잔여 기체가 있을 때 리스폰하고, 없으면 OnDeath를 발생시킨다.
	 */
	UFUNCTION(BlueprintCallable, Category="Health")
	void ApplyDamage(int32 Damage);

	/** HP 회복. MaxHealth 초과 불가. Shield는 건드리지 않음 */
	UFUNCTION(BlueprintCallable, Category="Health")
	void RecoverHealth(int32 Amount);

	/** Shield를 MaxShield로 즉시 회복 */
	UFUNCTION(BlueprintCallable, Category="Health")
	void FullRecoverShield();

	/**
	 * 출처별 무적 시작. 같은 출처는 현재 잔여 시간과 새 시간 중 긴 값을 유지한다.
	 */
	UFUNCTION(BlueprintCallable, Category="Health")
	void StartInvincibility(EInvincibilitySource Source, float Duration);

	UFUNCTION(BlueprintCallable, Category="Health")
	void ClearAllInvincibility();

	/**
	 * 외부 데이터(DataTable 등)에서 스탯을 주입하는 초기화 함수.
	 * LoadoutManager가 AircraftRow/ShieldRow 기반으로 호출. HP/Shield/잔여 기체 모두 리셋.
	 */
	UFUNCTION(BlueprintCallable, Category="Health")
	void InitFromData(
		int32 InMaxHealth,
		int32 InMaxShield,
		float InShieldRecoveryDelay,
		float InShieldRecoveryDuration,
		float InBreakInvincibilityDuration);

	/** 진입 연출 종료 시 생존 자원 복구와 리스폰 무적을 적용한다. */
	void CompleteRespawn();

	// ── BlueprintPure 게터 (HUD용) ────────────────────────────────────────────

	UFUNCTION(BlueprintPure, Category="Health")
	float GetHealthPercent() const;

	UFUNCTION(BlueprintPure, Category="Health")
	float GetShieldPercent() const;

	UFUNCTION(BlueprintPure, Category="Health|Life")
	int32 GetCurrentLife() const { return CurrentLife; }

private:
	/** Shield 재생 누적 타이머(초). ShieldRecoveryDuration 도달 시 1칸 회복 후 차감 */
	float ShieldRecoveryAccumulator = 0.0f;

	/** 유효 피해 후 Shield 재생 시작까지 남은 대기 시간 */
	float ShieldRecoveryDelayRemaining = 0.0f;

	/** 출처별 무적 잔여 시간 */
	TMap<EInvincibilitySource, float> InvincibilityRemainingBySource;

	/** HP 0 도달 시 호출. 잔여 기체 있으면 Respawn, 없으면 OnDeath 브로드캐스트 */
	void HandleDeath();

	/** Owner의 OnTakeAnyDamage 콜백 */
	UFUNCTION()
	void OnActorTakeAnyDamage(
		AActor* DamagedActor,
		float   ActualDamage,
		const UDamageType* DamageType,
		AController*       InstigatedBy,
		AActor*            DamageCauser);

	/** 무적 잔여 시간 카운트다운. Tick에서 무적 상태일 때만 호출 */
	void TickInvincibility(float DeltaTime);

	/** Shield 자동 재생 누적 처리. Tick에서 Shield 미충전 시에만 호출 */
	void TickShieldRecovery(float DeltaTime);

	/** 유효 피해/사망/초기화 시 재생 대기와 누적값을 함께 초기화 */
	void ResetShieldRecovery(bool bStartDelay);

	/** 출처 타이머 변경 후 전체 무적 상태와 이벤트를 갱신 */
	void RefreshInvincibilityState();

	/** Shield 재생/무적 카운트다운 필요 여부에 따라 컴포넌트 Tick을 켜고 끔 */
	void RefreshTickEnabled();
};
