#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Camera/StageCameraActor.h"
#include "Core/DualFireDataTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDualFireEnemyRenderHeightTest,
	"DualFire.Enemy.RenderHeight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDualFireEnemyRenderHeightTest::RunTest(const FString& Parameters)
{
	const FEnemyRow DefaultRow;
	TestTrue(TEXT("enemy render-height ratio defaults to zero"),
		FMath::IsNearlyZero(DefaultRow.RenderHeightRatio));

	TestTrue(TEXT("playfield world height uses orthographic width and aspect ratio"),
		FMath::IsNearlyEqual(AStageCameraActor::CalculatePlayableWorldHeight(1600.0f, 4.0f / 3.0f), 1200.0f));
	TestTrue(TEXT("invalid aspect ratio remains finite"),
		FMath::IsFinite(AStageCameraActor::CalculatePlayableWorldHeight(1600.0f, 0.0f)));

	const FVector VerticalOffset = AStageCameraActor::CalculateRenderHeightOffset(
		0.25f, 1600.0f, 4.0f / 3.0f, FRotator(-90.0f, 0.0f, 0.0f));
	TestTrue(TEXT("vertical camera raises the visual in world height"),
		VerticalOffset.Equals(FVector(0.0f, 0.0f, 300.0f), KINDA_SMALL_NUMBER));

	const FVector NearVerticalOffset = AStageCameraActor::CalculateRenderHeightOffset(
		0.25f, 1600.0f, 4.0f / 3.0f, FRotator(-89.0f, 0.0f, 0.0f));
	TestTrue(TEXT("near-vertical camera keeps the requested world height"),
		FMath::IsNearlyEqual(NearVerticalOffset.Z, 300.0f));
	TestTrue(TEXT("near-vertical camera adds projection compensation"), NearVerticalOffset.X < 0.0f);
	TestTrue(TEXT("projection compensation stays on the camera view ray"),
		NearVerticalOffset.GetSafeNormal().Equals(-FRotator(-89.0f, 0.0f, 0.0f).Vector(), KINDA_SMALL_NUMBER));
	TestTrue(TEXT("negative render-height ratio does not move the visual"),
		AStageCameraActor::CalculateRenderHeightOffset(-1.0f, 1600.0f, 4.0f / 3.0f, FRotator(-90.0f, 0.0f, 0.0f)).IsNearlyZero());

	return true;
}

#endif
