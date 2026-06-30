// FDLobbyWidget.h

#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FDLobbyWidget.generated.h"

class UButton;
class UTextBlock;

UCLASS()
class FUNNYORDIE_API UFDLobbyWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	UButton* StartButton;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* WaitingText; // 대기자에게 보여줄 안내 문구 

	UFUNCTION()
	void OnStartButtonClicked();

public:
	// 방장 여부 + 시작 가능 여부에 따라 버튼/안내문구 표시 전환
	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void UpdateHostUI(bool bIsHost, bool bCanStart);
	
};