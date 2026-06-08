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
	ShieldRegenAccumulator = 0.0f;

	if (bBindToActorDamage)
	{
		if (AActor* Owner = GetOwner())
		{
			Owner->OnTakeAnyDamage.AddDynamic(this, &UHealthComponent::OnActorTakeAnyDamage);
		}
	}

	RefreshTickEnabled();
}

void UHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bUseInvincibility && bIsInvincible)
	{
		TickInvincibility(DeltaTime);
	}

	if (bUseShield && CurrentShield < MaxShield)
	{
		TickShieldRegen(DeltaTime);
	}
}

// ── 공개 API ──────────────────────────────────────────────────────────────────

void UHealthComponent::ApplyDamage(int32 Damage)
{
	if (bUseInvincibility && bIsInvincible)
	{
		return;
	}

	const int32 FinalDamage = FMath::Max(Damage, 1);

	// Shield 흡수 레이어
	if (bUseShield && CurrentShield >= 1)
	{
		const int32 ShieldDamage = FMath::Min(CurrentShield, FinalDamage);
		CurrentShield -= ShieldDamage;

		OnShieldChanged.Broadcast(CurrentShield, MaxShield);

		if (CurrentShield == 0)
		{
			// 보호막 파괴 무적 (초과 데미지는 차단)
			if (bUseInvincibility && BreakInvincibilitySec > 0.0f)
			{
				StartInvincibility(BreakInvincibilitySec);
			}
			OnShieldBroken.Broadcast();

			UE_LOG(LogDualFire, Log, TEXT("[Health] Shield 파괴 — %s"), *GetOwner()->GetName());
		}
		// ★ Shield가 데미지를 흡수한 경우 HP에 전달하지 않음
		return;
	}

	// HP 직접 피격
	CurrentHealth -= FinalDamage;
	CurrentHealth = FMath::Max(CurrentHealth, 0);

	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);

	if (bUseInvincibility && HitInvincibilityDuration > 0.0f)
	{
		StartInvincibility(HitInvincibilityDuration);
	}

	UE_LOG(LogDualFire, Log, TEXT("[Health] 피격 — %s HP: %d / %d"),
		*GetOwner()->GetName(), CurrentHealth, MaxHealth);

	if (CurrentHealth <= 0)
	{
		// TODO: 사망 처리 (OnDeath 델리게이트 / 잔기 / 리스폰) — 다음 청크
		UE_LOG(LogDualFire, Warning, TEXT("[Health] HP 0 도달 — 사망 처리 미구현 (%s)"), *GetOwner()->GetName());
	}
}

void UHealthComponent::Heal(int32 Amount)
{
	if (Amount <= 0 || CurrentHealth >= MaxHealth)
	{
		return;
	}

	CurrentHealth = FMath::Min(CurrentHealth + Amount, MaxHealth);
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

void UHealthComponent::FullHealHealth()
{
	CurrentHealth = MaxHealth;
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

void UHealthComponent::FullHealShield()
{
	if (!bUseShield)
	{
		return;
	}

	CurrentShield = MaxShield;
	ShieldRegenAccumulator = 0.0f;
	OnShieldChanged.Broadcast(CurrentShield, MaxShield);

	RefreshTickEnabled();
}

void UHealthComponent::StartInvincibility(float Duration)
{
	if (!bUseInvincibility || Duration <= 0.0f)
	{
		return;
	}

	// 최장 우선 정책 — 짧은 Duration이 더 긴 것을 덮지 않음
	InvincibilityRemaining = FMath::Max(InvincibilityRemaining, Duration);

	if (!bIsInvincible)
	{
		bIsInvincible = true;
		OnInvincibilityChanged.Broadcast(true);
	}

	RefreshTickEnabled();
}

void UHealthComponent::InitFromData(int32 InMaxHealth, int32 InMaxShield, float InRegenInterval, float InBreakInvincSec)
{
	MaxHealth = FMath::Max(InMaxHealth, 1);
	MaxShield = FMath::Max(InMaxShield, 0);
	ShieldRegenInterval = FMath::Max(InRegenInterval, 0.1f);
	BreakInvincibilitySec = FMath::Max(InBreakInvincSec, 0.0f);

	CurrentHealth = MaxHealth;
	CurrentShield = bUseShield ? MaxShield : 0;
	ShieldRegenAccumulator = 0.0f;

	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
	OnShieldChanged.Broadcast(CurrentShield, MaxShield);

	RefreshTickEnabled();
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
	InvincibilityRemaining -= DeltaTime;

	if (InvincibilityRemaining <= 0.0f)
	{
		InvincibilityRemaining = 0.0f;
		bIsInvincible = false;
		OnInvincibilityChanged.Broadcast(false);

		RefreshTickEnabled();
	}
}

void UHealthComponent::TickShieldRegen(float DeltaTime)
{
	ShieldRegenAccumulator += DeltaTime;

	if (ShieldRegenAccumulator >= ShieldRegenInterval)
	{
		ShieldRegenAccumulator -= ShieldRegenInterval;
		CurrentShield = FMath::Min(CurrentShield + 1, MaxShield);
		OnShieldChanged.Broadcast(CurrentShield, MaxShield);

		if (CurrentShield >= MaxShield)
		{
			ShieldRegenAccumulator = 0.0f;
			RefreshTickEnabled();
		}
	}
}

void UHealthComponent::RefreshTickEnabled()
{
	// Shield 재생 대기 중이거나 무적 카운트다운 중일 때만 Tick 활성화
	const bool bNeedsTick =
		(bUseInvincibility && bIsInvincible) ||
		(bUseShield && CurrentShield < MaxShield);

	SetComponentTickEnabled(bNeedsTick);
}
