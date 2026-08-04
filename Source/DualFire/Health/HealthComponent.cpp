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
	ShieldRecoveryAccumulator = 0.0f;

	if (AActor* Owner = GetOwner())
	{
		// 리스폰 복귀 지점 = 시작 위치 (외부에서 SetRespawnLocation으로 변경 가능)
		RespawnLocation = Owner->GetActorLocation();

		if (bBindToActorDamage)
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
		TickShieldRecovery(DeltaTime);
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
			if (bUseInvincibility && BreakInvincibilityDuration > 0.0f)
			{
				StartInvincibility(BreakInvincibilityDuration);
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
		HandleDeath();
	}
}

void UHealthComponent::HandleDeath()
{
	AActor* Owner = GetOwner();
	const FString OwnerName = IsValid(Owner) ? Owner->GetName() : TEXT("Unknown");

	// 잔여 기체가 남아있으면 부활, 없으면 최종 사망
	if (bUseLife && CurrentLife > 0)
	{
		CurrentLife -= 1;
		OnLifeChanged.Broadcast(CurrentLife);

		UE_LOG(LogDualFire, Log, TEXT("[Health] %s 사망 → 리스폰 (잔여 기체 %d 남음)"), *OwnerName, CurrentLife);
		Respawn();
		return;
	}

	UE_LOG(LogDualFire, Warning, TEXT("[Health] %s 최종 사망 (잔여 기체 소진)"), *OwnerName);
	OnDeath.Broadcast();
}

void UHealthComponent::Respawn()
{
	// HP / Shield 풀충전 (사양 §5.3.7)
	CurrentHealth = MaxHealth;
	CurrentShield = bUseShield ? MaxShield : 0;
	ShieldRecoveryAccumulator = 0.0f;

	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
	OnShieldChanged.Broadcast(CurrentShield, MaxShield);

	// 리스폰 무적
	StartInvincibility(RespawnInvincibilityDuration);

	// 시작 위치로 복귀
	if (AActor* Owner = GetOwner())
	{
		Owner->SetActorLocation(RespawnLocation);
	}

	OnRespawn.Broadcast();
	RefreshTickEnabled();
}

void UHealthComponent::RecoverHealth(int32 Amount)
{
	if (Amount <= 0 || CurrentHealth >= MaxHealth)
	{
		return;
	}

	CurrentHealth = FMath::Min(CurrentHealth + Amount, MaxHealth);
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

void UHealthComponent::FullRecoverHealth()
{
	CurrentHealth = MaxHealth;
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

void UHealthComponent::FullRecoverShield()
{
	if (!bUseShield)
	{
		return;
	}

	CurrentShield = MaxShield;
	ShieldRecoveryAccumulator = 0.0f;
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

void UHealthComponent::InitFromData(int32 InMaxHealth, int32 InMaxShield, float InShieldRecoveryDuration, float InBreakInvincibilityDuration)
{
	MaxHealth = FMath::Max(InMaxHealth, 1);
	MaxShield = FMath::Max(InMaxShield, 0);
	ShieldRecoveryDuration = FMath::Max(InShieldRecoveryDuration, 0.1f);
	BreakInvincibilityDuration = FMath::Max(InBreakInvincibilityDuration, 0.0f);

	CurrentHealth = MaxHealth;
	CurrentShield = bUseShield ? MaxShield : 0;
	CurrentLife = MaxLife;
	ShieldRecoveryAccumulator = 0.0f;

	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
	OnShieldChanged.Broadcast(CurrentShield, MaxShield);
	OnLifeChanged.Broadcast(CurrentLife);

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

void UHealthComponent::TickShieldRecovery(float DeltaTime)
{
	ShieldRecoveryAccumulator += DeltaTime;

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

void UHealthComponent::RefreshTickEnabled()
{
	// Shield 재생 대기 중이거나 무적 카운트다운 중일 때만 Tick 활성화
	const bool bNeedsTick =
		(bUseInvincibility && bIsInvincible) ||
		(bUseShield && CurrentShield < MaxShield);

	SetComponentTickEnabled(bNeedsTick);
}
