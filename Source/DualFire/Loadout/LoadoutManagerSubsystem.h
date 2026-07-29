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
 * DataTable 없는 테스트 모드에서도 동작 (AircraftRow/ShieldRow 없으면 기본값 사용).
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

	/** 모든 슬롯과 DataTable 행을 검증한 뒤에만 현재 로드아웃을 변경한다. */
	UFUNCTION(BlueprintCallable, Category="Loadout")
	bool TrySetActiveLoadout(const FLoadout& Loadout, FText& OutError, FName& OutInvalidField);

	/** UI와 출격 흐름에서 공통으로 사용하는 전체 로드아웃 검증이다. */
	UFUNCTION(BlueprintCallable, Category="Loadout")
	bool ValidateLoadout(const FLoadout& Loadout, FText& OutError, FName& OutInvalidField) const;

	UFUNCTION(BlueprintPure, Category="Loadout")
	const FLoadout& GetActiveLoadout() const { return ActiveLoadout; }

	// ── DataTable 참조 (생성자에서 자동 할당, 에디터에서 오버라이드 가능) ────────

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Loadout|Data")
	TObjectPtr<UDataTable> AircraftDataTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Loadout|Data")
	TObjectPtr<UDataTable> WeaponDataTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Loadout|Data")
	TObjectPtr<UDataTable> SuperWeaponDataTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Loadout|Data")
	TObjectPtr<UDataTable> ShieldDataTable;

	// ── 적용 ─────────────────────────────────────────────────────────────────

	/**
	 * ActiveLoadout을 Pawn에 적용.
	 *  - WeaponComponent: ApplyLoadout()
	 *  - HealthComponent: AircraftRow(MaxHealth) + ShieldRow(Shield파라미터) → InitFromData()
	 */
	UFUNCTION(BlueprintCallable, Category="Loadout")
	bool ApplyToPlayer(ADualFirePlayerPawn* Pawn);

	/**
	 * ActiveLoadout.AircraftID로 AircraftRow를 조회해 스폰할 Pawn 클래스를 반환.
	 * GameMode::GetDefaultPawnClassForController_Implementation에서 호출.
	 * AircraftRow를 못 찾으면 nullptr (호출측이 기본 DefaultPawnClass로 폴백).
	 */
	UFUNCTION(BlueprintCallable, Category="Loadout")
	TSubclassOf<ADualFirePlayerPawn> ResolveAircraftClass() const;

private:
	/** 현재 선택된 로드아웃. SetActiveLoadout로 설정, ApplyToPlayer로 주입 */
	UPROPERTY()
	FLoadout ActiveLoadout;
};
