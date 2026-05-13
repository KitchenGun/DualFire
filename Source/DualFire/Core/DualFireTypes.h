// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DualFireTypes.generated.h"

/** Shared Ground/Air attribute used by P0 data and matching code. */
UENUM(BlueprintType)
enum class EDualFireAttribute : uint8
{
	None   UMETA(DisplayName = "None"),
	Ground UMETA(DisplayName = "Ground"),
	Air    UMETA(DisplayName = "Air"),
};

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	Ground    UMETA(DisplayName = "Ground"),
	Air       UMETA(DisplayName = "Air"),
	Universal UMETA(DisplayName = "Universal"),
};

UENUM(BlueprintType)
enum class EWeaponCategory : uint8
{
	Primary UMETA(DisplayName = "Primary"),
	Special UMETA(DisplayName = "Special"),
};

UENUM(BlueprintType)
enum class EFireMode : uint8
{
	Auto       UMETA(DisplayName = "Auto"),
	SingleShot UMETA(DisplayName = "Single Shot"),
};

UENUM(BlueprintType)
enum class EHitBehavior : uint8
{
	Destroy          UMETA(DisplayName = "Destroy"),
	Penetrate        UMETA(DisplayName = "Penetrate"),
	LimitedPenetrate UMETA(DisplayName = "Limited Penetrate"),
};

/** Prototype screen-space spawn anchors. These are logical in-screen entry points, not exact screen bounds. */
UENUM(BlueprintType)
enum class ESpawnAnchor : uint8
{
	TopLeft      UMETA(DisplayName = "Top Left"),
	TopCenter    UMETA(DisplayName = "Top Center"),
	TopRight     UMETA(DisplayName = "Top Right"),
	Left         UMETA(DisplayName = "Left"),
	Center       UMETA(DisplayName = "Center"),
	Right        UMETA(DisplayName = "Right"),
	BottomLeft   UMETA(DisplayName = "Bottom Left"),
	BottomCenter UMETA(DisplayName = "Bottom Center"),
	BottomRight  UMETA(DisplayName = "Bottom Right"),
};

UENUM(BlueprintType)
enum class EStageState : uint8
{
	Timeline    UMETA(DisplayName = "Timeline"),
	EliteCombat UMETA(DisplayName = "Elite Combat"),
	Ended       UMETA(DisplayName = "Ended"),
};

UENUM(BlueprintType)
enum class EMissionResult : uint8
{
	None    UMETA(DisplayName = "None"),
	Cleared UMETA(DisplayName = "Cleared"),
	Failed  UMETA(DisplayName = "Failed"),
};

UENUM(BlueprintType)
enum class EEnemyMovementPattern : uint8
{
	Linear    UMETA(DisplayName = "Linear"),
	EnterStop UMETA(DisplayName = "Enter Stop"),
	Hover     UMETA(DisplayName = "Hover"),
};

UENUM(BlueprintType)
enum class EEnemyAttackPattern : uint8
{
	None    UMETA(DisplayName = "None"),
	Single  UMETA(DisplayName = "Single"),
	Spread3 UMETA(DisplayName = "Spread 3"),
	Rotate3 UMETA(DisplayName = "Rotate 3"),
};

UENUM(BlueprintType)
enum class EAttributeVisualStyle : uint8
{
	ColorOnly               UMETA(DisplayName = "Color Only"),
	ColorAndShape           UMETA(DisplayName = "Color + Shape"),
	ColorShapeAndShadow     UMETA(DisplayName = "Color + Shape + Shadow"),
	ColorShapeAndLayer      UMETA(DisplayName = "Color + Shape + Position Layer"),
};

/** Enemy-side attribute flags. Mixed enemies set both bGround and bAir. */
USTRUCT(BlueprintType)
struct DUALFIRE_API FEnemyAttribute
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute")
	bool bGround = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute")
	bool bAir = false;
};
