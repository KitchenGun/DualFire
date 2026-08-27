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
enum class EWeaponCategory : uint8
{
	Primary UMETA(DisplayName = "Primary"),
	Special UMETA(DisplayName = "Special"),
};

UENUM(BlueprintType)
enum class ELoadoutSlot : uint8
{
	PrimaryWeapon  UMETA(DisplayName = "Primary Weapon"),
	SpecialWeapon1 UMETA(DisplayName = "Special Weapon 1"),
	SpecialWeapon2 UMETA(DisplayName = "Special Weapon 2"),
	SuperWeapon    UMETA(DisplayName = "Super Weapon"),
	Shield         UMETA(DisplayName = "Shield"),
};

UENUM(BlueprintType)
enum class EHitBehavior : uint8
{
	Destroy          UMETA(DisplayName = "Destroy"),
	Penetrate        UMETA(DisplayName = "Penetrate"),
	LimitedPenetrate UMETA(DisplayName = "Limited Penetrate"),
};

UENUM(BlueprintType)
enum class ESuperWeaponEffectType : uint8
{
	None         UMETA(DisplayName = "None"),
	Damage       UMETA(DisplayName = "Damage"),
	Recovery     UMETA(DisplayName = "Recovery"),
	Invincibility UMETA(DisplayName = "Invincibility"),
	Movement     UMETA(DisplayName = "Movement"),
	Attack       UMETA(DisplayName = "Attack"),
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
enum class EDualFireMissionFailureReason : uint8
{
	None                 UMETA(DisplayName = "None"),
	PlayerDestroyed      UMETA(DisplayName = "Player Destroyed"),
	StageConditionFailed UMETA(DisplayName = "Stage Condition Failed"),
};

UENUM(BlueprintType)
enum class EDualFireStartRoute : uint8
{
	None      UMETA(DisplayName = "None"),
	Campaign  UMETA(DisplayName = "Campaign"),
	Briefing  UMETA(DisplayName = "Briefing"),
};

UENUM(BlueprintType)
enum class EStagePauseResumeCondition : uint8
{
	RealTime       UMETA(DisplayName = "Real Time"),
	WaveDefeated   UMETA(DisplayName = "Wave Defeated"),
	EnemyDefeated  UMETA(DisplayName = "Enemy Defeated"),
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

/** Enemy-side attribute flags. Mixed enemies set both bGround and bAir. */
USTRUCT(BlueprintType)
struct DUALFIRE_API FEnemyAttribute
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute")
	bool bGround = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute")
	bool bAir = false;

	bool HasGround() const { return bGround; }
	bool HasAir() const { return bAir; }
	bool IsMixed() const { return bGround && bAir; }
	bool IsNone() const { return !bGround && !bAir; }
	bool Matches(EDualFireAttribute Attribute) const
	{
		switch (Attribute)
		{
		case EDualFireAttribute::Ground:
			return bGround;
		case EDualFireAttribute::Air:
			return bAir;
		default:
			return false;
		}
	}
	bool MatchesAny(const TArray<EDualFireAttribute>& Attributes) const
	{
		for (const EDualFireAttribute Attribute : Attributes)
		{
			if (Matches(Attribute))
			{
				return true;
			}
		}
		return false;
	}
	static FEnemyAttribute FromAttribute(EDualFireAttribute Attribute)
	{
		FEnemyAttribute Result;
		Result.bGround = Attribute == EDualFireAttribute::Ground;
		Result.bAir = Attribute == EDualFireAttribute::Air;
		return Result;
	}
	static FEnemyAttribute FromAttributes(const TArray<EDualFireAttribute>& Attributes)
	{
		FEnemyAttribute Result;
		Result.bGround = Attributes.Contains(EDualFireAttribute::Ground);
		Result.bAir = Attributes.Contains(EDualFireAttribute::Air);
		return Result;
	}
	static bool IsMatch(const TArray<EDualFireAttribute>& ProjectileAttributes, const FEnemyAttribute& EnemyAttribute)
	{
		return EnemyAttribute.MatchesAny(ProjectileAttributes);
	}
};
