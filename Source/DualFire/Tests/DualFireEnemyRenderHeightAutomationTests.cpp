#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Camera/StageCameraActor.h"
#include "Core/DualFireDataTypes.h"
#include "Enemy/EnemyBase.h"
#include "Engine/DataTable.h"
#include "Player/DualFirePlayerPawn.h"
#include "Stage/StageController.h"
#include "Components/SceneComponent.h"
#include "PaperFlipbookComponent.h"
#include "PaperFlipbook.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
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
	TestTrue(TEXT("stage air-shadow offset defaults to zero"),
		DefaultStageRow.AirShadowOffsetPerHeight.IsNearlyZero());
	TestTrue(TEXT("stage air-shadow opacity defaults to thirty-five percent"),
		FMath::IsNearlyEqual(DefaultStageRow.AirShadowOpacity, 0.35f));
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
	TestTrue(TEXT("stage air-shadow offset accepts finite XY"),
		AStageController::IsAirShadowOffsetPerHeightValid(FVector2D(0.57f, -0.40f)));
	TestFalse(TEXT("stage air-shadow offset rejects infinity"),
		AStageController::IsAirShadowOffsetPerHeightValid(FVector2D(std::numeric_limits<float>::infinity(), 0.0f)));
	TestTrue(TEXT("stage air-shadow opacity accepts range endpoints"),
		AStageController::IsAirShadowOpacityValid(0.0f) && AStageController::IsAirShadowOpacityValid(1.0f));
	TestFalse(TEXT("stage air-shadow opacity rejects out-of-range values"),
		AStageController::IsAirShadowOpacityValid(-0.01f) || AStageController::IsAirShadowOpacityValid(1.01f));
	TestFalse(TEXT("stage air-shadow opacity rejects infinity"),
		AStageController::IsAirShadowOpacityValid(std::numeric_limits<float>::infinity()));

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
		UPaperFlipbookComponent* PlayerVisual = Cast<UPaperFlipbookComponent>(
			PlayerPawn->GetDefaultSubobjectByName(TEXT("AircraftVisual")));
		UPaperFlipbookComponent* PlayerGroundShadow = Cast<UPaperFlipbookComponent>(
			PlayerPawn->GetDefaultSubobjectByName(TEXT("GroundShadow")));
		TestNotNull(TEXT("player visual is a flipbook component"), PlayerVisual);
		TestNotNull(TEXT("player ground shadow is a flipbook component"), PlayerGroundShadow);
		const FVector InitialRootLocation = PlayerPawn->GetRootComponent()->GetRelativeLocation();
		PlayerPawn->SetRenderHeightRatio(0.2f);
		TestTrue(TEXT("player stores stage render-height ratio"),
			FMath::IsNearlyEqual(PlayerPawn->GetRenderHeightRatio(), 0.2f));
		TestTrue(TEXT("player render-height ratio does not move the actor root"),
			PlayerPawn->GetRootComponent()->GetRelativeLocation().Equals(InitialRootLocation));
		PlayerPawn->SetAirShadowOffsetPerHeight(FVector2D(0.57f, -0.40f));
		PlayerPawn->SetAirShadowOpacity(0.8f);
		if (PlayerGroundShadow)
		{
			TestTrue(TEXT("player silhouette is black with requested opacity"),
				PlayerGroundShadow->GetSpriteColor().Equals(FLinearColor(0.0f, 0.0f, 0.0f, 0.8f)));
			TestTrue(TEXT("player ground shadow uses two-unit clearance"),
				FMath::IsNearlyEqual(PlayerGroundShadow->GetRelativeLocation().Z, 2.0f));
			TestFalse(TEXT("player silhouette cannot collide"),
				PlayerGroundShadow->GetCollisionEnabled() != ECollisionEnabled::NoCollision);
			TestFalse(TEXT("player silhouette does not cast shadows"), PlayerGroundShadow->CastShadow);
		}
		UPaperFlipbook* TestFlipbook = LoadObject<UPaperFlipbook>(
			nullptr, TEXT("/Game/Player/Aircraft/F22/PFB_F22_Bank.PFB_F22_Bank"));
		TestNotNull(TEXT("player bank flipbook loads for silhouette sync"), TestFlipbook);
		if (PlayerVisual && PlayerGroundShadow && TestFlipbook)
		{
			PlayerPawn->ApplyAircraftVisual(TestFlipbook);
			PlayerPawn->SetAircraftBankPose(EAircraftBankPose::Left45);
			TestEqual(TEXT("player silhouette uses the aircraft flipbook"),
				PlayerGroundShadow->GetFlipbook(), PlayerVisual->GetFlipbook());
			TestEqual(TEXT("player silhouette follows aircraft bank frame"),
				PlayerGroundShadow->GetPlaybackPositionInFrames(), PlayerVisual->GetPlaybackPositionInFrames());
			TestTrue(TEXT("player silhouette follows aircraft rotation and XYZ scale"),
				PlayerGroundShadow->GetRelativeRotation().Equals(PlayerVisual->GetRelativeRotation()) &&
				FMath::IsNearlyEqual(PlayerGroundShadow->GetRelativeScale3D().X, PlayerVisual->GetRelativeScale3D().X) &&
				FMath::IsNearlyEqual(PlayerGroundShadow->GetRelativeScale3D().Y, PlayerVisual->GetRelativeScale3D().Y) &&
				FMath::IsNearlyEqual(PlayerGroundShadow->GetRelativeScale3D().Z, PlayerVisual->GetRelativeScale3D().Z));
		}
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
	TestTrue(TEXT("ground shadow offset preserves visual XY and applies coefficient plus clearance"),
		AStageCameraActor::CalculateGroundShadowOffset(FVector(10.0f, 20.0f, 60.0f), FVector2D(0.57f, -0.40f))
			.Equals(FVector(44.2f, -4.0f, 2.0f), KINDA_SMALL_NUMBER));

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
	USkeletalMeshComponent* GroundShadow = Cast<USkeletalMeshComponent>(
		Enemy->GetDefaultSubobjectByName(TEXT("GroundShadow")));
	TestNotNull(TEXT("enemy ground shadow is a skeletal mesh component"), GroundShadow);
	if (!GroundShadow)
	{
		return false;
	}
	const FVector InitialActorLocation = Enemy->GetActorLocation();
	const FVector InitialHitboxLocation = Enemy->GetRootComponent()->GetRelativeLocation();
	FEnemyRow ShadowRow = *MeshRow;
	ShadowRow.Attribute.bGround = true;
	ShadowRow.Attribute.bAir = false;
	TestTrue(TEXT("ground enemy initializes"), Enemy->InitFromEnemyRow(ShadowRow));
	TestTrue(TEXT("ground enemy enables static native shadow"), Mesh->CastShadow);
	TestTrue(TEXT("ground enemy enables dynamic native shadow"), Mesh->bCastDynamicShadow);
	TestFalse(TEXT("ground enemy hides silhouette shadow"), GroundShadow->IsVisible());

	ShadowRow.Attribute.bGround = false;
	ShadowRow.Attribute.bAir = true;
	TestTrue(TEXT("air enemy reinitializes"), Enemy->InitFromEnemyRow(ShadowRow));
	Enemy->SetAirShadowOpacity(0.8f);
	TestFalse(TEXT("air enemy disables static native shadow"), Mesh->CastShadow);
	TestFalse(TEXT("air enemy disables dynamic native shadow"), Mesh->bCastDynamicShadow);
	TestTrue(TEXT("air enemy shows silhouette shadow"), GroundShadow->IsVisible());
	TestEqual(TEXT("air silhouette uses enemy mesh"), GroundShadow->GetSkeletalMeshAsset(), Mesh->GetSkeletalMeshAsset());
	TestTrue(TEXT("air silhouette follows the enemy mesh pose"),
		GroundShadow->LeaderPoseComponent.Get() == Mesh);
	TestTrue(TEXT("air silhouette flattens Z and preserves XY scale"),
		FMath::IsNearlyEqual(GroundShadow->GetRelativeScale3D().X, Mesh->GetRelativeScale3D().X) &&
		FMath::IsNearlyEqual(GroundShadow->GetRelativeScale3D().Y, Mesh->GetRelativeScale3D().Y) &&
		FMath::IsNearlyEqual(GroundShadow->GetRelativeScale3D().Z, 0.01f));
	TestTrue(TEXT("air silhouette remains at ground clearance"),
		FMath::IsNearlyEqual(GroundShadow->GetRelativeLocation().Z, 2.0f));
	UMaterialInstanceDynamic* EnemyShadowMaterial = Cast<UMaterialInstanceDynamic>(GroundShadow->GetMaterial(0));
	TestNotNull(TEXT("enemy opacity setter creates dynamic shadow material"), EnemyShadowMaterial);
	if (EnemyShadowMaterial)
	{
		float EnemyShadowOpacity = 0.0f;
		TestTrue(TEXT("enemy opacity setter applies scalar parameter"),
			EnemyShadowMaterial->GetScalarParameterValue(TEXT("ShadowOpacity"), EnemyShadowOpacity) &&
			FMath::IsNearlyEqual(EnemyShadowOpacity, 0.8f));
	}

	ShadowRow.Attribute.bGround = true;
	TestTrue(TEXT("mixed enemy reinitializes"), Enemy->InitFromEnemyRow(ShadowRow));
	TestFalse(TEXT("mixed enemy disables static native shadow"), Mesh->CastShadow);
	TestFalse(TEXT("mixed enemy disables dynamic native shadow"), Mesh->bCastDynamicShadow);
	TestTrue(TEXT("mixed enemy restores silhouette shadow"), GroundShadow->IsVisible());

	ShadowRow.Attribute.bAir = false;
	TestTrue(TEXT("ground enemy reinitializes after air enemy"), Enemy->InitFromEnemyRow(ShadowRow));
	TestTrue(TEXT("ground enemy reset restores static native shadow"), Mesh->CastShadow);
	TestTrue(TEXT("ground enemy reset restores dynamic native shadow"), Mesh->bCastDynamicShadow);
	TestFalse(TEXT("ground enemy reset hides silhouette shadow"), GroundShadow->IsVisible());

	ShadowRow.Attribute.bGround = false;
	ShadowRow.Attribute.bAir = true;
	TestTrue(TEXT("air enemy reinitializes before pool release"), Enemy->InitFromEnemyRow(ShadowRow));
	Enemy->SetAirShadowOpacity(0.8f);
	TestTrue(TEXT("pool release starts from a visible air silhouette shadow"), GroundShadow->IsVisible());
	// Transient NewObject actor에는 BlueprintNativeEvent Execute dispatch가 native 구현까지 도달하지 않는다.
	// 이 테스트는 ActorPoolSubsystem 통합이 아니라 AEnemyBase의 풀 lifecycle 상태 초기화를 검증한다.
	Enemy->OnReleasedToPool_Implementation();
	TestFalse(TEXT("pool release hides silhouette shadow"), GroundShadow->IsVisible());
	TestNull(TEXT("pool release clears silhouette leader pose"), GroundShadow->LeaderPoseComponent.Get());
	TestNull(TEXT("pool release clears silhouette mesh"), GroundShadow->GetSkeletalMeshAsset());
	UMaterialInstanceDynamic* ReleasedShadowMaterial = Cast<UMaterialInstanceDynamic>(GroundShadow->GetMaterial(0));
	TestNotNull(TEXT("pool release retains a current dynamic shadow material"), ReleasedShadowMaterial);
	if (ReleasedShadowMaterial)
	{
		float ReleasedShadowOpacity = 0.0f;
		TestTrue(TEXT("pool release resets shadow opacity to default"),
			ReleasedShadowMaterial->GetScalarParameterValue(TEXT("ShadowOpacity"), ReleasedShadowOpacity) &&
			FMath::IsNearlyEqual(ReleasedShadowOpacity, 0.35f));
	}
	Enemy->OnAcquiredFromPool_Implementation();
	TestFalse(TEXT("pool reacquire keeps silhouette hidden until Init"), GroundShadow->IsVisible());
	UMaterialInstanceDynamic* AcquiredShadowMaterial = Cast<UMaterialInstanceDynamic>(GroundShadow->GetMaterial(0));
	TestNotNull(TEXT("pool reacquire retains a current dynamic shadow material"), AcquiredShadowMaterial);
	if (AcquiredShadowMaterial)
	{
		float AcquiredShadowOpacity = 0.0f;
		TestTrue(TEXT("pool reacquire retains reset shadow opacity"),
			AcquiredShadowMaterial->GetScalarParameterValue(TEXT("ShadowOpacity"), AcquiredShadowOpacity) &&
			FMath::IsNearlyEqual(AcquiredShadowOpacity, 0.35f));
	}
	TestTrue(TEXT("render layer initialization does not move enemy actor"),
		Enemy->GetActorLocation().Equals(InitialActorLocation));
	TestTrue(TEXT("render layer initialization does not move enemy hitbox root"),
		Enemy->GetRootComponent()->GetRelativeLocation().Equals(InitialHitboxLocation));

	return true;
}

#endif
