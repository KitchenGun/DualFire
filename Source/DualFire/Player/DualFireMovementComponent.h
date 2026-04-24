// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PawnMovementComponent.h"
#include "DualFireMovementComponent.generated.h"

/**
 * 2D 스크롤 슈팅용 플레이어 이동 컴포넌트.
 * 이동 평면: XZ (X=좌우, Z=상하), Y 고정.
 * Enhanced Input → AddMovementInput → ConsumeInputVector 경로로 입력 수신.
 * 가속도 기반 선형 보간(VInterpConstantTo), 뷰포트 UV 마진 화면 경계 클램핑.
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
    float MoveSpeed = 900.f;

    // 가속도 (units/s²). VInterpConstantTo의 초당 속도 변화량
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement")
    float Acceleration = 8000.f;

    /** 슈퍼웨폰 둔화/가속용 배율. 기본값 1.0 (스텁) */
    UFUNCTION(BlueprintCallable, Category="Movement")
    void SetSpeedMultiplier(float InMultiplier);

    /** 스턴/기술 발동 중 이동 락. true 시 즉시 정지 (스텁) */
    UFUNCTION(BlueprintCallable, Category="Movement")
    void SetMovementLocked(bool bLocked);

private:
    // GC 불필요(원시 타입)이나 BP 디버그 노출용
    UPROPERTY(BlueprintReadOnly, Category="Movement", meta=(AllowPrivateAccess="true"))
    float SpeedMultiplier = 1.f;

    UPROPERTY(BlueprintReadOnly, Category="Movement", meta=(AllowPrivateAccess="true"))
    bool bMovementLocked = false;

    /**
     * StageCameraActor::GetPlayableBounds() 기반 XZ 클램핑.
     * StageCamera가 아직 없으면 InLocation을 그대로 반환 + 경고 로그.
     */
    FVector ClampToScreenBounds(const FVector& InLocation) const;
};
