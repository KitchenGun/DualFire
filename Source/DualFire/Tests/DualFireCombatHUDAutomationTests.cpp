// Copyright DualFire. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Weapon/Projectile/BaseProjectile.h"
#include "Weapon/WeaponComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDualFireCombatHUDWeaponContractTest,
	"DualFire.UI.CombatHUD.WeaponContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDualFireCombatHUDWeaponContractTest::RunTest(const FString& Parameters)
{
	UWeaponComponent* WeaponComponent = NewObject<UWeaponComponent>();
	FWeaponRow SlotData;
	TestFalse(TEXT("an unresolved special slot has no HUD data"), WeaponComponent->GetResolvedSlotData(ELoadoutSlot::SpecialWeapon1, SlotData));
	TestTrue(TEXT("an unresolved special slot clears HUD data"), SlotData.WeaponID.IsNone());
	TestEqual(TEXT("an unresolved special slot has no cooldown"), WeaponComponent->GetCooldownRemainingPercent(ELoadoutSlot::SpecialWeapon1), 0.0f);

	int32 AppliedNotificationCount = 0;
	WeaponComponent->OnResolvedLoadoutApplied.AddLambda([&AppliedNotificationCount]()
	{
		++AppliedNotificationCount;
	});

	auto MakeWeaponRow = [](const FName WeaponID, const EWeaponCategory Category)
	{
		FWeaponRow Row;
		Row.WeaponID = WeaponID;
		Row.Category = Category;
		Row.ProjectileClass = ABaseProjectile::StaticClass();
		Row.AttributeArray = { EDualFireAttribute::Ground };
		Row.FireRate = 1.0f;
		Row.ProjectileSpeed = 1000.0f;
		return Row;
	};

	const FWeaponRow Primary = MakeWeaponRow(TEXT("PRIMARY_TEST"), EWeaponCategory::Primary);
	const FWeaponRow Special1 = MakeWeaponRow(TEXT("SPECIAL_1_TEST"), EWeaponCategory::Special);
	const FWeaponRow Special2 = MakeWeaponRow(TEXT("SPECIAL_2_TEST"), EWeaponCategory::Special);
	FText ApplyError;
	FName InvalidField;
	TestTrue(TEXT("a resolved loadout is committed"), WeaponComponent->TryApplyResolvedLoadout(Primary, Special1, Special2, ApplyError, InvalidField));
	TestEqual(TEXT("a successful loadout commit notifies the HUD once"), AppliedNotificationCount, 1);
	TestTrue(TEXT("the committed special slot is available to the HUD"), WeaponComponent->GetResolvedSlotData(ELoadoutSlot::SpecialWeapon1, SlotData));
	TestEqual(TEXT("the HUD reads the committed special weapon"), SlotData.WeaponID, Special1.WeaponID);
	return true;
}

#endif
