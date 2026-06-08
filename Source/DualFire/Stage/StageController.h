// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/DualFireTypes.h"
#include "Core/DualFireDataTypes.h"
#include "StageController.generated.h"

class AEnemyBase;
class AStageCameraActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStageStateChanged, EStageState, NewState);

/**
 * 타임라인 / 웨이브 / 엘리트 전투 / 미션 결과를 관리하는 스테이지 컨트롤러.
 * 레벨에 하나 배치하거나 GameMode가 스폰한다.
 *
 * 현재 구현:
 *   - EStageState 상태머신 (Timeline → EliteCombat → Ended)
 *   - Tick 기반 웨이브 트리거 (TriggerTime 오름차순)
 *   - ESpawnAnchor → 월드 좌표 변환 (StageCameraActor GetPlayableBounds 이용)
 *   - DataTable 없을 때 TestWaves 하드코딩 배열로 대체
 *
 * 미구현: GameMode.OnMissionFail/Clear 연결 (다음 청크)
 */
UCLASS(BlueprintType, Blueprintable)
class DUALFIRE_API AStageController : public AActor
{
	GENERATED_BODY()

public:
	AStageController();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// ── 상태 ─────────────────────────────────────────────────────────────────

	/** 현재 스테이지 단계. Timeline → EliteCombat → Ended. 런타임 전용 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Stage")
	EStageState CurrentState = EStageState::Timeline;

	/** Timeline 시작 후 누적 경과 시간(초). 웨이브·엘리트 트리거 기준 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Stage")
	float ElapsedTime = 0.0f;

	/** 상태 전환 시 발생. HUD 단계 표시·연출 트리거용 */
	UPROPERTY(BlueprintAssignable, Category="Stage|Events")
	FOnStageStateChanged OnStageStateChanged;

	// ── 스테이지 파라미터 ─────────────────────────────────────────────────────

	/** 이 시간(초) 경과 후 EliteCombat으로 전환 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stage", meta=(ClampMin="1.0"))
	float EliteTriggerTime = 80.0f;

	/** EliteCombat 제한 시간(초). 이 시간 내에 처리 못하면 EliteCombat → Ended (실패) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stage", meta=(ClampMin="1.0"))
	float EliteTimeLimit = 60.0f;

	// ── 적 스폰 ───────────────────────────────────────────────────────────────

	/**
	 * 웨이브에서 EnemyID를 찾지 못할 때 사용하는 기본 적 클래스.
	 * BP_EnemyBase 등을 할당하면 DataTable 없이도 테스트 가능.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Stage|Spawn")
	TSubclassOf<AEnemyBase> DefaultEnemyClass;

	/** EnemyID → 적 클래스 매핑. DataTable 없이 BP에서 직접 지정 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stage|Spawn")
	TMap<FName, TSubclassOf<AEnemyBase>> EnemyClassMap;

	/** 스폰 앵커 X 위치 = 화면 우측 경계 + 이 값 (cm) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stage|Spawn", meta=(ClampMin="0.0"))
	float SpawnMarginX = 300.0f;

	// ── 테스트 웨이브 (DataTable 없을 때 사용) ────────────────────────────────

	/**
	 * true: TestWaves 하드코딩 배열 사용.
	 * false: WaveDataTable + StageID 조합으로 DataTable 조회 (미구현).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stage|Debug")
	bool bUseTestWaves = true;

	/** bUseTestWaves=true일 때 사용할 웨이브 목록. 에디터에서 직접 편집 가능 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stage|Debug")
	TArray<FWaveRow> TestWaves;

	// ── 공개 API ──────────────────────────────────────────────────────────────

	/** EliteCombat 단계에서 엘리트 적이 격파되면 호출 → Ended(클리어)로 전환 */
	UFUNCTION(BlueprintCallable, Category="Stage")
	void OnEliteDefeated();

	UFUNCTION(BlueprintPure, Category="Stage")
	float GetElapsedTime() const { return ElapsedTime; }

private:
	/** TriggerTime 오름차순으로 정렬된 실행 대상 웨이브 목록 */
	TArray<FWaveRow> ActiveWaves;

	/** 다음에 트리거할 ActiveWaves 인덱스 */
	int32 NextWaveIndex = 0;

	/** EliteCombat 제한 시간 타이머 핸들 */
	FTimerHandle EliteTimeLimitHandle;

	/** ActiveWaves를 구성하고 TriggerTime 기준 정렬 (BeginPlay) */
	void BuildActiveWaves();

	/** Timeline 단계 Tick — 경과 시간 누적, 웨이브·엘리트 트리거 */
	void TickTimeline(float DeltaTime);

	/** 단일 웨이브 발동 — 순차 스폰 시작 */
	void TriggerWave(const FWaveRow& Wave);

	/** SpawnInterval 간격으로 적을 한 마리씩 재귀 스폰 */
	void SpawnWaveSequential(FWaveRow Wave, int32 AlreadySpawned);

	/** EnemyID → 적 클래스 해석. EnemyClassMap 우선, 없으면 DefaultEnemyClass */
	TSubclassOf<AEnemyBase> ResolveEnemyClass(FName EnemyID) const;

	/** ESpawnAnchor + 오프셋 → 월드 좌표. 카메라 GetPlayableBounds 기반 */
	FVector ResolveSpawnAnchor(ESpawnAnchor Anchor, const FVector& Offset) const;

	/** GameMode를 거쳐 현재 StageCameraActor 획득 */
	AStageCameraActor* GetStageCamera() const;

	/** 상태 전환 + 브로드캐스트 + 단계별 진입 처리 */
	void SetState(EStageState NewState);

	/** 엘리트 제한 시간 만료 콜백 → Ended(실패) */
	void OnEliteTimeLimitExpired();
};
