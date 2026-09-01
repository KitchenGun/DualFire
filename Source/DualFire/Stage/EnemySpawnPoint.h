// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TargetPoint.h"
#include "EnemySpawnPoint.generated.h"

/** 레벨에 배치하는 데이터 기반 적 스폰 위치. 회전은 스폰에 사용하지 않는다. */
UCLASS()
class DUALFIRE_API AEnemySpawnPoint : public ATargetPoint
{
	GENERATED_BODY()

public:
	UPROPERTY(EditInstanceOnly, Category = "Spawn Point")
	FName SpawnPointID = NAME_None;
};
