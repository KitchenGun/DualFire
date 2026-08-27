// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DualFireStageDataLibrary.generated.h"

class UCurveFloat;

/** Python/MCP 스테이지 데이터 저작에서 UCurveFloat 키를 안전하게 설정한다. */
UCLASS()
class DUALFIRE_API UDualFireStageDataLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Stage|Authoring")
	static bool SetNormalizedCurveKeys(UCurveFloat* Curve, const TArray<FVector2D>& TimeValueKeys);
};
