// Copyright DualFire. All Rights Reserved.

#include "UI/DualFireMissionPlayerController.h"

#include "DualFire.h"
#include "GameInstance/DualFireMissionResultSubsystem.h"
#include "GameModes/DualFireGameModeBase.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

void ADualFireMissionPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (ADualFireGameModeBase* GameMode = GetWorld()->GetAuthGameMode<ADualFireGameModeBase>())
	{
		GameMode->OnMissionEnded.AddDynamic(this, &ThisClass::HandleMissionEnded);
	}
}

void ADualFireMissionPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ADualFireGameModeBase* GameMode = GetWorld()->GetAuthGameMode<ADualFireGameModeBase>())
	{
		GameMode->OnMissionEnded.RemoveDynamic(this, &ThisClass::HandleMissionEnded);
	}

	Super::EndPlay(EndPlayReason);
}

void ADualFireMissionPlayerController::HandleMissionEnded(EMissionResult /*Result*/)
{
	if (!IsLocalPlayerController() || bResultTravelStarted)
	{
		return;
	}

	UDualFireMissionResultSubsystem* ResultSubsystem =
		GetGameInstance() ? GetGameInstance()->GetSubsystem<UDualFireMissionResultSubsystem>() : nullptr;
	if (!IsValid(ResultSubsystem) || !ResultSubsystem->HasPendingResult())
	{
		UE_LOG(LogDualFire, Error, TEXT("[MissionResult] 결과 데이터 저장 실패로 레벨 전환 중단"));
		return;
	}

	bResultTravelStarted = true;
	UGameplayStatics::OpenLevel(this, ResultLevelName);
}
