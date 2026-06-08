// Copyright DualFire. All Rights Reserved.

#include "Loadout/LoadoutManager.h"
#include "Player/DualFirePlayerPawn.h"
#include "Health/HealthComponent.h"
#include "Weapon/WeaponComponent.h"
#include "Core/LoadoutDataLibrary.h"
#include "DualFire.h"

void ULoadoutManager::SetActiveLoadout(const FLoadout& Loadout)
{
	ActiveLoadout = Loadout;
	UE_LOG(LogDualFire, Log, TEXT("[LoadoutManager] 로드아웃 설정 — Ship:%s"), *Loadout.ShipID.ToString());
}

bool ULoadoutManager::ApplyToPlayer(ADualFirePlayerPawn* Pawn)
{
	if (!IsValid(Pawn))
	{
		UE_LOG(LogDualFire, Warning, TEXT("[LoadoutManager] ApplyToPlayer: Pawn이 유효하지 않음"));
		return false;
	}

	// ── 무장 적용 ─────────────────────────────────────────────────────────────
	UWeaponComponent* WeaponComp = Pawn->GetWeaponComp();
	if (IsValid(WeaponComp))
	{
		WeaponComp->ApplyLoadout(ActiveLoadout);
	}

	// ── HP / Shield 적용 ──────────────────────────────────────────────────────
	UHealthComponent* HealthComp = Pawn->GetHealthComp();
	if (!IsValid(HealthComp))
	{
		UE_LOG(LogDualFire, Warning, TEXT("[LoadoutManager] HealthComponent 없음"));
		return false;
	}

	// ShipRow에서 MaxHealth 조회 (없으면 현재 MaxHealth 유지)
	int32 MaxHealth = HealthComp->MaxHealth;
	FShipRow ShipRow;
	if (ULoadoutDataLibrary::FindShipRow(ShipDataTable, ActiveLoadout.ShipID, ShipRow))
	{
		MaxHealth = FMath::Max(ShipRow.MaxHealth, 1);
	}

	// ShieldRow에서 Shield 파라미터 조회 (없으면 기본값)
	int32 MaxShield = 0;
	float RegenInterval = 5.0f;
	float BreakInvincSec = 0.5f;

	FShieldRow ShieldRow;
	if (ULoadoutDataLibrary::FindShieldRow(ShieldDataTable, ActiveLoadout.ShieldID, ShieldRow))
	{
		MaxShield     = FMath::Max(ShieldRow.MaxShield, 0);
		RegenInterval = FMath::Max(ShieldRow.RegenInterval, 0.1f);
		BreakInvincSec = FMath::Max(ShieldRow.BreakInvincibilitySec, 0.0f);
	}

	HealthComp->bUseShield = (MaxShield > 0);
	HealthComp->InitFromData(MaxHealth, MaxShield, RegenInterval, BreakInvincSec);

	UE_LOG(LogDualFire, Log, TEXT("[LoadoutManager] 적용 완료 — HP:%d, Shield:%d"), MaxHealth, MaxShield);
	return true;
}
