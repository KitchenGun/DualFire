// Copyright DualFire. All Rights Reserved.

#include "UI/DualFireResultPlayerController.h"

#include "DualFire.h"
#include "CommonActivatableWidget.h"
#include "Engine/GameInstance.h"
#include "GameInstance/DualFireMissionResultSubsystem.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "UI/DualFireMissionResultWidgets.h"

ADualFireResultPlayerController::ADualFireResultPlayerController()
{
	RootLayoutClass = nullptr;
	MissionResultWidgetClass = TSoftClassPtr<UDualFireMissionResultWidget>(
		FSoftClassPath(TEXT("/Game/Blueprint/UI/Screen/WBP_MissionResult.WBP_MissionResult_C")));
	MissionResultBackgroundClass = TSoftClassPtr<UCommonActivatableWidget>(
		FSoftClassPath(TEXT(
			"/Game/Blueprint/UI/Screen/WBP_MissionResultBackground.WBP_MissionResultBackground_C")));
}

void ADualFireResultPlayerController::BeginPlay()
{
	if (!IsValid(RootLayoutClass))
	{
		RootLayoutClass = LoadClass<UDualFirePrimaryLayout>(
			nullptr,
			TEXT("/Game/Blueprint/UI/Root/WBP_StartRoot.WBP_StartRoot_C"));
	}
	if (!IsValid(UIInputMapping))
	{
		UIInputMapping = LoadObject<UInputMappingContext>(
			nullptr,
			TEXT("/Game/Input/UI/IMC_UI_Menu.IMC_UI_Menu"));
	}

	Super::BeginPlay();

	if (!IsLocalPlayerController())
	{
		return;
	}

	UDualFireMissionResultSubsystem* ResultSubsystem =
		GetGameInstance() ? GetGameInstance()->GetSubsystem<UDualFireMissionResultSubsystem>() : nullptr;
	if (!IsValid(ResultSubsystem) || !ResultSubsystem->HasPendingResult())
	{
		UE_LOG(LogDualFire, Error, TEXT("[UI] 표시할 미션 결과 데이터가 없어 로비로 복귀"));
		OpenLobbyWhenResultIsMissing();
		return;
	}

	TSubclassOf<UCommonActivatableWidget> BackgroundClass =
		MissionResultBackgroundClass.LoadSynchronous();
	if (!IsValid(PushWidgetToLayer(EDualFireUILayer::Game, BackgroundClass)))
	{
		UE_LOG(LogDualFire, Error, TEXT("[UI] 미션 결과 전체 화면 배경 Push 실패"));
		OpenLobbyWhenResultIsMissing();
		return;
	}

	TSubclassOf<UDualFireMissionResultWidget> ResultWidgetClass =
		MissionResultWidgetClass.LoadSynchronous();
	UDualFireMissionResultWidget* ResultWidget = Cast<UDualFireMissionResultWidget>(
		PushWidgetToLayer(EDualFireUILayer::Menu, ResultWidgetClass));
	if (!IsValid(ResultWidget))
	{
		UE_LOG(LogDualFire, Error, TEXT("[UI] 미션 결과 화면 Push 실패"));
		OpenLobbyWhenResultIsMissing();
		return;
	}

	ResultWidget->SetMissionResultData(ResultSubsystem->GetPendingResult());
}

void ADualFireResultPlayerController::OpenLobbyWhenResultIsMissing()
{
	UGameplayStatics::OpenLevel(this, FName(TEXT("/Game/Level/LV_Start")));
}
