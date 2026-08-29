#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Camera/StageCameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/DualFireViewportLayout.h"
#include "UObject/UObjectGlobals.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDualFireViewportLayoutTest,
	"DualFire.Camera.ViewportLayout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDualFireViewportLayoutTest::RunTest(const FString& Parameters)
{
	const struct { FIntPoint ViewportSize; float AspectRatio; float ExpectedFullWidth; FIntRect ExpectedScreenRect; } Cases[] = {
		{ FIntPoint(1600, 1200), 4.0f / 3.0f, 1600.0f, FIntRect(350, 0, 1250, 1200) },
		{ FIntPoint(1920, 1080), 16.0f / 9.0f, 1200.0f * 16.0f / 9.0f, FIntRect(555, 0, 1365, 1080) },
		{ FIntPoint(2520, 1080), 21.0f / 9.0f, 2800.0f, FIntRect(855, 0, 1665, 1080) },
	};
	for (const auto& Case : Cases)
	{
		const DualFireViewportLayout::FLayout Layout = DualFireViewportLayout::MakeLayout(Case.AspectRatio);
		TestTrue(TEXT("full view width follows render height times viewport aspect"), FMath::IsNearlyEqual(Layout.FullViewWidth, Case.ExpectedFullWidth));
		TestTrue(TEXT("central playfield remains 900 units wide"), FMath::IsNearlyEqual(Layout.FullViewWidth - Layout.SideDimWidth * 2.0f, DualFireViewportLayout::PlayfieldWidth));
		const FIntRect ScreenRect = DualFireViewportLayout::MakeCenteredPlayfieldScreenRect(Case.ViewportSize);
		TestEqual(TEXT("HUD playfield rect left edge is centered"), ScreenRect.Min.X, Case.ExpectedScreenRect.Min.X);
		TestEqual(TEXT("HUD playfield rect right edge is centered"), ScreenRect.Max.X, Case.ExpectedScreenRect.Max.X);
		TestEqual(TEXT("HUD playfield rect starts at viewport top"), ScreenRect.Min.Y, Case.ExpectedScreenRect.Min.Y);
		TestEqual(TEXT("HUD playfield rect fills viewport height"), ScreenRect.Max.Y, Case.ExpectedScreenRect.Max.Y);
	}

	const FBox2D Bounds = DualFireViewportLayout::MakePlayableBounds(FVector2D(100.0f, -200.0f), FVector2D(30.0f, 40.0f));
	TestTrue(TEXT("playable bounds retain fixed render height after inset"), FMath::IsNearlyEqual(Bounds.GetSize().X, 1120.0f));
	TestTrue(TEXT("playable bounds retain fixed field width after inset"), FMath::IsNearlyEqual(Bounds.GetSize().Y, 840.0f));
	TestTrue(TEXT("render height is viewport independent"), FMath::IsNearlyEqual(DualFireViewportLayout::RenderHeight * 0.05f, 60.0f));

	AStageCameraActor* Camera = NewObject<AStageCameraActor>(GetTransientPackage());
	Camera->OnConstruction(FTransform::Identity);
	TArray<UStaticMeshComponent*> Overlays;
	Camera->GetViewportOverlayComponents(Overlays);
	TestEqual(TEXT("camera owns two dim planes and two boundary bands"), Overlays.Num(), 4);
	if (Overlays.Num() != 4)
	{
		return false;
	}
	for (const UStaticMeshComponent* Overlay : Overlays)
	{
		TestNotNull(TEXT("viewport overlay component exists"), Overlay);
		if (Overlay)
		{
			TestEqual(TEXT("viewport overlay has no collision"), Overlay->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
			TestFalse(TEXT("viewport overlay does not cast shadows"), Overlay->CastShadow);
		}
	}
	TestEqual(TEXT("dim planes share explicit translucent sort priority"), Overlays[0]->TranslucencySortPriority, Overlays[1]->TranslucencySortPriority);
	TestEqual(TEXT("boundary bands share explicit translucent sort priority"), Overlays[2]->TranslucencySortPriority, Overlays[3]->TranslucencySortPriority);
	TestTrue(TEXT("boundary bands render after dim planes"), Overlays[2]->TranslucencySortPriority > Overlays[0]->TranslucencySortPriority);
	TestTrue(TEXT("dim planes are horizontally symmetric"), FMath::IsNearlyEqual(Overlays[0]->GetRelativeLocation().Y, -Overlays[1]->GetRelativeLocation().Y));
	TestTrue(TEXT("boundary bands are horizontally symmetric"), FMath::IsNearlyEqual(Overlays[2]->GetRelativeLocation().Y, -Overlays[3]->GetRelativeLocation().Y));
	TestTrue(TEXT("dim planes have matching widths"), FMath::IsNearlyEqual(Overlays[0]->GetRelativeScale3D().Y, Overlays[1]->GetRelativeScale3D().Y));
	TestEqual(
		TEXT("boundary bands use the configured 52uu width"),
		static_cast<float>(Overlays[2]->GetRelativeScale3D().Y * 100.0),
		52.0f,
		UE_KINDA_SMALL_NUMBER);

	const UCameraComponent* CameraComponent = Camera->FindComponentByClass<UCameraComponent>();
	TestNotNull(TEXT("stage camera component exists"), CameraComponent);
	if (CameraComponent)
	{
		TestEqual(TEXT("stage camera uses orthographic projection"), CameraComponent->ProjectionMode.GetValue(), ECameraProjectionMode::Orthographic);
		TestFalse(TEXT("stage camera does not constrain viewport aspect"), CameraComponent->bConstrainAspectRatio);
		TestEqual(TEXT("stage camera maintains vertical FOV"), CameraComponent->AspectRatioAxisConstraint.GetValue(), EAspectRatioAxisConstraint::AspectRatio_MaintainYFOV);
		TestTrue(TEXT("stage camera OrthoWidth preserves the fixed 1200uu render height"), FMath::IsNearlyEqual(CameraComponent->OrthoWidth, 1200.0f));
	}
	return true;
}

#endif
