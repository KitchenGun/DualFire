// Copyright DualFire. All Rights Reserved.

#include "UI/DualFireMissionPlayerController.h"

#include "DualFire.h"
#include "GameInstance/DualFireMissionResultSubsystem.h"
#include "GameModes/DualFireGameModeBase.h"
#include "Player/DualFirePlayerPawn.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"

void ADualFireMissionPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (ADualFireGameModeBase* GameMode = GetWorld()->GetAuthGameMode<ADualFireGameModeBase>())
	{
		GameMode->OnMissionEnded.AddDynamic(this, &ThisClass::HandleMissionEnded);
	}

	AddGameplayInputMapping(GetPawn());
}

void ADualFireMissionPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveGameplayInputMapping();

	if (ADualFireGameModeBase* GameMode = GetWorld()->GetAuthGameMode<ADualFireGameModeBase>())
	{
		GameMode->OnMissionEnded.RemoveDynamic(this, &ThisClass::HandleMissionEnded);
	}

	Super::EndPlay(EndPlayReason);
}

void ADualFireMissionPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	RemoveGameplayInputMapping();
	AddGameplayInputMapping(InPawn);
}

void ADualFireMissionPlayerController::OnUnPossess()
{
	RemoveGameplayInputMapping();
	Super::OnUnPossess();
}

void ADualFireMissionPlayerController::AddGameplayInputMapping(APawn* InPawn)
{
	if (IsValid(ActiveGameplayInputMapping) || !IsLocalPlayerController())
	{
		return;
	}

	const ADualFirePlayerPawn* PlayerPawn = Cast<ADualFirePlayerPawn>(InPawn);
	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!IsValid(PlayerPawn) || !IsValid(LocalPlayer))
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
		LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	UInputMappingContext* MappingContext = PlayerPawn->ResolveInputMappingContext();
	if (!IsValid(InputSubsystem) || !IsValid(MappingContext))
	{
		return;
	}

	InputSubsystem->AddMappingContext(MappingContext, PlayerPawn->GetInputMappingPriority());
	ActiveGameplayInputMapping = MappingContext;
}

void ADualFireMissionPlayerController::RemoveGameplayInputMapping()
{
	UInputMappingContext* MappingContext = ActiveGameplayInputMapping;
	ActiveGameplayInputMapping = nullptr;
	if (!IsValid(MappingContext))
	{
		return;
	}

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
			LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			InputSubsystem->RemoveMappingContext(MappingContext);
		}
	}
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
