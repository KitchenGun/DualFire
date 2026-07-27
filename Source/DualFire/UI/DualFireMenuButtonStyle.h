// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "DualFireMenuButtonStyle.generated.h"

/** 메뉴 버튼의 상태별 외형과 공통 크기를 정의하는 기본 Common UI 스타일이다. */
UCLASS(BlueprintType, Blueprintable)
class DUALFIRE_API UDualFireMenuButtonStyle : public UCommonButtonStyle
{
	GENERATED_BODY()

public:
	UDualFireMenuButtonStyle();
};
