// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
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
	FText Description;

	UPROPERTY(BlueprintReadOnly, Category="Hangar")
	TSoftObjectPtr<UTexture2D> Icon;

	bool IsValid() const { return !ItemID.IsNone(); }
};

/** UCommonListView가 표시하는 단일 격납고 항목 데이터다. */
UCLASS(BlueprintType)
class DUALFIRE_API UDualFireHangarItemObject : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(const FDualFireHangarItemViewData& InItem, bool bInEquipped);

	UPROPERTY(BlueprintReadOnly, Category="Hangar")
	FDualFireHangarItemViewData Item;

	UPROPERTY(BlueprintReadOnly, Category="Hangar")
	bool bEquipped = false;
};
