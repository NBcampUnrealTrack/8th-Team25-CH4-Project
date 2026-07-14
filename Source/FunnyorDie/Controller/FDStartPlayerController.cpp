// FDStartPlayerController.cpp

#include "Controller/FDStartPlayerController.h"
#include "UI/FDStartWidget.h"
#include "Blueprint/UserWidget.h"

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