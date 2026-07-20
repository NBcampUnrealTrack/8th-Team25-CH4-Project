// FDStartPlayerController.cpp

#include "Controller/FDStartPlayerController.h"
#include "UI/FDStartWidget.h"
#include "Blueprint/UserWidget.h"
#include "GameInstance/FDGameInstance.h"

void AFDStartPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController() && StartWidgetClass)
	{
		StartWidget = CreateWidget<UFDStartWidget>(this, StartWidgetClass);
		StartWidget->AddToViewport();

		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(StartWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);

		bShowMouseCursor = true;
	}

	// 시작 화면 브금 재생 (클라이언트 로컬)
	if (IsLocalController())
	{
		if (UFDGameInstance* GI = GetGameInstance<UFDGameInstance>())
		{
			GI->PlayMusic(GI->StartMusic);
		}
	}
}

void AFDStartPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsLocalController())
	{
		SetInputMode(FInputModeGameOnly());
		bShowMouseCursor = false;
	}

	Super::EndPlay(EndPlayReason);
}