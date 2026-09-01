// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/DualFireTypes.h"
#include "Core/DualFireDataTypes.h"
#include "StageController.generated.h"

class AEnemyBase;
class ADualFirePrototypeBossCube;
class AStageCameraActor;
class AEnemySpawnPoint;
class ABaseProjectile;
class APawn;
class UDataTable;
class UCurveFloat;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStageStateChanged, EStageState, NewState);

/**
 * ConfigureStage 검증을 통과한 StageRow와 WaveRow만 커밋하고,
 * 타임라인 / 웨이브 / 데이터 기반 스크롤·멈춤 / 미션 결과를 관리한다.
 */
UCLASS(BlueprintType, Blueprintable)
class DUALFIRE_API AStageController : public AActor
{
	GENERATED_BODY()

public:
	AStageController();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;

	// ── 상태 ─────────────────────────────────────────────────────────────────

	/** 현재 스테이지 단계. Timeline → BossSequence → Ended. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Stage")
	EStageState CurrentState = EStageState::Timeline;

	/** 스테이지 누적 경과 시간(초). Pause 트리거 동안에는 증가하지 않는다. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Stage")
	float ElapsedTime = 0.0f;

	/** 상태 전환 시 발생. HUD 단계 표시·연출 트리거용 */
	UPROPERTY(BlueprintAssignable, Category="Stage|Events")
	FOnStageStateChanged OnStageStateChanged;

	// ── 스테이지 파라미터 ─────────────────────────────────────────────────────

	/** 이 시간(초) 경과 후 보스 시퀀스로 전환 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stage", meta=(ClampMin="1.0"))
	float BossTriggerTime = 80.0f;

	/** 프로토타입 보스가 화면 안으로 내려오는 시간 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stage|Prototype Boss", meta=(ClampMin="0.1"))
	float PrototypeBossDescentDuration = 3.0f;

	/** 도착 이벤트가 오지 않을 때 미션을 실패시키는 제한 시간 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stage|Prototype Boss", meta=(ClampMin="0.1"))
	float PrototypeBossArrivalTimeout = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stage|Prototype Boss", meta=(ClampMin="0.0"))
	float PrototypeBossSpawnOffset = 270.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stage|Prototype Boss", meta=(ClampMin="0.0", ClampMax="1.0"))
	float PrototypeBossDestinationRatio = 0.33f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Stage|Prototype Boss")
	TSubclassOf<ADualFirePrototypeBossCube> PrototypeBossClass;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Stage|Data")
	FName StageID = TEXT("STAGE_TEST");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stage|Data")
	TObjectPtr<UDataTable> WaveDataTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stage|Data")
	TObjectPtr<UDataTable> EnemyDataTable;

	// ── 적 스폰 ───────────────────────────────────────────────────────────────

	/** EnemyClassMap에서 EnemyID를 찾지 못할 때 사용하는 클래스 폴백. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Stage|Spawn")
	TSubclassOf<AEnemyBase> DefaultEnemyClass;

	/** EnemyID → 적 클래스 매핑. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stage|Spawn")
	TMap<FName, TSubclassOf<AEnemyBase>> EnemyClassMap;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stage|Pool", meta=(ClampMin="0"))
	int32 EnemyPrewarmCountPerClass = 32;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stage|Pool")
	TSubclassOf<ABaseProjectile> EnemyProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stage|Pool", meta=(ClampMin="0"))
	int32 EnemyProjectilePrewarmCount = 128;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stage|Pool")
	TArray<TSubclassOf<AActor>> PlayerProjectilePrewarmClasses;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stage|Pool", meta=(ClampMin="0"))
	int32 PlayerProjectilePrewarmCountPerClass = 64;

	// ── 공개 API ──────────────────────────────────────────────────────────────

	/** StageID를 해석·검증하고 성공한 데이터만 런타임 상태로 커밋한다. */
	UFUNCTION(BlueprintCallable, Category="Stage")
	bool ConfigureStage(FName InStageID, FText& OutError);

	UFUNCTION(BlueprintPure, Category="Stage")
	float GetElapsedTime() const { return ElapsedTime; }

	UFUNCTION(BlueprintPure, Category="Stage|Result")
	int32 GetAirEnemiesSpawned() const { return AirEnemiesSpawned; }

	UFUNCTION(BlueprintPure, Category="Stage|Result")
	int32 GetAirEnemiesDefeated() const { return AirEnemiesDefeated; }

	UFUNCTION(BlueprintPure, Category="Stage|Result")
	int32 GetGroundEnemiesSpawned() const { return GroundEnemiesSpawned; }

	UFUNCTION(BlueprintPure, Category="Stage|Result")
	int32 GetGroundEnemiesDefeated() const { return GroundEnemiesDefeated; }

	void RegisterEnemy(AEnemyBase* Enemy);
	void UnregisterEnemy(AEnemyBase* Enemy);
	void NotifyEnemyDefeated(AEnemyBase* Enemy);

	static float EvaluateScrollSpeed(const FStageRow& StageRow, float StageTime);
	static bool ShouldResumePause(
		const FStagePauseTrigger& Trigger,
		float PauseElapsedTime,
		int32 ActiveScopedEnemies,
		int32 PendingScopedSpawns,
		bool bTargetEnemyDefeated);
	static FVector2D GetSpawnAnchorRatios(ESpawnAnchor Anchor);
	static float FindNextTimelineBoundary(
		float InBossTriggerTime,
		const TArray<FStagePauseTrigger>& PauseTriggers,
		int32 PauseTriggerIndex,
		const TArray<FWaveRow>& Waves,
		int32 WaveIndex,
		float InElapsedTime);
	static bool ShouldProcessPauseFirst(float PauseTriggerTime, float WaveTriggerTime);
	/** Automation seam for point-ID validation performed before runtime state commit. */
	static bool AreSpawnPointReferencesValid(
		const TArray<FWaveRow>& Waves,
		const TArray<FName>& SpawnPointIDs);
	static FVector ResolveSpawnPointLocation(const FVector& SpawnPointLocation, const FVector& SpawnOffset);
	/** Stage/Wave 높이 비율 검증과 웨이브별 EnemyRow 합성에 쓰는 Automation seam. */
	static bool IsRenderHeightRatioValid(float Ratio, float MinimumRatio);
	static FEnemyRow ResolveWaveEnemyRow(const FEnemyRow& EnemyRow, const FWaveRow& Wave);

private:
	/** TriggerTime 오름차순으로 정렬된 실행 대상 웨이브 목록 */
	TArray<FWaveRow> ActiveWaves;

	/** 다음에 트리거할 ActiveWaves 인덱스 */
	int32 NextWaveIndex = 0;

	FStageRow ActiveStageRow;
	TArray<FStagePauseTrigger> ActivePauseTriggers;
	int32 NextPauseTriggerIndex = 0;
	FStagePauseTrigger ActivePauseTrigger;
	TSet<TWeakObjectPtr<AEnemyBase>> PauseScopedEnemies;
	float PauseElapsedTime = 0.0f;
	int32 PendingPauseScopedSpawns = 0;
	int32 PauseScopeGeneration = 0;
	bool bTargetEnemyDefeated = false;
	bool bStagePaused = false;
	bool bConfigured = false;
	TMap<FName, TWeakObjectPtr<AEnemySpawnPoint>> SpawnPointCache;

	FTimerHandle PrototypeBossArrivalTimeoutHandle;

	TArray<FTimerHandle> SequenceSpawnTimerHandles;

	UPROPERTY()
	TArray<TObjectPtr<AEnemyBase>> ActiveEnemies;

	UPROPERTY()
	TObjectPtr<ADualFirePrototypeBossCube> ActivePrototypeBoss;

	TWeakObjectPtr<APawn> CachedPlayerPawn;
	FVector CachedPlayerLocation = FVector::ZeroVector;
	bool bHasCachedPlayerLocation = false;
	int32 AirEnemiesSpawned = 0;
	int32 AirEnemiesDefeated = 0;
	int32 GroundEnemiesSpawned = 0;
	int32 GroundEnemiesDefeated = 0;

	bool BuildValidatedWaves(FName InStageID, TArray<FWaveRow>& OutWaves, FText& OutError, FName& OutField) const;
	bool ValidateStageRow(FName InStageID, const FStageRow& Row, const TArray<FWaveRow>& Waves, FText& OutError, FName& OutField) const;
	void LogConfigurationError(FName InStageID, FName Field, const FText& Error) const;

	/** Timeline 단계 Tick — 경과 시간 누적, 웨이브·보스 트리거 */
	void TickTimeline(float DeltaTime);

	/** 단일 웨이브 발동 — 순차 스폰 시작 */
	void TriggerWave(const FWaveRow& Wave, int32 ScopeGeneration);

	/** SpawnInterval 간격으로 적을 한 마리씩 재귀 스폰 */
	void SpawnWaveSequential(FWaveRow Wave, int32 AlreadySpawned, int32 ScopeGeneration);
	void MarkPauseScopedSpawnComplete(int32 ScopeGeneration, AEnemyBase* SpawnedEnemy);

	/** EnemyID → 적 클래스 해석. EnemyClassMap 우선, 없으면 DefaultEnemyClass */
	TSubclassOf<AEnemyBase> ResolveEnemyClass(FName EnemyID) const;

	/** ESpawnAnchor + 오프셋 → 월드 좌표. 카메라 GetPlayableBounds 기반 */
	FVector ResolveSpawnAnchor(ESpawnAnchor Anchor, const FVector& Offset) const;
	FVector ResolveWaveSpawnLocation(const FWaveRow& Wave) const;

	/** GameMode를 거쳐 현재 StageCameraActor 획득 */
	AStageCameraActor* GetStageCamera() const;

	/** 상태 전환 + 브로드캐스트 + 단계별 진입 처리 */
	void SetState(EStageState NewState);
	void ProcessTimelineBoundary();
	float GetNextTimelineBoundary() const;
	void BeginStagePause(const FStagePauseTrigger& Trigger);
	void TickStagePause(float& RemainingTime);
	void EvaluateStagePause();
	void EndStagePause();
	void UpdateCameraScrollSpeed();

	void BeginPrototypeBossSequence();
	void StopCombatForPrototypeBoss();
	void HandlePrototypeBossDestinationReached();
	void HandlePrototypeBossArrivalTimeout();

	void PrewarmPools();
	void ClearSequenceSpawnTimers();
	void RegisterPlacedEnemies();
	bool FindEnemyRow(FName EnemyID, FEnemyRow& OutEnemyRow) const;
	void TickEnemyAI(float DeltaTime);
	FVector GetCachedPlayerLocation();
	void CachePlayerPawn(APawn* NewPawn);
	void RecordEnemySpawned(const FEnemyAttribute& Attribute);

	UFUNCTION()
	void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);
};
