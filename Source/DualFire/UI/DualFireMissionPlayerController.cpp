// Copyright DualFire. All Rights Reserved.

#include "UI/DualFireMissionPlayerController.h"

#include "DualFire.h"
#include "GameModes/DualFireGameModeBase.h"
#include "InputMappingContext.h"
#include "UI/DualFireMissionResultWidgets.h"

ADualFireMissionPlayerController::ADualFireMissionPlayerController()
{
	// 전투 레벨도 시작 화면과 같은 Common UI 루트와 Action Bar 구성을 사용한다.
	RootLayoutClass = nullptr;
	MissionResultWidgetClass = TSoftClassPtr<UDualFireMissionResultWidget>(
		FSoftClassPath(TEXT("/Game/Blueprint/UI/Screen/WBP_MissionResult.WBP_MissionResult_C")));
}

void ADualFireMissionPlayerController::BeginPlay()
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

void ADualFireMissionPlayerController::HandleMissionEnded(EMissionResult Result)
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	ADualFireGameModeBase* GameMode = GetWorld()->GetAuthGameMode<ADualFireGameModeBase>();
	TSubclassOf<UDualFireMissionResultWidget> ResultWidgetClass = MissionResultWidgetClass.LoadSynchronous();
	if (!IsValid(GameMode) || !IsValid(ResultWidgetClass))
	{
		UE_LOG(LogDualFire, Error, TEXT("[UI] 미션 결과 화면 클래스 또는 GameMode를 찾지 못함"));
		return;
	}

	ClearLayer(EDualFireUILayer::Menu);
	UDualFireMissionResultWidget* ResultWidget = Cast<UDualFireMissionResultWidget>(
		PushWidgetToLayer(EDualFireUILayer::Menu, ResultWidgetClass));
	if (!IsValid(ResultWidget))
	{
		UE_LOG(LogDualFire, Error, TEXT("[UI] 미션 결과 화면 Push 실패"));
		return;
	}

	ResultWidget->SetMissionResultData(GameMode->GetMissionResultData());
}
