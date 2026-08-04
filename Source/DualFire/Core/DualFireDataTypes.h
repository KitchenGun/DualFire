// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "DualFireTypes.h"
#include "DualFireDataTypes.generated.h"

class USkeletalMesh;
class UStaticMesh;
class UTexture2D;
class UPaperFlipbook;
class ABaseProjectile;
class UMaterialInterface;
class ADualFirePlayerPawn;

namespace DualFireLoadout
{
	constexpr int32 MaxPresetCount = 50;
}

USTRUCT(BlueprintType)
struct DUALFIRE_API FWeaponRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 무기 고유 식별자. 로드아웃(FLoadout)이 이 ID로 무기를 참조한다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FName WeaponID = NAME_None;

	/** UI에 표시되는 무기 이름 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FText DisplayName;

	/** UI에 표시되는 무기 설명 텍스트 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FText Description;

	/** 슬롯 분류. Primary=기본 무기 슬롯, Special=특수 무장 슬롯 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	EWeaponCategory Category = EWeaponCategory::Primary;

	/** 발사할 투사체 액터 클래스 (ABaseProjectile 파생) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TSoftClassPtr<ABaseProjectile> ProjectileClass;

	/** 이 무기가 명중시킬 수 있는 속성(지상/공중). 복수 선택 시 양쪽 모두 명중 가능 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TArray<EDualFireAttribute> AttributeArray;

	/** 탄환 1발의 데미지 (최소 1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "1"))
	int32 BaseDamage = 1;

	/** 초당 발사 수 (0보다 커야 함) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "0.001"))
	float FireRate = 1.0f;

	/** 탄환 이동 속도 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "0.0"))
	float ProjectileSpeed = 1000.0f;

	/** 명중 처리 방식. Destroy=소멸, Penetrate=무한 관통, LimitedPenetrate=PenetrationLimit만큼만 관통 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	EHitBehavior HitBehavior = EHitBehavior::Destroy;

	/** HitBehavior가 LimitedPenetrate일 때만 사용하는 관통 가능 횟수 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "0"))
	int32 PenetrationLimit = 0;

	/** 기체 기준 탄환 발사 지점 오프셋 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FVector MuzzleOffset = FVector::ZeroVector;

	/** 탄환 외형 StaticMesh (공용 메시. 무기별 구분은 ProjectileMaterial 교체로 처리) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TSoftObjectPtr<UStaticMesh> ProjectileMesh;

	/** 탄환에 적용할 머티리얼. 속성(지상/공중) 식별 및 무기별 외형 차별화를 머티리얼 교체로 처리 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TSoftObjectPtr<UMaterialInterface> ProjectileMaterial;

	/** 로드아웃 UI에 표시할 무기 아이콘 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TSoftObjectPtr<UTexture2D> Icon;

	/** 무기 분류 태그 (UI 필터링/그룹핑용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FName CategoryTag = NAME_None;

	/** 해금 조건 ID. 장비해금 시스템과 연동 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FName UnlockID = NAME_None;
};

USTRUCT(BlueprintType)
struct DUALFIRE_API FEnemyRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	FName EnemyID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	FEnemyAttribute Attribute;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (ClampMin = "1"))
	int32 MaxHealth = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	TSoftObjectPtr<USkeletalMesh> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	EEnemyMovementPattern MovementPattern = EEnemyMovementPattern::Linear;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (ClampMin = "0"))
	int32 ContactDamage = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	EEnemyAttackPattern AttackPattern = EEnemyAttackPattern::Single;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (ClampMin = "0"))
	int32 AttackDamage = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (ClampMin = "0.0"))
	float FireInterval = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (ClampMin = "0.0"))
	float EnemyProjectileSpeed = 600.0f;
};

USTRUCT(BlueprintType)
struct DUALFIRE_API FShieldRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 보호막 장비 고유 식별자. 로드아웃(FLoadout)이 이 ID로 보호막을 참조한다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shield")
	FName ShieldID = NAME_None;

	/** UI에 표시되는 보호막 이름 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shield")
	FText DisplayName;

	/** UI에 표시되는 보호막 설명 텍스트 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shield")
	FText Description;

	/** 보호막 최대 잔량. 0이면 보호막 없음 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shield", meta = (ClampMin = "0"))
	int32 MaxShield = 0;

	/** 보호막 1칸이 재생되는 데 걸리는 시간(초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shield", meta = (ClampMin = "0.0"))
	float ShieldRecoveryDuration = 0.0f;

	/** 보호막이 0이 되는 순간 부여되는 무적 시간(초). 0이면 미적용 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shield", meta = (ClampMin = "0.0"))
	float BreakInvincibilityDuration = 0.0f;

	/** 로드아웃 UI에 표시할 보호막 아이콘 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shield")
	TSoftObjectPtr<UTexture2D> Icon;

	/** 보호막 분류 태그 (UI 필터링/그룹핑용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shield")
	FName CategoryTag = NAME_None;

	/** 해금 조건 ID. 장비해금 시스템과 연동 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shield")
	FName UnlockID = NAME_None;
};

USTRUCT(BlueprintType)
struct DUALFIRE_API FSuperWeaponEffect
{
	GENERATED_BODY()

	/** 효과 종류. None/Damage(데미지)/Recovery(회복)/Invincibility(무적)/Movement(기동)/Attack(공격) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperWeapon")
	ESuperWeaponEffectType EffectType = ESuperWeaponEffectType::None;

	/** 이 효과가 적용되는 대상 속성(지상/공중). 무기의 AttributeArray와 동일한 개념 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperWeapon")
	TArray<EDualFireAttribute> AttributeArray;

	/** 효과 수치. EffectType에 따라 의미가 달라짐 (예: Damage면 데미지량, Recovery면 회복량) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperWeapon")
	float Value = 0.0f;

	/** 효과 적용 범위(반경) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperWeapon", meta = (ClampMin = "0.0"))
	float Range = 0.0f;

	/** 효과 지속 시간(초). 즉발 효과면 0 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperWeapon", meta = (ClampMin = "0.0"))
	float Duration = 0.0f;
};

USTRUCT(BlueprintType)
struct DUALFIRE_API FSuperWeaponRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 슈퍼웨폰 고유 식별자. 로드아웃(FLoadout)이 이 ID로 슈퍼웨폰을 참조한다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperWeapon")
	FName SuperWeaponID = NAME_None;

	/** UI에 표시되는 슈퍼웨폰 이름 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperWeapon")
	FText DisplayName;

	/** UI에 표시되는 슈퍼웨폰 설명 텍스트 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperWeapon")
	FText Description;

	/** 발동 시 적용되는 효과 목록. 하나의 슈퍼웨폰이 여러 효과를 동시에 가질 수 있다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperWeapon")
	TArray<FSuperWeaponEffect> Effects;

	/** 1회 발동에 소모되는 자원(스택) 수 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperWeapon", meta = (ClampMin = "0"))
	int32 ActivationCost = 0;

	/** 최대 보유(충전) 가능 스택 수 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperWeapon", meta = (ClampMin = "1"))
	int32 MaxStock = 1;

	/** 충전 속도 배율. 1.0이 기본 속도, 값이 클수록 빨리 충전 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperWeapon", meta = (ClampMin = "0.0"))
	float ChargeSpeedMultiplier = 1.0f;

	/** true면 슈퍼웨폰 사용 중에도 이동 가능 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperWeapon")
	bool bAllowMovementDuringUse = true;

	/** true면 슈퍼웨폰 사용 중에도 다른 공격(기본무기/특수무장) 가능 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperWeapon")
	bool bAllowAttackDuringUse = false;

	/** 로드아웃 UI에 표시할 슈퍼웨폰 아이콘 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperWeapon")
	TSoftObjectPtr<UTexture2D> Icon;

	/** 슈퍼웨폰 분류 태그 (UI 필터링/그룹핑용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperWeapon")
	FName CategoryTag = NAME_None;

	/** 해금 조건 ID. 장비해금 시스템과 연동 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperWeapon")
	FName UnlockID = NAME_None;
};

USTRUCT(BlueprintType)
struct DUALFIRE_API FAircraftRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 기체 고유 식별자. 로드아웃(FLoadout)이 이 ID로 기체를 참조한다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aircraft")
	FName AircraftID = NAME_None;

	/** UI에 표시되는 기체 이름 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aircraft")
	FText DisplayName;

	/** UI에 표시되는 기체 설명 텍스트 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aircraft")
	FText Description;

	/** 기체 최대 체력. HealthComponent.MaxHealth로 주입된다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aircraft", meta = (ClampMin = "1"))
	int32 MaxHealth = 1;

	/**
	 * 스폰할 공통 Pawn 블루프린트 클래스. 외형은 BankFlipbook으로 주입하고,
	 * 히트박스 크기(HitboxComp 반지름)는 이 BP의 컴포넌트 설정으로 관리한다.
	 * GameMode가 미션 시작 시 이 클래스로 플레이어 Pawn을 스폰한다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aircraft")
	TSoftClassPtr<ADualFirePlayerPawn> AircraftClass;

	/** 좌우 입력에 따라 프레임을 선택하는 7포즈 Paper2D Flipbook */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aircraft")
	TSoftObjectPtr<UPaperFlipbook> BankFlipbook;

	/** 로드아웃 UI에 표시할 기체 아이콘 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aircraft")
	TSoftObjectPtr<UTexture2D> Icon;

	/** 해금 조건 ID. 장비해금 시스템과 연동 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aircraft")
	FName UnlockID = NAME_None;
};

USTRUCT(BlueprintType)
struct DUALFIRE_API FStageRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage")
	FName StageID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage")
	float ScrollSpeed = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage", meta = (ClampMin = "0.0"))
	float EliteTriggerTime = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage")
	FName EliteEnemyID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage", meta = (ClampMin = "0.0"))
	float EliteTimeLimit = 60.0f;
};

USTRUCT(BlueprintType)
struct DUALFIRE_API FWaveRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	FName StageID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	FName WaveID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave", meta = (ClampMin = "0.0"))
	float TriggerTime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	FName EnemyID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	ESpawnAnchor SpawnAnchor = ESpawnAnchor::TopCenter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	FVector SpawnOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave", meta = (ClampMin = "1"))
	int32 Count = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave", meta = (ClampMin = "0.0"))
	float SpawnInterval = 0.0f;
};

USTRUCT(BlueprintType)
struct DUALFIRE_API FLoadout
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loadout")
	FName AircraftID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loadout")
	FName PrimaryWeaponID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loadout")
	FName SpecialWeapon1ID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loadout")
	FName SpecialWeapon2ID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loadout")
	FName SuperWeaponID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loadout")
	FName ShieldID = NAME_None;

	bool IsComplete() const
	{
		return !AircraftID.IsNone()
			&& !PrimaryWeaponID.IsNone()
			&& !SpecialWeapon1ID.IsNone()
			&& !SpecialWeapon2ID.IsNone()
			&& !SuperWeaponID.IsNone()
			&& !ShieldID.IsNone();
	}

	FName GetEquipmentID(ELoadoutSlot Slot) const
	{
		switch (Slot)
		{
		case ELoadoutSlot::PrimaryWeapon:
			return PrimaryWeaponID;
		case ELoadoutSlot::SpecialWeapon1:
			return SpecialWeapon1ID;
		case ELoadoutSlot::SpecialWeapon2:
			return SpecialWeapon2ID;
		case ELoadoutSlot::SuperWeapon:
			return SuperWeaponID;
		case ELoadoutSlot::Shield:
			return ShieldID;
		default:
			return NAME_None;
		}
	}
};

/**
 * 기획자/디자이너가 BP 디테일 패널에서 로드아웃을 손쉽게 구성하기 위한 입력용 구조체.
 * 각 필드는 FDataTableRowHandle이라 "데이터테이블 선택 → 행 이름 드롭다운" UI가 자동 제공된다
 * (FName 직접 타이핑 대비 오타 방지). RowType 메타로 잘못된 테이블 선택도 막는다.
 * 런타임 로직(FLoadout)과는 분리된 "저작 전용" 타입 — 사용 시 MakeLoadoutFromRowHandles()로 변환.
 */
USTRUCT(BlueprintType)
struct DUALFIRE_API FLoadoutRowHandles
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loadout", meta = (RowType = "/Script/DualFire.AircraftRow"))
	FDataTableRowHandle AircraftRow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loadout", meta = (RowType = "/Script/DualFire.WeaponRow"))
	FDataTableRowHandle PrimaryWeaponRow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loadout", meta = (RowType = "/Script/DualFire.WeaponRow"))
	FDataTableRowHandle SpecialWeapon1Row;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loadout", meta = (RowType = "/Script/DualFire.WeaponRow"))
	FDataTableRowHandle SpecialWeapon2Row;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loadout", meta = (RowType = "/Script/DualFire.SuperWeaponRow"))
	FDataTableRowHandle SuperWeaponRow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loadout", meta = (RowType = "/Script/DualFire.ShieldRow"))
	FDataTableRowHandle ShieldRow;
};

USTRUCT(BlueprintType)
struct DUALFIRE_API FLoadoutPreset
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loadout")
	FName PresetID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loadout")
	FText PresetName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loadout")
	FLoadout Loadout;

	bool IsComplete() const
	{
		return !PresetID.IsNone() && Loadout.IsComplete();
	}
};
