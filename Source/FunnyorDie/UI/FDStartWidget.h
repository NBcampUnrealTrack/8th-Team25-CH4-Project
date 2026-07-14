// FDStartWidget.h

#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameInstance/FDGameInstance.h"
#include "FDStartWidget.generated.h"

class UButton;
class UTextBlock;
class UWidget;

UCLASS()
class FUNNYORDIE_API UFDStartWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	
	// Play
	UPROPERTY(meta = (BindWidget))
	UButton* StartButton;

	UPROPERTY(meta = (BindWidget))
	UButton* HowToPlayButton;

	UPROPERTY(meta = (BindWidget))
	UButton* QuitButton;

	UPROPERTY(meta = (BindWidgetOptional))
	UWidget* HowToPlayPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* CloseHowToPlayButton;
	
	UPROPERTY(meta = (BindWidgetOptional))
	UWidget* SessionPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* HostButton;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* JoinButton;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BackButton;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* StatusText;


	UFUNCTION()
	void OnStartClicked();    

	UFUNCTION()
	void OnBackClicked();      

	UFUNCTION()
	void OnHostClicked();     

	UFUNCTION()
	void OnJoinClicked();   

	UFUNCTION()
	void OnQuitClicked();

	UFUNCTION()
	void OnHowToPlayClicked();

	UFUNCTION()
	void OnCloseHowToPlayClicked();

	// GameInstance의 상태 방송을 받는 함수
	UFUNCTION()
	void HandleSessionStatusChanged(EFDSessionStatus NewStatus);

private:
	void ApplyStatusToUI(EFDSessionStatus Status);
	void SetSessionPanelOpen(bool bOpen);

	UPROPERTY()
	UFDGameInstance* FDGameInstance = nullptr;
};