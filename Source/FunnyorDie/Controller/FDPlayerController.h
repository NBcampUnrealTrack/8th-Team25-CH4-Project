// PlayerController.h

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FDPlayerController.generated.h"

UCLASS()
class FUNNYORDIE_API AFDPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	// 서버 → 술래 포획 팝업 띄우기
	UFUNCTION(Client, Reliable)
	void Client_ShowCapturePopup();

	// 술래 → 서버: 아웃 요청
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestOut();

	// 술래 → 서버: 봐주기 요청
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestSpare();
};