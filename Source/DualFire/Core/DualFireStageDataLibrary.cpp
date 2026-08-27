// Copyright DualFire. All Rights Reserved.

#include "Core/DualFireStageDataLibrary.h"

#include "Curves/CurveFloat.h"

bool UDualFireStageDataLibrary::SetNormalizedCurveKeys(
	UCurveFloat* Curve,
	const TArray<FVector2D>& TimeValueKeys)
{
	if (!IsValid(Curve) || TimeValueKeys.Num() == 0)
	{
		return false;
	}
	for (int32 Index = 0; Index < TimeValueKeys.Num(); ++Index)
	{
		const FVector2D& Key = TimeValueKeys[Index];
		if (Key.X < 0.0f || Key.Y < 0.0f || Key.Y > 1.0f ||
			(Index > 0 && Key.X <= TimeValueKeys[Index - 1].X))
		{
			return false;
		}
	}

	Curve->Modify();
	Curve->FloatCurve.Reset();
	for (int32 Index = 0; Index < TimeValueKeys.Num(); ++Index)
	{
		const FVector2D& Key = TimeValueKeys[Index];
		const FKeyHandle Handle = Curve->FloatCurve.AddKey(Key.X, Key.Y);
		const bool bHoldUntilNext = Index + 1 < TimeValueKeys.Num() &&
			FMath::IsNearlyEqual(Key.Y, TimeValueKeys[Index + 1].Y);
		Curve->FloatCurve.SetKeyInterpMode(
			Handle, bHoldUntilNext ? RCIM_Constant : RCIM_Cubic);
		if (!bHoldUntilNext)
		{
			Curve->FloatCurve.SetKeyTangentMode(Handle, RCTM_Auto);
		}
	}
	Curve->FloatCurve.AutoSetTangents();
	Curve->MarkPackageDirty();
	return true;
}
