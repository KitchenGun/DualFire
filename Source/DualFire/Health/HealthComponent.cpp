// Copyright DualFire. All Rights Reserved.

#include "Health/HealthComponent.h"
#include "DualFire.h"

UHealthComponent::UHealthComponent()
{
	// Tick은 Shield/무적 사용 시에만 켠다. RefreshTickEnabled() 참조.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;
	CurrentShield = bUseShield ? MaxShield : 0;
	CurrentLife = MaxLife;
	bIsDead = false;
	InvincibilityRemainingBySource.Reset();
	bIsInvincible = false;
	ResetShieldRecovery(false);

	if (AActor* Owner = GetOwner())
	{
		Owner->OnTakeAnyDamage.AddDynamic(this, &UHealthComponent::OnActorTakeAnyDamage);
	}

}

void UHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bUseInvincibility && bIsInvincible)
	{
		TickInvincibility(DeltaTime);
	}

	if (bUseShield && !bIsDead && CurrentShield < MaxShield)
	{
		TickShieldRecovery(DeltaTime);
	}
}

// ── 공개 API ──────────────────────────────────────────────────────────────────

void UHealthComponent::ApplyDamage(int32 Damage)
{
	if (bIsDead || (bUseInvincibility && bIsInvincible))
	{
		return;
	}

	const int32 FinalDamage = FMath::Max(Damage, 1);
	ResetShieldRecovery(true);
	OnDamageReceived.Broadcast();

	// Shield 흡수 레이어
	if (bUseShield && CurrentShield >= 1)
	{
		const int32 ShieldDamage = FMath::Min(CurrentShield, FinalDamage);
		CurrentShield -= ShieldDamage;
		RefreshTickEnabled();

		OnShieldChanged.Broadcast(CurrentShield, MaxShield);

		if (CurrentShield == 0)
		{
			// 보호막 파괴 무적 (초과 데미지는 차단)
			OnShieldBroken.Broadcast();
			StartInvincibility(EInvincibilitySource::ShieldBreak, BreakInvincibilityDuration);

			UE_LOG(LogDualFire, Log, TEXT("[Health] Shield 파괴 — %s"), *GetOwner()->GetName());
		}
		// Shield가 흡수한 피해는 HP에 전달하지 않는다.
		return;
	}

	// HP 직접 피격
	CurrentHealth -= FinalDamage;
	CurrentHealth = FMath::Max(CurrentHealth, 0);

	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);

	UE_LOG(LogDualFire, Log, TEXT("[Health] 피격 — %s HP: %d / %d"),
		*GetOwner()->GetName(), CurrentHealth, MaxHealth);

	if (CurrentHealth <= 0)
	{
		HandleDeath();
		return;
	}

	StartInvincibility(EInvincibilitySource::Hit, HitInvincibilityDuration);
}

void UHealthComponent::HandleDeath()
{
	AActor* Owner = GetOwner();
	const FString OwnerName = IsValid(Owner) ? Owner->GetName() : TEXT("Unknown");
	bIsDead = true;
	ClearAllInvincibility();
	ResetShieldRecovery(false);

	// 잔여 기체가 남아있으면 부활, 없으면 최종 사망
	if (bUseLife && CurrentLife > 0)
	{
		CurrentLife -= 1;
		OnLifeChanged.Broadcast(CurrentLife);

		UE_LOG(LogDualFire, Log, TEXT("[Health] %s 사망 → 리스폰 요청 (잔여 기체 %d 남음)"), *OwnerName, CurrentLife);
		OnRespawnRequested.Broadcast();
		return;
	}

	UE_LOG(LogDualFire, Warning, TEXT("[Health] %s 최종 사망 (잔여 기체 소진)"), *OwnerName);
	OnDeath.Broadcast();
}

void UHealthComponent::CompleteRespawn()
{
	if (!bIsDead)
	{
		return;
	}

	CurrentHealth = MaxHealth;
	CurrentShield = bUseShield ? MaxShield : 0;
	bIsDead = false;
	ResetShieldRecovery(false);

	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
	OnShieldChanged.Broadcast(CurrentShield, MaxShield);

	StartInvincibility(EInvincibilitySource::Respawn, RespawnInvincibilityDuration);
}

void UHealthComponent::RecoverHealth(int32 Amount)
{
	if (bIsDead || Amount <= 0 || CurrentHealth >= MaxHealth)
	{
		return;
	}

	CurrentHealth = FMath::Min(CurrentHealth + Amount, MaxHealth);
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

void UHealthComponent::FullRecoverShield()
{
	if (bIsDead || !bUseShield)
	{
		return;
	}

	CurrentShield = MaxShield;
	ResetShieldRecovery(false);
	OnShieldChanged.Broadcast(CurrentShield, MaxShield);
}

void UHealthComponent::StartInvincibility(EInvincibilitySource Source, float Duration)
{
	if (bIsDead || !bUseInvincibility || Duration <= 0.0f)
	{
		return;
	}

	float& Remaining = InvincibilityRemainingBySource.FindOrAdd(Source);
	Remaining = FMath::Max(Remaining, Duration);
	RefreshInvincibilityState();
	RefreshTickEnabled();
}

void UHealthComponent::ClearAllInvincibility()
{
	InvincibilityRemainingBySource.Reset();
	RefreshInvincibilityState();
	RefreshTickEnabled();
}

void UHealthComponent::InitFromData(
	int32 InMaxHealth,
	int32 InMaxShield,
	float InShieldRecoveryDelay,
	float InShieldRecoveryDuration,
	float InBreakInvincibilityDuration)
{
	MaxHealth = FMath::Max(InMaxHealth, 1);
	MaxShield = FMath::Max(InMaxShield, 0);
	ShieldRecoveryDelay = FMath::Max(InShieldRecoveryDelay, 0.0f);
	ShieldRecoveryDuration = FMath::Max(InShieldRecoveryDuration, 0.1f);
	BreakInvincibilityDuration = FMath::Max(InBreakInvincibilityDuration, 0.0f);

	CurrentHealth = MaxHealth;
	CurrentShield = bUseShield ? MaxShield : 0;
	CurrentLife = MaxLife;
	bIsDead = false;
	ClearAllInvincibility();
	ResetShieldRecovery(false);

	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
	OnShieldChanged.Broadcast(CurrentShield, MaxShield);
	OnLifeChanged.Broadcast(CurrentLife);
}

float UHealthComponent::GetHealthPercent() const
{
	return MaxHealth > 0 ? static_cast<float>(CurrentHealth) / MaxHealth : 0.0f;
}

float UHealthComponent::GetShieldPercent() const
{
	return (bUseShield && MaxShield > 0) ? static_cast<float>(CurrentShield) / MaxShield : 0.0f;
}

// ── 내부 구현 ─────────────────────────────────────────────────────────────────

void UHealthComponent::OnActorTakeAnyDamage(
	AActor*            DamagedActor,
	float              ActualDamage,
	const UDamageType* DamageType,
	AController*       InstigatedBy,
	AActor*            DamageCauser)
{
	ApplyDamage(FMath::RoundToInt(ActualDamage));
}

void UHealthComponent::TickInvincibility(float DeltaTime)
{
	for (auto It = InvincibilityRemainingBySource.CreateIterator(); It; ++It)
	{
		It.Value() -= DeltaTime;
		if (It.Value() <= 0.0f)
		{
			It.RemoveCurrent();
		}
	}

	RefreshInvincibilityState();
	RefreshTickEnabled();
}

void UHealthComponent::TickShieldRecovery(float DeltaTime)
{
	float RecoveryDelta = DeltaTime;
	if (ShieldRecoveryDelayRemaining > 0.0f)
	{
		if (RecoveryDelta < ShieldRecoveryDelayRemaining)
		{
			ShieldRecoveryDelayRemaining -= RecoveryDelta;
			return;
		}

		RecoveryDelta -= ShieldRecoveryDelayRemaining;
		ShieldRecoveryDelayRemaining = 0.0f;
	}

	ShieldRecoveryAccumulator += RecoveryDelta;

	const int32 ElapsedIntervals = FMath::FloorToInt(ShieldRecoveryAccumulator / ShieldRecoveryDuration);
	if (ElapsedIntervals > 0)
	{
		ShieldRecoveryAccumulator -= ElapsedIntervals * ShieldRecoveryDuration;
		CurrentShield = FMath::Min(CurrentShield + ElapsedIntervals, MaxShield);
		OnShieldChanged.Broadcast(CurrentShield, MaxShield);

		if (CurrentShield >= MaxShield)
		{
			ShieldRecoveryAccumulator = 0.0f;
			RefreshTickEnabled();
		}
	}
}

void UHealthComponent::ResetShieldRecovery(bool bStartDelay)
{
	ShieldRecoveryDelayRemaining = bStartDelay ? ShieldRecoveryDelay : 0.0f;
	ShieldRecoveryAccumulator = 0.0f;
	RefreshTickEnabled();
}

void UHealthComponent::RefreshInvincibilityState()
{
	const bool bWasInvincible = bIsInvincible;
	bIsInvincible = bUseInvincibility && InvincibilityRemainingBySource.Num() > 0;
	if (bWasInvincible != bIsInvincible)
	{
		OnInvincibilityChanged.Broadcast(bIsInvincible);
	}
}

void UHealthComponent::RefreshTickEnabled()
{
	// Shield 재생 대기 중이거나 무적 카운트다운 중일 때만 Tick 활성화
	const bool bNeedsTick =
		(bUseInvincibility && bIsInvincible) ||
		(bUseShield && !bIsDead && CurrentShield < MaxShield);

	SetComponentTickEnabled(bNeedsTick);
}
