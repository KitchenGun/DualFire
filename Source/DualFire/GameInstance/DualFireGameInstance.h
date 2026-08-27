// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Types/SlateEnums.h"
#include "DualFireGameInstance.generated.h"

class UDataTable;

/**
 * DualFire 게임 인스턴스.
 * 레벨 전환 간 유지되는 게임 전역 상태를 담당한다.
 * 서브시스템(ULoadoutManagerSubsystem 등)은 이 클래스를 통해 자동 등록된다.
 */
UCLASS()
class DUALFIRE_API UDualFireGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;

	UFUNCTION(BlueprintPure, Category="Mission Flow|Data")
	UDataTable* GetMissionDataTable() const { return MissionDataTable; }

	UFUNCTION(BlueprintPure, Category="Mission Flow|Data")
	UDataTable* GetStageDataTable() const { return StageDataTable; }

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mission Flow|Data", meta=(RequiredAssetDataTags="RowStructure=/Script/DualFire.MissionRow"))
	TObjectPtr<UDataTable> MissionDataTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mission Flow|Data", meta=(RequiredAssetDataTags="RowStructure=/Script/DualFire.StageRow"))
	TObjectPtr<UDataTable> StageDataTable;

private:
	bool bMenuNavigationInstalled = false;
	bool bHadWNavigationRule = false;
	bool bHadSNavigationRule = false;
	EUINavigation PreviousWNavigation = EUINavigation::Invalid;
	EUINavigation PreviousSNavigation = EUINavigation::Invalid;
};
