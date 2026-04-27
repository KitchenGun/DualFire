// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "DualFirePlayerPawn.generated.h"

// Forward declarations — 헤더 인클루드 최소화
class USceneComponent;
class USkeletalMeshComponent;
class USphereComponent;
class UDualFireMovementComponent;
class UWeaponComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

/**
 * 2D 스크롤 슈팅 플레이어 폰 (단일 플레이어 전용).
 * APawn 기반, UDualFireMovementComponent로 XY 평면 이동.
 * Enhanced Input → AddMovementInput → MovementComp->ConsumeInputVector 흐름.
 * 비주얼: USkeletalMeshComponent (ShipMesh), 충돌: USphereComponent (HitboxComp)
 *
 * 미구현: HP/잔기, 사격 (BulletClass 스텁 프로퍼티만 보유)
 */
UCLASS(BlueprintType, Blueprintable)
class DUALFIRE_API ADualFirePlayerPawn : public APawn
{
    GENERATED_BODY()

    // ── 컴포넌트 (private + Blueprint 노출) ─────────────────────────────────

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components",
        meta=(AllowPrivateAccess="true"))
    TObjectPtr<USceneComponent> SceneRoot;

    // 3D 스켈레탈 메시 비주얼. 충돌 없음 (히트박스는 HitboxComp 전담)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components",
        meta=(AllowPrivateAccess="true"))
    TObjectPtr<USkeletalMeshComponent> Mesh;

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

public:
    ADualFirePlayerPawn();

    // ── APawn 오버라이드 ─────────────────────────────────────────────────────

    virtual void BeginPlay() override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    /** MovementComp를 반환. AIController 등 외부 시스템이 이동 컴포넌트를 찾을 때 사용 */
    virtual UPawnMovementComponent* GetMovementComponent() const override;

    // ── Enhanced Input 에셋 참조 ─────────────────────────────────────────────
    // TSoftObjectPtr: 패키징 크기 절약. BeginPlay/SetupInput에서 LoadSynchronous() 사용.
    // BP_DualFirePlayerPawn의 Details 패널에서 에셋 직접 할당.

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
    TSoftObjectPtr<UInputMappingContext> IMC_Player;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
    TSoftObjectPtr<UInputAction> IA_Move;

    /** 대지 무장 발사 (K키) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
    TSoftObjectPtr<UInputAction> IA_FireGround;

    /** 대공 무장 발사 (J키) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
    TSoftObjectPtr<UInputAction> IA_FireAir;

    /** 범용 무장 발사 (L키) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
    TSoftObjectPtr<UInputAction> IA_FireUniversal;

    // 여러 IMC가 스택될 때 우선순위. 높을수록 먼저 처리
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input",
        meta=(ClampMin="0"))
    int32 InputMappingPriority = 1;

    // ── 이동 제어 위임 ───────────────────────────────────────────────────────

    /** 슈퍼웨폰 발동 등에서 이동 속도 배율 변경. MovementComp에 위임 */
    UFUNCTION(BlueprintCallable, Category="Movement")
    void SetSpeedMultiplier(float InMultiplier);

    /** 스턴/연출 중 이동 잠금. MovementComp에 위임 */
    UFUNCTION(BlueprintCallable, Category="Movement")
    void SetMovementLocked(bool bLocked);

protected:
    // ── Enhanced Input 핸들러 ────────────────────────────────────────────────

    /** IA_Move (Axis2D) 입력 처리. Triggered 이벤트로 매 프레임 호출 */
    void OnMoveInput(const FInputActionValue& Value);

    /** 대지 무장 발사 (K키, Triggered=연사) */
    void OnFireGroundInput(const FInputActionValue& Value);

    /** 대공 무장 발사 (J키, Triggered=연사) */
    void OnFireAirInput(const FInputActionValue& Value);

    /** 범용 무장 발사 (L키, Triggered=연사) */
    void OnFireUniversalInput(const FInputActionValue& Value);

    // ── 히트박스 오버랩 ──────────────────────────────────────────────────────

    /**
     * HitboxComp의 OnComponentBeginOverlap 콜백.
     * UFUNCTION 필수 — AddDynamic 바인딩이 리플렉션을 요구함
     */
    UFUNCTION()
    void OnHitboxOverlapBegin(
        UPrimitiveComponent* OverlappedComp,
        AActor*              OtherActor,
        UPrimitiveComponent* OtherComp,
        int32                OtherBodyIndex,
        bool                 bFromSweep,
        const FHitResult&    SweepResult);

private:
    /** IMC를 로드해 LocalPlayer Subsystem에 등록. BeginPlay에서 1회 호출 */
    void SetupInputMappingContext();
};
