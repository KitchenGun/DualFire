#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/DataTable.h"
#include "Stage/StageController.h"
#include "UObject/UObjectGlobals.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDualFireStageDataDrivenTimelineTest,
	"DualFire.Stage.DataDrivenTimeline",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDualFireStageDataDrivenTimelineTest::RunTest(const FString& Parameters)
{
	const UDataTable* StageTable = LoadObject<UDataTable>(
		nullptr,
		TEXT("/Game/Data/Stage/DT_Stages.DT_Stages"));
	TestNotNull(TEXT("DT_Stages loads"), StageTable);
	if (!StageTable)
	{
		return false;
	}

	const FStageRow* StageRow = StageTable->FindRow<FStageRow>(
		TEXT("STAGE_TEST"),
		TEXT("DualFire.Stage.DataDrivenTimeline"));
	TestNotNull(TEXT("STAGE_TEST exists"), StageRow);
	if (!StageRow)
	{
		return false;
	}

	TestTrue(TEXT("0s evaluates to max speed"),
		FMath::IsNearlyEqual(AStageController::EvaluateScrollSpeed(*StageRow, 0.0f), 200.0f));
	TestTrue(TEXT("25s evaluates to zero speed"),
		FMath::IsNearlyZero(AStageController::EvaluateScrollSpeed(*StageRow, 25.0f)));
	TestTrue(TEXT("zero speed is held at 30s"),
		FMath::IsNearlyZero(AStageController::EvaluateScrollSpeed(*StageRow, 30.0f)));
	TestTrue(TEXT("zero speed is held at 40s"),
		FMath::IsNearlyZero(AStageController::EvaluateScrollSpeed(*StageRow, 40.0f)));
	TestTrue(TEXT("55s evaluates to max speed"),
		FMath::IsNearlyEqual(AStageController::EvaluateScrollSpeed(*StageRow, 55.0f), 200.0f));

	FStagePauseTrigger RealTimeTrigger;
	RealTimeTrigger.ResumeCondition = EStagePauseResumeCondition::RealTime;
	RealTimeTrigger.ResumeDelay = 1.0f;
	TestFalse(TEXT("real-time pause waits for delay"),
		AStageController::ShouldResumePause(RealTimeTrigger, 0.99f, 0, 0, false));
	TestTrue(TEXT("real-time pause resumes at delay"),
		AStageController::ShouldResumePause(RealTimeTrigger, 1.0f, 0, 0, false));

	FStagePauseTrigger WaveTrigger;
	WaveTrigger.ResumeCondition = EStagePauseResumeCondition::WaveDefeated;
	TestFalse(TEXT("wave pause waits for active enemies"),
		AStageController::ShouldResumePause(WaveTrigger, 0.0f, 1, 0, false));
	TestFalse(TEXT("wave pause waits for sequential spawns"),
		AStageController::ShouldResumePause(WaveTrigger, 0.0f, 0, 1, false));
	TestTrue(TEXT("wave pause resumes after scope is empty"),
		AStageController::ShouldResumePause(WaveTrigger, 0.0f, 0, 0, false));

	FStagePauseTrigger EnemyTrigger;
	EnemyTrigger.ResumeCondition = EStagePauseResumeCondition::EnemyDefeated;
	TestFalse(TEXT("enemy pause waits for target"),
		AStageController::ShouldResumePause(EnemyTrigger, 0.0f, 0, 0, false));
	TestTrue(TEXT("enemy pause resumes after target defeat"),
		AStageController::ShouldResumePause(EnemyTrigger, 0.0f, 0, 0, true));

	TArray<FStagePauseTrigger> PauseTriggers;
	PauseTriggers.SetNum(3);
	PauseTriggers[0].TriggerTime = 18.0f;
	PauseTriggers[1].TriggerTime = 22.0f;
	PauseTriggers[2].TriggerTime = 42.0f;
	TArray<FWaveRow> Waves;
	Waves.SetNum(2);
	Waves[0].TriggerTime = 22.0f;
	Waves[1].TriggerTime = 42.0f;

	TestTrue(TEXT("large frame is split at the 18s boundary"), FMath::IsNearlyEqual(
		AStageController::FindNextTimelineBoundary(80.0f, PauseTriggers, 0, Waves, 0, 0.0f), 18.0f));
	TestTrue(TEXT("next segment stops at the shared 22s boundary"), FMath::IsNearlyEqual(
		AStageController::FindNextTimelineBoundary(80.0f, PauseTriggers, 1, Waves, 0, 18.0f), 22.0f));
	TestTrue(TEXT("timeline boundaries remain active while curve speed is zero"), FMath::IsNearlyEqual(
		AStageController::FindNextTimelineBoundary(80.0f, PauseTriggers, 2, Waves, 1, 30.0f), 42.0f));
	TestTrue(TEXT("pause event wins a shared timestamp"),
		AStageController::ShouldProcessPauseFirst(22.0f, 22.0f));
	TestFalse(TEXT("an earlier wave is not reordered behind a later pause"),
		AStageController::ShouldProcessPauseFirst(23.0f, 22.0f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDualFireStageSpawnAnchorsTest,
	"DualFire.Stage.SpawnAnchors",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDualFireStageSpawnAnchorsTest::RunTest(const FString& Parameters)
{
	const ESpawnAnchor Anchors[] = {
		ESpawnAnchor::TopLeft,
		ESpawnAnchor::TopCenter,
		ESpawnAnchor::TopRight,
		ESpawnAnchor::Left,
		ESpawnAnchor::Center,
		ESpawnAnchor::Right,
		ESpawnAnchor::BottomLeft,
		ESpawnAnchor::BottomCenter,
		ESpawnAnchor::BottomRight,
	};

	TArray<FVector2D> Ratios;
	for (const ESpawnAnchor Anchor : Anchors)
	{
		const FVector2D Ratio = AStageController::GetSpawnAnchorRatios(Anchor);
		TestTrue(TEXT("anchor X ratio stays inside playable bounds"), Ratio.X >= 0.0f && Ratio.X <= 1.0f);
		TestTrue(TEXT("anchor Y ratio stays inside playable bounds"), Ratio.Y >= 0.0f && Ratio.Y <= 1.0f);
		Ratios.Add(Ratio);
	}

	for (int32 LeftIndex = 0; LeftIndex < Ratios.Num(); ++LeftIndex)
	{
		for (int32 RightIndex = LeftIndex + 1; RightIndex < Ratios.Num(); ++RightIndex)
		{
			TestFalse(TEXT("all nine anchor coordinates are distinct"),
				Ratios[LeftIndex].Equals(Ratios[RightIndex]));
		}
	}

	return true;
}

#endif
