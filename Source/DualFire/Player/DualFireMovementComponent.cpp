// Copyright DualFire. All Rights Reserved.

#include "DualFireMovementComponent.h"

#include "DualFire.h"
#include "GameModes/DualFireGameModeBase.h"
#include "Camera/StageCameraActor.h"

#include "GameFramework/Pawn.h"

UDualFireMovementComponent::UDualFireMovementComponent()
{
    PrimaryComponentTick.bCanEverTick = true;

    // 중력/물리 없음. Z 축 방향 평면(XY)으로 이동 제한
    bConstrainToPlane = true;
    SetPlaneConstraintAxisSetting(EPlaneConstraintAxisSetting::Z);
}

void UDualFireMovementComponent::BeginPlay()
{
    Super::BeginPlay();

    // 폰의 초기 Y 위치를 평면 원점으로 등록해 Y 고정 보장
    if (UpdatedComponent)
    {
        SetPlaneConstraintOrigin(UpdatedComponent->GetComponentLocation());
    }
}

void UDualFireMovementComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // dedicated server 스킵, hidden pawn 스킵, UpdatedComponent 없음 스킵
    if (ShouldSkipUpdate(DeltaTime))
    {
        return;
    }

    // AddMovementInput으로 누적된 입력 소비 (Enhanced Input → Pawn → 여기)
    FVector InputVector = ConsumeInputVector();

    if (bMovementLocked)
    {
        InputVector = FVector::ZeroVector;
    }

    // XY 평면 정규화 후 속도 적용 (대각선 포함 모든 방향 동일 속도 보장)
    const FVector InputDir = FVector(InputVector.X, InputVector.Y, 0.f).GetSafeNormal();
    const FVector TargetVelocity = InputDir * MoveSpeed * SpeedMultiplier;

    Velocity = TargetVelocity;
    Velocity.Z = 0.f; // bConstrainToPlane 이중 보장

    AStageCameraActor* Camera = GetStageCamera();
    const FVector ScrollDelta = IsValid(Camera) ? Camera->GetScrollVelocity() * DeltaTime : FVector::ZeroVector;
    FVector Delta = (Velocity * DeltaTime) + ScrollDelta;

    if (Delta.IsNearlyZero(0.01f))
    {
        Velocity = FVector::ZeroVector;
        UpdateComponentVelocity();
        return;
    }

    // 이동 후 예상 위치에 화면 경계 클램핑 적용
    const FVector PrevLocation = UpdatedComponent->GetComponentLocation();
    const FVector DesiredLocation = PrevLocation + Delta;
    const FVector ClampedDesired = ClampToScreenBounds(DesiredLocation, Camera);

    Delta = ClampedDesired - PrevLocation;
    Delta.Z = 0.f;

    // 충돌 포함 이동. bSweep=true → 충돌 감지
    FHitResult Hit(1.f);
    SafeMoveUpdatedComponent(Delta, UpdatedComponent->GetComponentQuat(), true, Hit);

    if (Hit.IsValidBlockingHit())
    {
        // 충돌면을 따라 슬라이딩 (남은 거리 비율: 1 - Hit.Time)
        SlideAlongSurface(Delta, 1.f - Hit.Time, Hit.Normal, Hit, true);
    }

    // 실제 이동 거리로 Velocity 재계산 → 클램핑·충돌 결과 반영
    const FVector ActualDelta = UpdatedComponent->GetComponentLocation() - PrevLocation;
    Velocity = (DeltaTime > SMALL_NUMBER) ? (ActualDelta / DeltaTime) : FVector::ZeroVector;

    UpdateComponentVelocity();
}

// ── 확장 슬롯 (스텁) ─────────────────────────────────────────────────────────

void UDualFireMovementComponent::SetSpeedMultiplier(float InMultiplier)
{
    SpeedMultiplier = FMath::Max(0.f, InMultiplier);
}

void UDualFireMovementComponent::SetMovementLocked(bool bLocked)
{
    bMovementLocked = bLocked;
    if (bMovementLocked)
    {
        Velocity = FVector::ZeroVector;
    }
}

// ── 화면 경계 클램핑 ──────────────────────────────────────────────────────────

FVector UDualFireMovementComponent::ClampToScreenBounds(
    const FVector& InLocation,
    const AStageCameraActor* Camera) const
{
    if (!IsValid(Camera))
    {
        return InLocation;
    }

    // FBox2D.X = 월드 X(앞뒤), FBox2D.Y = 월드 Y(좌우)
    const FBox2D Bounds = Camera->GetPlayableBounds();
    return FVector(
        FMath::Clamp(InLocation.X, Bounds.Min.X, Bounds.Max.X),
        FMath::Clamp(InLocation.Y, Bounds.Min.Y, Bounds.Max.Y),
        InLocation.Z
    );
}

AStageCameraActor* UDualFireMovementComponent::GetStageCamera() const
{
    // GameMode → StageCamera 경로로 이동 가능 영역 취득
    ADualFireGameModeBase* GM =
        GetWorld() ? GetWorld()->GetAuthGameMode<ADualFireGameModeBase>() : nullptr;

    if (!IsValid(GM))
    {
        UE_LOG(LogDualFire, Warning,
            TEXT("UDualFireMovementComponent: DualFireGameModeBase 없음 — 스크롤/클램핑 스킵"));
        return nullptr;
    }

    AStageCameraActor* Camera = GM->GetStageCamera();
    if (!IsValid(Camera))
    {
        UE_LOG(LogDualFire, Warning,
            TEXT("UDualFireMovementComponent: StageCamera nullptr — 스크롤/클램핑 스킵"));
        return nullptr;
    }

    return Camera;
}
