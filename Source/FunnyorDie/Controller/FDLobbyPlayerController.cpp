// FDLobbyPlayerController.cpp

#include "Controller/FDLobbyPlayerController.h"
#include "GameMode/FDLobbyGameMode.h"

void AFDLobbyPlayerController::ServerRPC_RequestStartMatch_Implementation()
{
	if (!HasAuthority())
	{
		return;
	}
	
	if (AFDLobbyGameMode* LobbyGM = GetWorld()->GetAuthGameMode<AFDLobbyGameMode>())
	{
		LobbyGM->TryStartMatch(this);
	}
}