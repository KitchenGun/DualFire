// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/** Pure world-space layout rules for the fixed 3:4 playfield. */
namespace DualFireViewportLayout
{
	inline constexpr float RenderHeight = 1200.0f;
	inline constexpr float PlayfieldWidth = 900.0f;
	inline constexpr float DefaultViewportAspectRatio = 4.0f / 3.0f;

	struct FLayout
	{
		float ViewportAspectRatio = DefaultViewportAspectRatio;
		float FullViewWidth = RenderHeight * DefaultViewportAspectRatio;
		float SideDimWidth = (FullViewWidth - PlayfieldWidth) * 0.5f;
	};

	inline FLayout MakeLayout(const float InViewportAspectRatio)
	{
		FLayout Result;
		Result.ViewportAspectRatio = FMath::Max(InViewportAspectRatio, KINDA_SMALL_NUMBER);
		Result.FullViewWidth = RenderHeight * Result.ViewportAspectRatio;
		Result.SideDimWidth = FMath::Max(0.0f, (Result.FullViewWidth - PlayfieldWidth) * 0.5f);
		return Result;
	}

	inline FBox2D MakePlayableBounds(const FVector2D& Center, const FVector2D& Inset)
	{
		const float HorizontalInset = FMath::Clamp(Inset.X, 0.0f, PlayfieldWidth * 0.5f);
		const float VerticalInset = FMath::Clamp(Inset.Y, 0.0f, RenderHeight * 0.5f);
		return FBox2D(
			FVector2D(Center.X - RenderHeight * 0.5f + VerticalInset, Center.Y - PlayfieldWidth * 0.5f + HorizontalInset),
			FVector2D(Center.X + RenderHeight * 0.5f - VerticalInset, Center.Y + PlayfieldWidth * 0.5f - HorizontalInset));
	}

	/** Centered 3:4 HUD rect for supported landscape viewports (aspect >= 0.75). */
	inline FIntRect MakeCenteredPlayfieldScreenRect(const FIntPoint ViewportSize)
	{
		const int32 PlayfieldPixelWidth = FMath::Min(
			ViewportSize.X,
			FMath::RoundToInt(static_cast<float>(ViewportSize.Y) * (PlayfieldWidth / RenderHeight)));
		const int32 Left = (ViewportSize.X - PlayfieldPixelWidth) / 2;
		return FIntRect(Left, 0, Left + PlayfieldPixelWidth, ViewportSize.Y);
	}
}
