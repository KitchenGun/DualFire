// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StageCameraActor.generated.h"

// Forward declarations — 헤더 인클루드 최소화
class USceneComponent;
class UCameraComponent;

/**
 * 스테이지 전용 직교 카메라 액터.
 * GameMode가 BeginPlay에서 스폰 후 SetViewTarget으로 지정.
 * ScrollSpeed > 0 이면 Tick마다 +X 방향으로 자동 이동.
 * GetPlayableBounds()로 이동 가능 영역(XY 평면) FBox2D 제공.
 */
UCLASS(BlueprintType, Blueprintable)
class DUALFIRE_API AStageCameraActor : public AActor
{
    GENERATED_BODY()

    // ── 컴포넌트 (private + Blueprint 노출) ────────────────────────────────────

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components",
        meta=(AllowPrivateAccess="true"))
    TObjectPtr<USceneComponent> SceneRoot;

    // +Z 위치에서 -Z 방향(XY 게임 평면)을 내려다보는 직교 카메라. Pitch=-90
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components",
        meta=(AllowPrivateAccess="true"))
    TObjectPtr<UCameraComponent> CameraComp;

public:
    AStageCameraActor();

    // ── AActor 오버라이드 ────────────────────────────────────────────────────────
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void Tick(float DeltaTime) override;

    // ── 카메라 설정 ──────────────────────────────────────────────────────────────

    // 직교 투영 너비 (월드 단위). 4:3 기준 1600x1200 플레이 영역
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Camera",
        meta=(ClampMin="256.0"))
    float OrthoWidth = 1600.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Camera",
        meta=(ClampMin="0.1"))
    float AspectRatio = 4.f / 3.f;

    // ── 스크롤 설정 ──────────────────────────────────────────────────────────────

    // +X 방향 자동 이동 속도 (units/s). 0 = 정지
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scroll")
    float ScrollSpeed = 200.f;

    // GetPlayableBounds() 마진. X=화면 가로(월드 Y), Y=화면 세로(월드 X)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scroll")
    FVector2D PlayableInset = FVector2D(64.f, 64.f);

    /** 스크롤 속도 변경. 연출·보스 페이즈 전환 시 호출 */
    UFUNCTION(BlueprintCallable, Category="Scroll")
    void SetScrollSpeed(float InSpeed);

    /** 카메라(및 스크롤) 일시 정지 토글 */
    UFUNCTION(BlueprintCallable, Category="Scroll")
    void SetPaused(bool bInPaused);

    /** 현재 스크롤 속도 벡터. 일시 정지 또는 속도 0이면 Zero */
    UFUNCTION(BlueprintPure, Category="Scroll")
    FVector GetScrollVelocity() const;

    /**
     * 현재 카메라 프러스텀에서 XY 이동 가능 영역 반환.
     * FBox2D.X = 월드 X(앞뒤), FBox2D.Y = 월드 Y(좌우).
     * OrthoWidth + 고정 종횡비 + PlayableInset 적용.
     */
    UFUNCTION(BlueprintPure, Category="Scroll")
    FBox2D GetPlayableBounds() const;

    /** 직교 카메라가 보이는 플레이필드의 월드 높이를 계산한다. */
    static float CalculatePlayableWorldHeight(float InOrthoWidth, float InAspectRatio);

    /**
     * RenderHeightRatio를 월드 Z 높이와 투영 보정을 포함한 오프셋으로 변환한다.
     * 카메라 전방 벡터를 따라 보정해 메시의 화면 앵커를 판정 위치에 유지한다.
     */
    static FVector CalculateRenderHeightOffset(
        float InRenderHeightRatio,
        float InOrthoWidth,
        float InAspectRatio,
        const FRotator& InCameraRotation);

    /** 현재 카메라 설정과 회전으로 시각 메시 보정 오프셋을 계산한다. */
    UFUNCTION(BlueprintPure, Category="Camera")
    FVector GetRenderHeightOffset(float InRenderHeightRatio) const;

private:
    void ApplyCameraSettings();

    // SetPaused로만 변경
    UPROPERTY(BlueprintReadOnly, Category="Scroll", meta=(AllowPrivateAccess="true"))
    bool bPaused = false;
};
