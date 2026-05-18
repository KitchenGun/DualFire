// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Core/DualFireTypes.h"
#include "EnemyAttributeInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UEnemyAttributeInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 적/파츠가 자신의 Ground/Air 속성을 외부에 노출하기 위한 인터페이스.
 * 탄환의 AttributeArray 배열과 비교해 데미지 적용 여부를 결정.
 *
 * 단일 액터가 단일 속성을 갖는다고 가정. 보스 파츠처럼 복합 속성이 필요한 경우
 * 파츠를 별도 액터로 분리해 각자 인터페이스 구현.
 */
class DUALFIRE_API IEnemyAttributeInterface
{
	GENERATED_BODY()
public:
	/** Preferred enemy attribute contract. Mixed enemies return both Ground and Air. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Weapon")
	FEnemyAttribute GetEnemyAttributes() const;
	virtual FEnemyAttribute GetEnemyAttributes_Implementation() const
	{
		return FEnemyAttribute::FromAttribute(GetEnemyAttribute_Implementation());
	}
	/** Legacy single-attribute contract kept for existing Blueprint assets. Use GetEnemyAttributes instead. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Weapon", meta=(DeprecatedFunction, DeprecationMessage="Use GetEnemyAttributes instead."))
	EDualFireAttribute GetEnemyAttribute() const;
	virtual EDualFireAttribute GetEnemyAttribute_Implementation() const
	{
		return EDualFireAttribute::None;
	}
};
