// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "DualFireGameInstance.generated.h"

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
};
