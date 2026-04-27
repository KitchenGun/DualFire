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

    // -Y 위치에서 +Y 방향(XZ 게임 평면) 바라봄. Yaw=90, 직교 투영
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components",
        meta=(AllowPrivateAccess="true"))
    TObjectPtr<UCameraComponent> CameraComp;

public:
    AStageCameraActor();

    // ── AActor 오버라이드 ────────────────────────────────────────────────────────
    virtual void Tick(float DeltaTime) override;

    // ── 카메라 설정 ──────────────────────────────────────────────────────────────

    // 직교 투영 너비 (월드 단위). 16:9 기준 높이 = OrthoWidth / AspectRatio
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Camera",
        meta=(ClampMin="256.0"))
    float OrthoWidth = 2048.f;

    // ── 스크롤 설정 ──────────────────────────────────────────────────────────────

    // +X 방향 자동 이동 속도 (units/s). 0 = 정지
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scroll")
    float ScrollSpeed = 0.f;

    // GetPlayableBounds() 적용 마진. X=좌우, Y=상하 (월드 단위)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scroll")
    FVector2D PlayableInset = FVector2D(64.f, 64.f);

    /** 스크롤 속도 변경. 연출·보스 페이즈 전환 시 호출 */
    UFUNCTION(BlueprintCallable, Category="Scroll")
    void SetScrollSpeed(float InSpeed);

    /** 카메라(및 스크롤) 일시 정지 토글 */
    UFUNCTION(BlueprintCallable, Category="Scroll")
    void SetPaused(bool bInPaused);

    /**
     * 현재 카메라 프러스텀에서 XY 이동 가능 영역 반환.
     * FBox2D.X = 월드 X(앞뒤), FBox2D.Y = 월드 Y(좌우).
     * OrthoWidth + 뷰포트 종횡비(없으면 16:9 폴백) + PlayableInset 적용.
     */
    UFUNCTION(BlueprintPure, Category="Scroll")
    FBox2D GetPlayableBounds() const;

private:
    // SetPaused로만 변경
    UPROPERTY(BlueprintReadOnly, Category="Scroll", meta=(AllowPrivateAccess="true"))
    bool bPaused = false;
};
