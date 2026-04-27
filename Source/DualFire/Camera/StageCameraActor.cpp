// Copyright DualFire. All Rights Reserved.

#include "StageCameraActor.h"

#include "DualFire.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/GameViewportClient.h"

AStageCameraActor::AStageCameraActor()
{
    PrimaryActorTick.bCanEverTick = true;

    // ViewTarget 지정 시 이 액터의 CameraComp를 자동 탐색
    bFindCameraComponentWhenViewTarget = true;

    // ── SceneRoot ──────────────────────────────────────────────────────────────
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    // ── CameraComp ─────────────────────────────────────────────────────────────
    // +Z 위치에서 -Z 방향(XY 게임 평면)을 내려다보는 직교 카메라. Pitch=-90 → 수직 하향
    CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
    CameraComp->SetupAttachment(SceneRoot);
    CameraComp->SetRelativeLocation(FVector(0.f, 0.f, 1500.f));
    CameraComp->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
    CameraComp->ProjectionMode = ECameraProjectionMode::Orthographic;
    CameraComp->OrthoWidth = OrthoWidth;
}

void AStageCameraActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bPaused || FMath::IsNearlyZero(ScrollSpeed))
    {
        return;
    }

    // +X 방향 자동 스크롤
    SetActorLocation(GetActorLocation() + FVector(ScrollSpeed * DeltaTime, 0.f, 0.f));
}

// ── 제어 함수 ──────────────────────────────────────────────────────────────────

void AStageCameraActor::SetScrollSpeed(float InSpeed)
{
    ScrollSpeed = InSpeed;
}

void AStageCameraActor::SetPaused(bool bInPaused)
{
    bPaused = bInPaused;
}

// ── 이동 가능 영역 계산 ───────────────────────────────────────────────────────────

FBox2D AStageCameraActor::GetPlayableBounds() const
{
    // ── 뷰포트 종횡비 계산 ───────────────────────────────────────────────────────
    // GEngine / GameViewport가 없으면 16:9 폴백
    float AspectRatio = 16.f / 9.f;

    if (GEngine && GEngine->GameViewport)
    {
        FVector2D ViewportSize;
        GEngine->GameViewport->GetViewportSize(ViewportSize);
        if (!ViewportSize.IsNearlyZero() && ViewportSize.Y > SMALL_NUMBER)
        {
            AspectRatio = ViewportSize.X / ViewportSize.Y;
        }
    }

    // OrthoWidth  → 월드 Y 축 범위 (좌우, 화면 수평)
    // OrthoHeight = OrthoWidth / AspectRatio → 월드 X 축 범위 (앞뒤, 화면 수직)
    const float HalfW = OrthoWidth * 0.5f;
    const float HalfH = (OrthoWidth / AspectRatio) * 0.5f;
    const FVector Loc = GetActorLocation();

    // FBox2D.X = 월드 X(앞뒤), FBox2D.Y = 월드 Y(좌우)
    return FBox2D(
        FVector2D(Loc.X - HalfH + PlayableInset.Y, Loc.Y - HalfW + PlayableInset.X),
        FVector2D(Loc.X + HalfH - PlayableInset.Y, Loc.Y + HalfW - PlayableInset.X)
    );
}
