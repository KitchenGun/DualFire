#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "UI/DualFireHangarTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDualFireHangarAttributePresentationTest,
	"DualFire.UI.Hangar.AttributePresentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDualFireHangarAttributePresentationTest::RunTest(const FString& Parameters)
{
	FDualFireHangarItemViewData Item;

	Item.SetTargetAttributePresentation({ EDualFireAttribute::Ground });
	TestTrue(TEXT("ground presentation is available"), Item.bHasTargetAttributePresentation);
	TestEqual(TEXT("ground uses the expected label"), Item.TargetAttributeLabel.ToString(), FString(TEXT("GROUND")));
	TestEqual(TEXT("ground uses orange"), Item.TargetAttributeColor, FLinearColor(1.0f, 0.5f, 0.0f));

	Item.SetTargetAttributePresentation({ EDualFireAttribute::Air });
	TestEqual(TEXT("air uses the expected label"), Item.TargetAttributeLabel.ToString(), FString(TEXT("AIR")));
	TestEqual(TEXT("air uses cyan"), Item.TargetAttributeColor, FLinearColor(0.0f, 0.8f, 1.0f));

	Item.SetTargetAttributePresentation({ EDualFireAttribute::Ground, EDualFireAttribute::Air });
	TestEqual(TEXT("dual-target uses the expected label"), Item.TargetAttributeLabel.ToString(), FString(TEXT("GROUND / AIR")));
	TestEqual(TEXT("dual-target uses white"), Item.TargetAttributeColor, FLinearColor::White);

	Item.SetTargetAttributePresentation({});
	TestFalse(TEXT("empty attributes do not show a presentation"), Item.bHasTargetAttributePresentation);
	return true;
}

#endif
