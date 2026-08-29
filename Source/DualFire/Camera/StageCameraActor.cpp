// Copyright DualFire. All Rights Reserved.

#include "StageCameraActor.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"

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
    ApplyCameraSettings();
}

void AStageCameraActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    ApplyCameraSettings();
}

void AStageCameraActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    const FVector ScrollVelocity = GetScrollVelocity();
    if (ScrollVelocity.IsNearlyZero())
    {
        return;
    }

    // +X 방향 자동 스크롤
    SetActorLocation(GetActorLocation() + ScrollVelocity * DeltaTime);
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

FVector AStageCameraActor::GetScrollVelocity() const
{
    return (bPaused || FMath::IsNearlyZero(ScrollSpeed))
        ? FVector::ZeroVector
        : FVector(ScrollSpeed, 0.f, 0.f);
}

// ── 이동 가능 영역 계산 ───────────────────────────────────────────────────────────

FBox2D AStageCameraActor::GetPlayableBounds() const
{
    // OrthoWidth  → 월드 Y 축 범위 (좌우, 화면 수평)
    // OrthoHeight = OrthoWidth / AspectRatio → 월드 X 축 범위 (앞뒤, 화면 수직)
    const float HalfW = OrthoWidth * 0.5f;
    const float HalfH = CalculatePlayableWorldHeight(OrthoWidth, AspectRatio) * 0.5f;
    const FVector Loc = GetActorLocation();

    // FBox2D.X = 월드 X(앞뒤), FBox2D.Y = 월드 Y(좌우)
    return FBox2D(
        FVector2D(Loc.X - HalfH + PlayableInset.Y, Loc.Y - HalfW + PlayableInset.X),
        FVector2D(Loc.X + HalfH - PlayableInset.Y, Loc.Y + HalfW - PlayableInset.X)
    );
}

float AStageCameraActor::CalculatePlayableWorldHeight(
    const float InOrthoWidth,
    const float InAspectRatio)
{
    return FMath::Max(InOrthoWidth, 0.0f) / FMath::Max(InAspectRatio, SMALL_NUMBER);
}

FVector AStageCameraActor::CalculateRenderHeightOffset(
    const float InRenderHeightRatio,
    const float InOrthoWidth,
    const float InAspectRatio,
    const FRotator& InCameraRotation)
{
    const float RenderHeight = FMath::Max(InRenderHeightRatio, 0.0f) *
        CalculatePlayableWorldHeight(InOrthoWidth, InAspectRatio);
    const FVector CameraForward = InCameraRotation.Vector().GetSafeNormal();
    if (FMath::Abs(CameraForward.Z) <= KINDA_SMALL_NUMBER)
    {
        return FVector(0.0f, 0.0f, RenderHeight);
    }

    return CameraForward * (RenderHeight / CameraForward.Z);
}

FVector AStageCameraActor::GetRenderHeightOffset(const float InRenderHeightRatio) const
{
    const FRotator CameraRotation = IsValid(CameraComp)
        ? CameraComp->GetComponentRotation()
        : GetActorRotation();
    return CalculateRenderHeightOffset(InRenderHeightRatio, OrthoWidth, AspectRatio, CameraRotation);
}

void AStageCameraActor::ApplyCameraSettings()
{
    if (!CameraComp)
    {
        return;
    }

    CameraComp->ProjectionMode = ECameraProjectionMode::Orthographic;
    CameraComp->OrthoWidth = OrthoWidth;
    CameraComp->bConstrainAspectRatio = true;
    CameraComp->AspectRatio = FMath::Max(AspectRatio, 0.1f);
}
