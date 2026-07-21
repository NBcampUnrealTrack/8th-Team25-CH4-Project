// FDCapturePopupWidget.cpp

#include "UI/FDCapturePopupWidget.h"
#include "Components/Button.h"
#include "Controller/FDPlayerController.h"

void UFDCapturePopupWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Out)
	{
		Out->OnClicked.AddDynamic(this, &UFDCapturePopupWidget::OnClickedOut);
	}
	if (Spare)
	{
		Spare->OnClicked.AddDynamic(this, &UFDCapturePopupWidget::OnClickedSpare);
	}
}

void UFDCapturePopupWidget::OnClickedOut()
{
	if (AFDPlayerController* PC = Cast<AFDPlayerController>(GetOwningPlayer()))
	{
		PC->Server_RequestOut();
	}
	RemoveFromParent();
}

void UFDCapturePopupWidget::OnClickedSpare()
{
	if (AFDPlayerController* PC = Cast<AFDPlayerController>(GetOwningPlayer()))
	{
		PC->Server_RequestSpare();
	}
	RemoveFromParent();
}