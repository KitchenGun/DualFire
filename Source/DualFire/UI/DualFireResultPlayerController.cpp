// Copyright DualFire. All Rights Reserved.

#include "UI/DualFireResultPlayerController.h"

#include "DualFire.h"
#include "Engine/GameInstance.h"
#include "GameMapsSettings.h"
#include "GameInstance/DualFireMissionFlowSubsystem.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "UI/DualFireMissionResultWidgets.h"

ADualFireResultPlayerController::ADualFireResultPlayerController()
{
	RootLayoutClass = nullptr;
	MissionResultWidgetClass = TSoftClassPtr<UDualFireMissionResultWidget>(
		FSoftClassPath(TEXT("/Game/Blueprint/UI/Screen/WBP_MissionResult.WBP_MissionResult_C")));
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

	UDualFireMissionFlowSubsystem* Flow =
		GetGameInstance() ? GetGameInstance()->GetSubsystem<UDualFireMissionFlowSubsystem>() : nullptr;
	if (!IsValid(Flow) || !Flow->HasPendingResult())
	{
		UE_LOG(LogDualFire, Error, TEXT("[UI] 표시할 미션 결과 데이터가 없어 미션 선택으로 복귀"));
		ReturnToMissionSelectWhenResultIsMissing();
		return;
	}

	TSubclassOf<UDualFireMissionResultWidget> ResultWidgetClass =
		MissionResultWidgetClass.LoadSynchronous();
	UDualFireMissionResultWidget* ResultWidget = Cast<UDualFireMissionResultWidget>(
		PushWidgetToLayer(EDualFireUILayer::Menu, ResultWidgetClass));
	if (!IsValid(ResultWidget))
	{
		UE_LOG(LogDualFire, Error, TEXT("[UI] 미션 결과 화면 Push 실패"));
		ReturnToMissionSelectWhenResultIsMissing();
		return;
	}

	ResultWidget->SetMissionResultData(Flow->GetPendingResult());
}

void ADualFireResultPlayerController::ReturnToMissionSelectWhenResultIsMissing()
{
	UGameplayStatics::OpenLevel(this, FName(*UGameMapsSettings::GetGameDefaultMap()));
}
