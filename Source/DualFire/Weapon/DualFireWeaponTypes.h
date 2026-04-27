// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DualFireWeaponTypes.generated.h"

/**
 * 탄환/무장 속성. TArray로 복수 속성 보유.
 * 범용 무장은 [Ground, Air] 두 값을 배열에 모두 넣는 방식으로 표현.
 */
UENUM(BlueprintType)
enum class EWeaponAttribute : uint8
{
	Ground  UMETA(DisplayName="Ground"),
	Air     UMETA(DisplayName="Air"),
};

/** 플레이어의 3가지 무장 슬롯 식별자 */
UENUM(BlueprintType)
enum class EWeaponSlot : uint8
{
	Ground    UMETA(DisplayName="Ground"),
	Air       UMETA(DisplayName="Air"),
	Universal UMETA(DisplayName="Universal"),
};
