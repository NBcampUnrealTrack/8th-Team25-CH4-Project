// FDLobbyPlayerController.h

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FDLobbyPlayerController.generated.h"

class UFDLobbyWidget;

UCLASS()
class FUNNYORDIE_API AFDLobbyPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	// 클라이언트가 호출 -> 서버로 전달되는 RPC
	// 위젯의 Start 버튼 OnClicked에서 이 함수를 호출하면 됨
	UFUNCTION(Server, Reliable)
	void ServerRPC_RequestStartMatch();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;


	UPROPERTY(EditDefaultsOnly, Category = "Lobby")
	TSubclassOf<UFDLobbyWidget> LobbyWidgetClass;

	UPROPERTY()
	UFDLobbyWidget* LobbyWidget;

	UPROPERTY(EditDefaultsOnly, Category = "Lobby")
	int32 MinPlayersToStart = 3;

	FTimerHandle RefreshTimerHandle;
	void RefreshLobbyUI();
};