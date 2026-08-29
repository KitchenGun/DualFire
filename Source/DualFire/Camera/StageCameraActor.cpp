// Copyright DualFire. All Rights Reserved.

#include "StageCameraActor.h"

#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Core/DualFireViewportLayout.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

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

    LeftDimPlane = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftDimPlane"));
    RightDimPlane = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightDimPlane"));
    LeftBoundaryBand = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftBoundaryBand"));
    RightBoundaryBand = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightBoundaryBand"));

    ConfigureOverlayComponent(LeftDimPlane, 10);
    ConfigureOverlayComponent(RightDimPlane, 10);
    ConfigureOverlayComponent(LeftBoundaryBand, 20);
    ConfigureOverlayComponent(RightBoundaryBand, 20);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMeshFinder(TEXT("/Engine/BasicShapes/Plane.Plane"));
    if (PlaneMeshFinder.Succeeded())
    {
        LeftDimPlane->SetStaticMesh(PlaneMeshFinder.Object);
        RightDimPlane->SetStaticMesh(PlaneMeshFinder.Object);
        LeftBoundaryBand->SetStaticMesh(PlaneMeshFinder.Object);
        RightBoundaryBand->SetStaticMesh(PlaneMeshFinder.Object);
    }
}

void AStageCameraActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    ApplyCameraSettings();
    RefreshViewportLayout(true);
}

void AStageCameraActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    RefreshViewportLayout(false);

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
    const FVector Loc = GetActorLocation();
    return DualFireViewportLayout::MakePlayableBounds(FVector2D(Loc.X, Loc.Y), PlayableInset);
}

FVector AStageCameraActor::CalculateRenderHeightOffset(
    const float InRenderHeightRatio,
    const FRotator& InCameraRotation)
{
    const float RenderHeight = FMath::Max(InRenderHeightRatio, 0.0f) * DualFireViewportLayout::RenderHeight;
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
    return CalculateRenderHeightOffset(InRenderHeightRatio, CameraRotation);
}

void AStageCameraActor::GetViewportOverlayComponents(TArray<UStaticMeshComponent*>& OutComponents) const
{
    OutComponents.Reset(4);
    OutComponents.Add(LeftDimPlane.Get());
    OutComponents.Add(RightDimPlane.Get());
    OutComponents.Add(LeftBoundaryBand.Get());
    OutComponents.Add(RightBoundaryBand.Get());
}

void AStageCameraActor::ApplyCameraSettings()
{
    if (!CameraComp)
    {
        return;
    }

    CameraComp->ProjectionMode = ECameraProjectionMode::Orthographic;
    CameraComp->bConstrainAspectRatio = false;
    CameraComp->AspectRatioAxisConstraint = EAspectRatioAxisConstraint::AspectRatio_MaintainYFOV;
    CameraComp->bOverrideAspectRatioAxisConstraint = true;
}

void AStageCameraActor::RefreshViewportLayout(const bool bForceRefresh)
{
    const float ViewportAspectRatio = ResolveViewportAspectRatio();
    if (!bForceRefresh && FMath::IsNearlyEqual(CurrentViewportAspectRatio, ViewportAspectRatio))
    {
        return;
    }

    CurrentViewportAspectRatio = ViewportAspectRatio;
    const DualFireViewportLayout::FLayout Layout = DualFireViewportLayout::MakeLayout(ViewportAspectRatio);
    if (CameraComp)
    {
        CameraComp->OrthoWidth = DualFireViewportLayout::RenderHeight;
    }

    const float DimCenterY = DualFireViewportLayout::PlayfieldWidth * 0.5f + Layout.SideDimWidth * 0.5f;
    const float BoundaryCenterY = DualFireViewportLayout::PlayfieldWidth * 0.5f;
    const FVector DimScale(DualFireViewportLayout::RenderHeight / 100.0f, Layout.SideDimWidth / 100.0f, 1.0f);
    const float EffectiveBandWidth = FMath::Min(BoundaryBandWidth, Layout.SideDimWidth);
    const FVector BandScale(DualFireViewportLayout::RenderHeight / 100.0f, EffectiveBandWidth / 100.0f, 1.0f);

    LeftDimPlane->SetRelativeLocation(FVector(0.0f, -DimCenterY, 0.0f));
    RightDimPlane->SetRelativeLocation(FVector(0.0f, DimCenterY, 0.0f));
    LeftDimPlane->SetRelativeScale3D(DimScale);
    RightDimPlane->SetRelativeScale3D(DimScale);
    LeftBoundaryBand->SetRelativeLocation(FVector(0.0f, -BoundaryCenterY, 0.0f));
    RightBoundaryBand->SetRelativeLocation(FVector(0.0f, BoundaryCenterY, 0.0f));
    LeftBoundaryBand->SetRelativeScale3D(BandScale);
    RightBoundaryBand->SetRelativeScale3D(BandScale);
    RefreshMaterialInstances(EffectiveBandWidth);

    const bool bHasSideRegion = Layout.SideDimWidth > KINDA_SMALL_NUMBER;
    LeftDimPlane->SetVisibility(bHasSideRegion && IsValid(DimMaterialInstance));
    RightDimPlane->SetVisibility(bHasSideRegion && IsValid(DimMaterialInstance));
    LeftBoundaryBand->SetVisibility(bHasSideRegion && EffectiveBandWidth > KINDA_SMALL_NUMBER && IsValid(BoundaryMaterialInstance));
    RightBoundaryBand->SetVisibility(bHasSideRegion && EffectiveBandWidth > KINDA_SMALL_NUMBER && IsValid(BoundaryMaterialInstance));
}

float AStageCameraActor::ResolveViewportAspectRatio() const
{
    if (GEngine && GEngine->GameViewport && GEngine->GameViewport->Viewport)
    {
        const FIntPoint Size = GEngine->GameViewport->Viewport->GetSizeXY();
        if (Size.X > 0 && Size.Y > 0)
        {
            return static_cast<float>(Size.X) / static_cast<float>(Size.Y);
        }
    }
    return DualFireViewportLayout::DefaultViewportAspectRatio;
}

void AStageCameraActor::RefreshMaterialInstances(const float EffectiveBandWidth)
{
    if (DimMaterial && !DimMaterialInstance)
    {
        DimMaterialInstance = UMaterialInstanceDynamic::Create(DimMaterial, this);
        LeftDimPlane->SetMaterial(0, DimMaterialInstance);
        RightDimPlane->SetMaterial(0, DimMaterialInstance);
    }
    if (BoundaryMaterial && !BoundaryMaterialInstance)
    {
        BoundaryMaterialInstance = UMaterialInstanceDynamic::Create(BoundaryMaterial, this);
        LeftBoundaryBand->SetMaterial(0, BoundaryMaterialInstance);
        RightBoundaryBand->SetMaterial(0, BoundaryMaterialInstance);
    }

    const float NormalizedEdgeWidth = EffectiveBandWidth > KINDA_SMALL_NUMBER
        ? FMath::Clamp(BoundaryEdgeWidth / (2.0f * EffectiveBandWidth), 0.0f, 0.5f)
        : 0.0f;
    const auto ApplyParameters = [this, NormalizedEdgeWidth](
        UMaterialInstanceDynamic* Material,
        const FLinearColor& Tint,
        const float Opacity,
        const float Fade)
    {
        if (!Material)
        {
            return;
        }
        Material->SetVectorParameterValue(TEXT("Tint"), Tint);
        Material->SetVectorParameterValue(TEXT("EdgeColor"), BoundaryEdgeColor);
        Material->SetScalarParameterValue(TEXT("Opacity"), Opacity);
        Material->SetScalarParameterValue(TEXT("Fade"), Fade);
        Material->SetScalarParameterValue(TEXT("Edge"), NormalizedEdgeWidth);
    };
    ApplyParameters(DimMaterialInstance, DimTint, DimOpacity, 0.0f);
    ApplyParameters(BoundaryMaterialInstance, DimTint, DimOpacity, 1.0f);
}

void AStageCameraActor::ConfigureOverlayComponent(UStaticMeshComponent* Component, const int32 SortPriority)
{
    Component->SetupAttachment(SceneRoot);
    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Component->SetGenerateOverlapEvents(false);
    Component->SetCastShadow(false);
    Component->SetVisibility(false);
    Component->TranslucencySortPriority = SortPriority;
}
