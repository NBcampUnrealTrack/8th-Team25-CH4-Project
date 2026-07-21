// FDCapturePopupWidget.h

#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FDCapturePopupWidget.generated.h"

UCLASS()
class FUNNYORDIE_API UFDCapturePopupWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	UPROPERTY(meta = (BindWidget))
	class UButton* Out;

	UPROPERTY(meta = (BindWidget))
	class UButton* Spare;

	virtual void NativeConstruct() override;

	UFUNCTION()
	void OnClickedOut();

	UFUNCTION()
	void OnClickedSpare();
};