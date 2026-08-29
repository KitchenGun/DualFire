// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/DualFireTypes.h"
#include "UObject/Object.h"
#include "DualFireHangarTypes.generated.h"

class UTexture2D;

UENUM(BlueprintType)
enum class EDualFireHangarCategory : uint8
{
	Aircraft,
	PrimaryWeapon,
	SpecialWeapon1,
	SpecialWeapon2,
	SuperWeapon,
	Shield,
};

USTRUCT(BlueprintType)
struct DUALFIRE_API FDualFireHangarItemViewData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Hangar")
	EDualFireHangarCategory Category = EDualFireHangarCategory::Aircraft;

	UPROPERTY(BlueprintReadOnly, Category="Hangar")
	FName ItemID = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category="Hangar")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category="Hangar")
	TSoftObjectPtr<UTexture2D> Icon;

	/** 무기가 공격 가능한 대상 속성의 로컬라이즈된 격납고 표시 텍스트다. */
	UPROPERTY(BlueprintReadOnly, Category="Hangar|Presentation")
	FText TargetAttributeLabel;

	/** TargetAttributeLabel에 함께 적용할 색상이다. */
	UPROPERTY(BlueprintReadOnly, Category="Hangar|Presentation")
	FLinearColor TargetAttributeColor = FLinearColor::White;

	/** 무기 항목일 때만 true이며, 비무기 항목은 WBP에서 속성 레이블을 숨긴다. */
	UPROPERTY(BlueprintReadOnly, Category="Hangar|Presentation")
	bool bHasTargetAttributePresentation = false;

	/** FWeaponRow::AttributeArray를 격납고용 대상 속성 텍스트와 색상으로 변환한다. */
	void SetTargetAttributePresentation(const TArray<EDualFireAttribute>& Attributes);

	bool IsValid() const { return !ItemID.IsNone(); }
};

/** UCommonListView가 표시하는 단일 격납고 항목 데이터다. */
UCLASS(BlueprintType)
class DUALFIRE_API UDualFireHangarItemObject : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(const FDualFireHangarItemViewData& InItem, bool bInEquipped)
	{
		Item = InItem;
		bEquipped = bInEquipped;
	}

	UPROPERTY(BlueprintReadOnly, Category="Hangar")
	FDualFireHangarItemViewData Item;

	UPROPERTY(BlueprintReadOnly, Category="Hangar")
	bool bEquipped = false;
};
