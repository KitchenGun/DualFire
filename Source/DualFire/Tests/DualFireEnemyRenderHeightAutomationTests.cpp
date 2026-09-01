#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Camera/StageCameraActor.h"
#include "Core/DualFireDataTypes.h"
#include "Enemy/EnemyBase.h"
#include "Engine/DataTable.h"
#include "Player/DualFirePlayerPawn.h"
#include "Stage/StageController.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "UObject/UObjectGlobals.h"

#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDualFireEnemyRenderHeightTest,
	"DualFire.Enemy.RenderHeight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDualFireEnemyRenderHeightTest::RunTest(const FString& Parameters)
{
	const FEnemyRow DefaultRow;
	TestTrue(TEXT("enemy render-height ratio defaults to zero"),
		FMath::IsNearlyZero(DefaultRow.RenderHeightRatio));
	const FStageRow DefaultStageRow;
	TestTrue(TEXT("player render-height ratio defaults to five percent"),
		FMath::IsNearlyEqual(DefaultStageRow.PlayerRenderHeightRatio, 0.05f));
	const FWaveRow DefaultWaveRow;
	TestTrue(TEXT("wave render-height override preserves EnemyRow by default"),
		FMath::IsNearlyEqual(DefaultWaveRow.RenderHeightRatioOverride, -1.0f));

	TestTrue(TEXT("stage player render-height ratio accepts zero"),
		AStageController::IsRenderHeightRatioValid(0.0f, 0.0f));
	TestFalse(TEXT("stage player render-height ratio rejects negative values"),
		AStageController::IsRenderHeightRatioValid(-0.01f, 0.0f));
	TestFalse(TEXT("wave render-height override rejects values below sentinel"),
		AStageController::IsRenderHeightRatioValid(-1.01f, -1.0f));
	TestFalse(TEXT("render-height ratio rejects infinity"),
		AStageController::IsRenderHeightRatioValid(std::numeric_limits<float>::infinity(), -1.0f));

	FEnemyRow EnemyRow;
	EnemyRow.RenderHeightRatio = 0.15f;
	FWaveRow WaveRow;
	TestTrue(TEXT("negative override preserves EnemyRow ratio"),
		FMath::IsNearlyEqual(AStageController::ResolveWaveEnemyRow(EnemyRow, WaveRow).RenderHeightRatio, 0.15f));
	WaveRow.RenderHeightRatioOverride = 0.0f;
	TestTrue(TEXT("zero override applies ground render layer"),
		FMath::IsNearlyZero(AStageController::ResolveWaveEnemyRow(EnemyRow, WaveRow).RenderHeightRatio));
	WaveRow.RenderHeightRatioOverride = 0.25f;
	TestTrue(TEXT("positive override applies elevated render layer"),
		FMath::IsNearlyEqual(AStageController::ResolveWaveEnemyRow(EnemyRow, WaveRow).RenderHeightRatio, 0.25f));

	ADualFirePlayerPawn* PlayerPawn = NewObject<ADualFirePlayerPawn>(GetTransientPackage());
	TestNotNull(TEXT("player pawn creates for render-height state test"), PlayerPawn);
	if (PlayerPawn)
	{
		const FVector InitialRootLocation = PlayerPawn->GetRootComponent()->GetRelativeLocation();
		PlayerPawn->SetRenderHeightRatio(0.2f);
		TestTrue(TEXT("player stores stage render-height ratio"),
			FMath::IsNearlyEqual(PlayerPawn->GetRenderHeightRatio(), 0.2f));
		TestTrue(TEXT("player render-height ratio does not move the actor root"),
			PlayerPawn->GetRootComponent()->GetRelativeLocation().Equals(InitialRootLocation));
	}

	const FVector VerticalOffset = AStageCameraActor::CalculateRenderHeightOffset(
		0.05f, FRotator(-90.0f, 0.0f, 0.0f));
	TestTrue(TEXT("vertical camera raises the visual in world height"),
		VerticalOffset.Equals(FVector(0.0f, 0.0f, 60.0f), KINDA_SMALL_NUMBER));

	const FVector NearVerticalOffset = AStageCameraActor::CalculateRenderHeightOffset(
		0.05f, FRotator(-89.0f, 0.0f, 0.0f));
	TestTrue(TEXT("near-vertical camera keeps the requested world height"),
		FMath::IsNearlyEqual(NearVerticalOffset.Z, 60.0f));
	TestTrue(TEXT("near-vertical camera adds projection compensation"), NearVerticalOffset.X < 0.0f);
	TestTrue(TEXT("projection compensation stays on the camera view ray"),
		NearVerticalOffset.GetSafeNormal().Equals(-FRotator(-89.0f, 0.0f, 0.0f).Vector(), KINDA_SMALL_NUMBER));
	TestTrue(TEXT("negative render-height ratio does not move the visual"),
		AStageCameraActor::CalculateRenderHeightOffset(-1.0f, FRotator(-90.0f, 0.0f, 0.0f)).IsNearlyZero());

	const UDataTable* EnemyTable = LoadObject<UDataTable>(
		nullptr, TEXT("/Game/Data/Stage/DT_Enemies.DT_Enemies"));
	TestNotNull(TEXT("DT_Enemies loads for shadow reset test"), EnemyTable);
	if (!EnemyTable)
	{
		return false;
	}
	TArray<FEnemyRow*> EnemyRows;
	EnemyTable->GetAllRows<FEnemyRow>(TEXT("DualFire.Enemy.RenderHeight"), EnemyRows);
	FEnemyRow* const* MeshRowPointer = EnemyRows.FindByPredicate([](const FEnemyRow* Row)
	{
		return Row && !Row->Mesh.IsNull();
	});
	const FEnemyRow* MeshRow = MeshRowPointer ? *MeshRowPointer : nullptr;
	TestNotNull(TEXT("DT_Enemies contains a mesh-backed row"), MeshRow);
	if (!MeshRow)
	{
		return false;
	}

	AEnemyBase* Enemy = NewObject<AEnemyBase>(GetTransientPackage());
	TestNotNull(TEXT("enemy creates for shadow reset test"), Enemy);
	if (!Enemy)
	{
		return false;
	}
	USkeletalMeshComponent* Mesh = Enemy->FindComponentByClass<USkeletalMeshComponent>();
	TestNotNull(TEXT("enemy mesh component exists"), Mesh);
	if (!Mesh)
	{
		return false;
	}
	const FVector InitialActorLocation = Enemy->GetActorLocation();
	const FVector InitialHitboxLocation = Enemy->GetRootComponent()->GetRelativeLocation();
	FEnemyRow ShadowRow = *MeshRow;
	ShadowRow.Attribute.bGround = true;
	ShadowRow.Attribute.bAir = false;
	TestTrue(TEXT("ground enemy initializes"), Enemy->InitFromEnemyRow(ShadowRow));
	TestFalse(TEXT("ground enemy clears static shadow"), Mesh->CastShadow);
	TestFalse(TEXT("ground enemy clears dynamic shadow"), Mesh->bCastDynamicShadow);

	ShadowRow.Attribute.bAir = true;
	TestTrue(TEXT("mixed enemy reinitializes"), Enemy->InitFromEnemyRow(ShadowRow));
	TestTrue(TEXT("mixed enemy restores static shadow"), Mesh->CastShadow);
	TestTrue(TEXT("mixed enemy restores dynamic shadow"), Mesh->bCastDynamicShadow);

	ShadowRow.Attribute.bAir = false;
	TestTrue(TEXT("ground enemy reinitializes after air enemy"), Enemy->InitFromEnemyRow(ShadowRow));
	TestFalse(TEXT("ground enemy reset clears static shadow"), Mesh->CastShadow);
	TestFalse(TEXT("ground enemy reset clears dynamic shadow"), Mesh->bCastDynamicShadow);
	TestTrue(TEXT("render layer initialization does not move enemy actor"),
		Enemy->GetActorLocation().Equals(InitialActorLocation));
	TestTrue(TEXT("render layer initialization does not move enemy hitbox root"),
		Enemy->GetRootComponent()->GetRelativeLocation().Equals(InitialHitboxLocation));

	return true;
}

#endif
