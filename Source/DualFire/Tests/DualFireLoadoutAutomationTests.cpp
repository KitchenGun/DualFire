#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/LoadoutDataLibrary.h"
#include "Engine/DataTable.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDualFireLoadoutRowLookupTest,
	"DualFire.Loadout.RowLookup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDualFireLoadoutRowLookupTest::RunTest(const FString& Parameters)
{
	UDataTable* Table = NewObject<UDataTable>();
	Table->RowStruct = FWeaponRow::StaticStruct();

	FWeaponRow RowNameFallback;
	Table->AddRow(TEXT("ROW_NAME"), RowNameFallback);

	FWeaponRow ExplicitID;
	ExplicitID.WeaponID = TEXT("WEAPON_ID");
	Table->AddRow(TEXT("DIFFERENT_ROW_NAME"), ExplicitID);

	FWeaponRow Found;
	TestTrue(TEXT("row name lookup succeeds"),
		ULoadoutDataLibrary::FindWeaponRow(Table, TEXT("ROW_NAME"), Found));
	TestEqual(TEXT("missing ID falls back to row name"), Found.WeaponID, FName(TEXT("ROW_NAME")));

	TestTrue(TEXT("explicit ID lookup succeeds"),
		ULoadoutDataLibrary::FindWeaponRow(Table, TEXT("WEAPON_ID"), Found));
	TestEqual(TEXT("explicit ID is preserved"), Found.WeaponID, FName(TEXT("WEAPON_ID")));
	return true;
}

#endif
