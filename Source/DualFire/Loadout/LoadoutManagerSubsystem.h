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
 *   1. UI(로드아웃 선택 화면)에서 TrySetActiveLoadout() 호출
 *   2. GameMode.StartMission()에서 TryApplyActiveLoadout() 호출
 *   3. WeaponComponent + HealthComponent 초기화 완료
 */
UCLASS()
class DUALFIRE_API ULoadoutManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// ── 로드아웃 관리 ─────────────────────────────────────────────────────────

	/** 모든 슬롯과 DataTable 행을 검증한 뒤에만 현재 로드아웃을 변경한다. */
	UFUNCTION(BlueprintCallable, Category="Loadout")
	bool TrySetActiveLoadout(const FLoadout& Loadout, FText& OutError, FName& OutInvalidField);

	/** UI와 출격 흐름에서 공통으로 사용하는 전체 로드아웃 검증이다. */
	UFUNCTION(BlueprintCallable, Category="Loadout")
	bool ValidateLoadout(const FLoadout& Loadout, FText& OutError, FName& OutInvalidField) const;

	UFUNCTION(BlueprintPure, Category="Loadout")
	const FLoadout& GetActiveLoadout() const { return ActiveLoadout; }

	UFUNCTION(BlueprintPure, Category="Loadout")
	bool HasActiveLoadout() const { return bHasActiveLoadout; }

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
	 * ActiveLoadout을 검증하고 Pawn에 원자적으로 적용.
	 *  - WeaponComponent: 해석된 세 무장 행 적용
	 *  - HealthComponent: AircraftRow(MaxHealth) + ShieldRow(Shield파라미터) → InitFromData()
	 */
	UFUNCTION(BlueprintCallable, Category="Loadout")
	bool TryApplyActiveLoadout(ADualFirePlayerPawn* Pawn, FText& OutError, FName& OutInvalidField);

	/**
	 * ActiveLoadout.AircraftID로 AircraftRow를 조회해 스폰할 Pawn 클래스를 반환.
	 * GameMode::GetDefaultPawnClassForController_Implementation에서 호출.
	 * AircraftRow를 못 찾으면 nullptr (호출측이 기본 DefaultPawnClass로 폴백).
	 */
	UFUNCTION(BlueprintCallable, Category="Loadout")
	TSubclassOf<ADualFirePlayerPawn> ResolveAircraftClass() const;

private:
	void CommitActiveLoadout(const FLoadout& Loadout);

	/** 현재 선택된 로드아웃. 검증 성공 후에만 CommitActiveLoadout으로 설정 */
	UPROPERTY()
	FLoadout ActiveLoadout;

	bool bHasActiveLoadout = false;
};
