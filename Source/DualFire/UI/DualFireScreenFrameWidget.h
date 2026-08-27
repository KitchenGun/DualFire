// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "DualFireScreenFrameWidget.generated.h"

class UImage;
class UTexture2D;

/** 화면별 콘텐츠 밖에서 공용 배경과 상단 Accent를 제공한다. */
UCLASS(BlueprintType, Blueprintable)
class DUALFIRE_API UDualFireScreenFrameWidget : public UCommonUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativePreConstruct() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Screen Frame", meta = (ExposeOnSpawn))
	TObjectPtr<UTexture2D> BackgroundTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Screen Frame", meta = (ExposeOnSpawn))
	bool bShowTopAccent = true;

	UPROPERTY(BlueprintReadOnly, Category = "Screen Frame|Widgets", meta = (BindWidget))
	TObjectPtr<UImage> BackgroundImage;

	UPROPERTY(BlueprintReadOnly, Category = "Screen Frame|Widgets", meta = (BindWidget))
	TObjectPtr<UWidget> TopAccentContainer;
};
