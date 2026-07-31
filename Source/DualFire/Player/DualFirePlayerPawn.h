// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Health/HealthComponent.h"
#include "Weapon/WeaponComponent.h"
#include "DualFirePlayerPawn.generated.h"

// Forward declarations — 헤더 인클루드 최소화
class USceneComponent;
class USphereComponent;
class UPaperFlipbook;
class UPaperFlipbookComponent;
class UDualFireMovementComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

/** 기체 시트의 고정 프레임 순서. 값은 Flipbook 프레임 인덱스와 일치한다. */
UENUM(BlueprintType)
enum class EAircraftBankPose : uint8
{
    Right45 = 0,
    Right30 = 1,
    Right15 = 2,
    Neutral = 3,
    Left15  = 4,
    Left30  = 5,
    Left45  = 6,
};

/**
 * 2D 스크롤 슈팅 플레이어 폰 (단일 플레이어 전용).
 * APawn 기반, UDualFireMovementComponent로 XY 평면 이동.
 * Enhanced Input → AddMovementInput → MovementComp->ConsumeInputVector 흐름.
 * 비주얼: UPaperFlipbookComponent (AircraftVisual), 충돌: USphereComponent (HitboxComp)
 *
 * 미구현: HP/잔여 기체, 사격 (BulletClass 스텁 프로퍼티만 보유)
 */
UCLASS(BlueprintType, Blueprintable)
class DUALFIRE_API ADualFirePlayerPawn : public APawn
{
    GENERATED_BODY()

    // ── 컴포넌트 (private + Blueprint 노출) ─────────────────────────────────

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components",
        meta=(AllowPrivateAccess="true"))
    TObjectPtr<USceneComponent> SceneRoot;

    // 7포즈 Paper2D 기체 비주얼. 충돌 없음 (히트박스는 HitboxComp 전담)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components",
        meta=(AllowPrivateAccess="true"))
    TObjectPtr<UPaperFlipbookComponent> AircraftVisual;

    // 피격 감지 전용 히트박스. Profile="PlayerPawn" (ObjectType=PlayerHitbox, EnemyBullet=Overlap)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components",
        meta=(AllowPrivateAccess="true"))
    TObjectPtr<USphereComponent> HitboxComp;

    // XY 평면 이동, 화면 경계 클램핑. UpdatedComponent=SceneRoot
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components",
        meta=(AllowPrivateAccess="true"))
    TObjectPtr<UDualFireMovementComponent> MovementComp;

    // 대지/대공/범용 3슬롯 무장 관리 컴포넌트
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components",
        meta=(AllowPrivateAccess="true"))
    TObjectPtr<UWeaponComponent> WeaponComp;

    // HP / Shield / 무적 관리. 적과 공유 가능한 범용 컴포넌트
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components",
        meta=(AllowPrivateAccess="true"))
    TObjectPtr<UHealthComponent> HealthComp;

public:
    ADualFirePlayerPawn();

    // ── APawn 오버라이드 ─────────────────────────────────────────────────────

    virtual void BeginPlay() override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    /** MovementComp를 반환. AIController 등 외부 시스템이 이동 컴포넌트를 찾을 때 사용 */
    virtual UPawnMovementComponent* GetMovementComponent() const override;

    // ── Enhanced Input 에셋 참조 ─────────────────────────────────────────────
    // TSoftObjectPtr: 패키징 크기 절약. MissionPlayerController가 빙의 시 로드한다.
    // BP_DualFirePlayerPawn의 Details 패널에서 에셋 직접 할당.

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
    TSoftObjectPtr<UInputMappingContext> IMC_Player;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
    TSoftObjectPtr<UInputAction> IA_Move;

    /** 기본 무기 슬롯 발사 (키보드 Z/Space, 게임패드 A) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
    TSoftObjectPtr<UInputAction> IA_FirePrimary;

    /** 특수무장 슬롯 1 발사 (키보드 X, 게임패드 B) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
    TSoftObjectPtr<UInputAction> IA_FireSpecial1;

    /** 특수무장 슬롯 2 발사 (키보드 C, 게임패드 X) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
    TSoftObjectPtr<UInputAction> IA_FireSpecial2;

    // 여러 IMC가 스택될 때 우선순위. 높을수록 먼저 처리
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input",
        meta=(ClampMin="0"))
    int32 InputMappingPriority = 1;

    /** MissionPlayerController가 수명주기를 관리할 gameplay IMC를 해석한다. */
    UInputMappingContext* ResolveInputMappingContext() const;

    int32 GetInputMappingPriority() const { return InputMappingPriority; }

    // ── 이동 제어 위임 ───────────────────────────────────────────────────────

    // ── 컴포넌트 접근자 (LoadoutManager 등 외부 시스템에서 사용) ────────────────

    UFUNCTION(BlueprintPure, Category="Components")
    UWeaponComponent* GetWeaponComp() const { return WeaponComp; }

    UFUNCTION(BlueprintPure, Category="Components")
    UHealthComponent* GetHealthComp() const { return HealthComp; }

    /** DataTable에서 로드한 기체 Flipbook을 현재 Pawn 외형에 적용한다. */
    UFUNCTION(BlueprintCallable, Category="Aircraft")
    void ApplyAircraftVisual(UPaperFlipbook* InFlipbook);

    /** 자동 재생 없이 지정된 뱅킹 프레임 하나를 표시한다. */
    UFUNCTION(BlueprintCallable, Category="Aircraft")
    void SetAircraftBankPose(EAircraftBankPose Pose);

    /** 슈퍼웨폰 발동 등에서 이동 속도 배율 변경. MovementComp에 위임 */
    UFUNCTION(BlueprintCallable, Category="Movement")
    void SetSpeedMultiplier(float InMultiplier);

    /** 스턴/연출 중 이동 잠금. MovementComp에 위임 */
    UFUNCTION(BlueprintCallable, Category="Movement")
    void SetMovementLocked(bool bLocked);

    // ── 디버그 콘솔 명령 (Exec — 에디터 PIE 콘솔에서 호출) ───────────────────

    /** ex) DF_Damage 3  →  HealthComp에 데미지 3 즉시 적용 */
    UFUNCTION(Exec)
    void DF_Damage(int32 Amount);

    /** ex) DF_RecoverHealth  →  HP 만회 */
    UFUNCTION(Exec)
    void DF_RecoverHealth();

    /** ex) DF_RecoverShield  →  Shield 만회 */
    UFUNCTION(Exec)
    void DF_RecoverShield();

    /** ex) DF_Kill  →  HP를 한 번에 0으로 (잔여 기체/리스폰/사망 흐름 검증용) */
    UFUNCTION(Exec)
    void DF_Kill();

	UFUNCTION(BlueprintPure, Category="Mission Result")
	int32 GetMissionHitCount() const { return MissionHitCount; }

	UFUNCTION(BlueprintPure, Category="Mission Result")
	int32 GetMissionDeathCount() const { return MissionDeathCount; }

protected:
    // ── Enhanced Input 핸들러 ────────────────────────────────────────────────

    /** IA_Move (Axis2D) 입력 처리. Triggered 이벤트로 매 프레임 호출 */
    void OnMoveInput(const FInputActionValue& Value);

    /** 이동 입력 종료 시 기체 포즈를 중립으로 되돌린다. */
    void OnMoveInputCompleted(const FInputActionValue& Value);

    /** 기본 무기 슬롯 발사 (Triggered=연사) */
    void OnFirePrimaryInput(const FInputActionValue& Value);

    /** 특수무장 슬롯 1 발사 (Triggered=연사) */
    void OnFireSpecial1Input(const FInputActionValue& Value);

    /** 특수무장 슬롯 2 발사 (Triggered=연사) */
    void OnFireSpecial2Input(const FInputActionValue& Value);

    // ── 사망 처리 ────────────────────────────────────────────────────────────

    /**
     * HealthComp.OnDeath(잔여 기체 소진 최종 사망) 콜백.
     * GameMode->OnMissionFail()로 연결. UFUNCTION 필수 (델리게이트 바인딩).
     */
    UFUNCTION()
    void OnPlayerFinalDeath();

	UFUNCTION()
	void OnPlayerRespawn();

	UFUNCTION()
	void OnMissionHealthChanged(int32 CurrentHealth, int32 MaxHealth);

	UFUNCTION()
	void OnMissionShieldChanged(int32 CurrentShield, int32 MaxShield);

private:
	int32 MissionHitCount = 0;
	int32 MissionDeathCount = 0;
	int32 LastRecordedHealth = 0;
	int32 LastRecordedShield = 0;

    /** 좌우 입력 세기를 15/30/45도 뱅킹 포즈로 변환한다. */
    void UpdateAircraftBankPose(float HorizontalInput);

};
