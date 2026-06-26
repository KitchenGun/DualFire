// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Core/DualFireDataTypes.h"
#include "LoadoutManagerSubsystem.generated.h"

class ADualFirePlayerPawn;
class UDataTable;

/**
 * 현재 로드아웃을 보관하고 플레이어에게 주입하는 GameInstance 서브시스템.
 *
 * 사용 흐름:
 *   1. UI(로드아웃 선택 화면)에서 SetActiveLoadout() 호출
 *   2. GameMode.StartMission()에서 ApplyToPlayer() 호출
 *   3. WeaponComponent + HealthComponent 초기화 완료
 *
 * DataTable 없는 테스트 모드에서도 동작 (ShipRow/ShieldRow 없으면 기본값 사용).
 */
UCLASS()
class DUALFIRE_API ULoadoutManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// ── 로드아웃 관리 ─────────────────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, Category="Loadout")
	void SetActiveLoadout(const FLoadout& Loadout);

	UFUNCTION(BlueprintPure, Category="Loadout")
	const FLoadout& GetActiveLoadout() const { return ActiveLoadout; }

	// ── DataTable 참조 (생성자에서 자동 할당, 에디터에서 오버라이드 가능) ────────

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Loadout|Data")
	TObjectPtr<UDataTable> ShipDataTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Loadout|Data")
	TObjectPtr<UDataTable> ShieldDataTable;

	// ── 적용 ─────────────────────────────────────────────────────────────────

	/**
	 * ActiveLoadout을 Pawn에 적용.
	 *  - WeaponComponent: ApplyLoadout()
	 *  - HealthComponent: ShipRow(MaxHealth) + ShieldRow(Shield파라미터) → InitFromData()
	 */
	UFUNCTION(BlueprintCallable, Category="Loadout")
	bool ApplyToPlayer(ADualFirePlayerPawn* Pawn);

private:
	/** 현재 선택된 로드아웃. SetActiveLoadout로 설정, ApplyToPlayer로 주입 */
	UPROPERTY()
	FLoadout ActiveLoadout;
};
