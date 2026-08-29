#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "GameInstance/DualFireGameUserSettings.h"
#include "Player/DualFireMovementComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDualFireSlowInputSettingsTest,
	"DualFire.Player.SlowInput.Settings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDualFireSlowInputSettingsTest::RunTest(const FString& Parameters)
{
	const UDualFireGameUserSettings* Settings = NewObject<UDualFireGameUserSettings>();
	TestEqual(TEXT("slow input defaults to hold"), Settings->GetSlowInputMode(), ESlowInputMode::Hold);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDualFireSlowMovementSpeedTest,
	"DualFire.Player.SlowInput.Speed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDualFireSlowMovementSpeedTest::RunTest(const FString& Parameters)
{
	UDualFireMovementComponent* Movement = NewObject<UDualFireMovementComponent>();
	Movement->MoveSpeed = 300.0f;
	Movement->SlowSpeedRatio = 0.5f;
	TestEqual(TEXT("normal movement keeps full speed"), Movement->GetEffectiveMoveSpeed(), 300.0f);
	Movement->SetSlowMovementActive(true);
	TestEqual(TEXT("slow movement applies configured ratio"), Movement->GetEffectiveMoveSpeed(), 150.0f);
	Movement->ToggleSlowMovement();
	TestFalse(TEXT("toggle leaves slow movement"), Movement->IsSlowMovementActive());
	Movement->ToggleSlowMovement();
	TestTrue(TEXT("toggle re-enters slow movement"), Movement->IsSlowMovementActive());
	Movement->ResetSlowMovement();
	TestFalse(TEXT("reset returns to normal movement"), Movement->IsSlowMovementActive());
	return true;
}

#endif
