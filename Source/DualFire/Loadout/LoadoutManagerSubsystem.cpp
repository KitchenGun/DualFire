// Copyright DualFire. All Rights Reserved.

#include "Loadout/LoadoutManagerSubsystem.h"
#include "Player/DualFirePlayerPawn.h"
#include "Health/HealthComponent.h"
#include "Weapon/WeaponComponent.h"
#include "Core/LoadoutDataLibrary.h"
#include "DualFire.h"

void ULoadoutManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (!AircraftDataTable)
	{
		AircraftDataTable = LoadObject<UDataTable>(nullptr,
			TEXT("/Game/Data/Loadout/DT_LoadoutAircrafts.DT_LoadoutAircrafts"));
	}
	if (!ShieldDataTable)
	{
		ShieldDataTable = LoadObject<UDataTable>(nullptr,
			TEXT("/Game/Data/Loadout/DT_LoadoutShields.DT_LoadoutShields"));
	}
}

void ULoadoutManagerSubsystem::SetActiveLoadout(const FLoadout& Loadout)
{
	ActiveLoadout = Loadout;
	UE_LOG(LogDualFire, Log, TEXT("[LoadoutManagerSubsystem] 로드아웃 설정 — Aircraft:%s"), *Loadout.AircraftID.ToString());
}

bool ULoadoutManagerSubsystem::ApplyToPlayer(ADualFirePlayerPawn* Pawn)
{
	if (!IsValid(Pawn))
	{
		UE_LOG(LogDualFire, Warning, TEXT("[LoadoutManagerSubsystem] ApplyToPlayer: Pawn이 유효하지 않음"));
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
		UE_LOG(LogDualFire, Warning, TEXT("[LoadoutManagerSubsystem] HealthComponent 없음"));
		return false;
	}

	// AircraftRow에서 MaxHealth 조회 (없으면 현재 MaxHealth 유지)
	int32 MaxHealth = HealthComp->MaxHealth;
	FAircraftRow AircraftRow;
	if (ULoadoutDataLibrary::FindAircraftRow(AircraftDataTable, ActiveLoadout.AircraftID, AircraftRow))
	{
		MaxHealth = FMath::Max(AircraftRow.MaxHealth, 1);
	}

	// ShieldRow에서 Shield 파라미터 조회 (없으면 기본값)
	int32 MaxShield = 0;
	float ShieldRecoveryDuration = 5.0f;
	float BreakInvincibilityDuration = 0.5f;

	FShieldRow ShieldRow;
	if (ULoadoutDataLibrary::FindShieldRow(ShieldDataTable, ActiveLoadout.ShieldID, ShieldRow))
	{
		MaxShield                  = FMath::Max(ShieldRow.MaxShield, 0);
		ShieldRecoveryDuration     = FMath::Max(ShieldRow.ShieldRecoveryDuration, 0.1f);
		BreakInvincibilityDuration = FMath::Max(ShieldRow.BreakInvincibilityDuration, 0.0f);
	}

	HealthComp->bUseShield = (MaxShield > 0);
	HealthComp->InitFromData(MaxHealth, MaxShield, ShieldRecoveryDuration, BreakInvincibilityDuration);

	UE_LOG(LogDualFire, Log, TEXT("[LoadoutManagerSubsystem] 적용 완료 — HP:%d, Shield:%d"), MaxHealth, MaxShield);
	return true;
}

TSubclassOf<ADualFirePlayerPawn> ULoadoutManagerSubsystem::ResolveAircraftClass() const
{
	FAircraftRow AircraftRow;
	if (!ULoadoutDataLibrary::FindAircraftRow(AircraftDataTable, ActiveLoadout.AircraftID, AircraftRow))
	{
		return nullptr;
	}

	if (AircraftRow.AircraftClass.IsNull())
	{
		return nullptr;
	}

	return AircraftRow.AircraftClass.LoadSynchronous();
}
