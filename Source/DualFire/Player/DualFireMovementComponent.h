// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PawnMovementComponent.h"
#include "DualFireMovementComponent.generated.h"

class AStageCameraActor;

/**
 * 2D 스크롤 슈팅용 플레이어 이동 컴포넌트.
 * 이동 평면: XY (X=앞뒤, Y=좌우), Z 고정.
 * Enhanced Input → AddMovementInput → ConsumeInputVector 경로로 입력 수신.
 * 입력 방향에 MoveSpeed를 적용하고 StageCamera 경계 안으로 클램핑한다.
 */
UCLASS(ClassGroup=Movement, meta=(BlueprintSpawnableComponent))
class DUALFIRE_API UDualFireMovementComponent : public UPawnMovementComponent
{
    GENERATED_BODY()

public:
    UDualFireMovementComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    // 최고 이동 속도 (units/s)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement")
    float MoveSpeed = 300.f;

private:
    /**
     * StageCameraActor::GetPlayableBounds() 기반 XY 클램핑.
     * StageCamera가 아직 없으면 InLocation을 그대로 반환 + 경고 로그.
     */
    FVector ClampToScreenBounds(const FVector& InLocation, const AStageCameraActor* Camera) const;

    AStageCameraActor* GetStageCamera() const;
};
