// FDLobbyGameMode.cpp

#include "GameMode/FDLobbyGameMode.h"
#include "Controller/FDLobbyPlayerController.h"
#include "PlayerState/FDPlayerState.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameStateBase.h"

// 여기서 하는 것 : 1. postlogin : 일단 들어온 사람들에게 playerstate 부여됨 (처음 들어온 사람 방장)
// 2. 중간에 나갈수도 있으니까 logout과 PromoteNewHost로 방어
// 3. TryStartMatch : LobbyPlayerController가 각각 부여되는데 거기에 serverRPC로 TryStartMatch 부를 수 있게 할 거임
// (UI 만들어서 OnClicked로 함수 호출되게) -> 그럼 TryStartMatch 안에서 방장인지 + 인원 충분한지 검증 후 게임 시작
// 만들어야 하는 UI : 방장에게 뜰 Start UI (사람 모이기 전엔 클릭 불가, 사람 모이면 클릭 가능하게) 
// + 대기자에게 뜰 UI 

AFDLobbyGameMode::AFDLobbyGameMode()
{
	PlayerStateClass = AFDPlayerState::StaticClass();
	PlayerControllerClass = AFDLobbyPlayerController::StaticClass();
	// 이 레벨에 접속하는 모든 플레이어는 AFDLobbyPlayerController를 받음
}

void AFDLobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	AFDPlayerState* FDPS = NewPlayer->GetPlayerState<AFDPlayerState>();
	if (!FDPS) return;

	if (!HostPlayerState.IsValid())
		// 아직 방장이 없으면(=첫 입장자) 이 사람을 방장으로 지정
	{
		HostPlayerState = FDPS;
		FDPS->bIsHost = true;
		
		// 디버깅 로그
		UE_LOG(LogTemp, Warning, TEXT("[Lobby] %s 방장으로 지정됨"), *FDPS->GetPlayerName());
	}

	// 디버깅 로그
	UE_LOG(LogTemp, Warning, TEXT("[Lobby] %s 입장, 현재 인원 %d"),
		*FDPS->GetPlayerName(), GameState->PlayerArray.Num());
}

void AFDLobbyGameMode::Logout(AController* Exiting)
{
	if (const APlayerState* ExitingPS = Exiting ? Exiting->GetPlayerState<APlayerState>() : nullptr)
	{
		if (ExitingPS == HostPlayerState.Get()) // 나간 사람이 방장이었으면 일단 reset (비움)
		{
			HostPlayerState.Reset();
		}
	}

	Super::Logout(Exiting);
	// PlayerArray에서 나간 사람을 빼줌 (언리얼 제공)
	// 방장 재지정은 이 이후에 해야함

	if (!HostPlayerState.IsValid())
	{
		PromoteNewHost();
	}
}

void AFDLobbyGameMode::PromoteNewHost() // 방장 재지정 코드
{
	if (GameState->PlayerArray.Num() == 0) return;

	APlayerState* NewHost = GameState->PlayerArray[0];
	if (AFDPlayerState* FDPS = Cast<AFDPlayerState>(NewHost))
	{
		FDPS->bIsHost = true;
		HostPlayerState = FDPS;
		
		// 디버깅 로그
		UE_LOG(LogTemp, Warning, TEXT("[Lobby] 방장 위임: %s"), *FDPS->GetPlayerName());
	}
}

void AFDLobbyGameMode::TryStartMatch(APlayerController* Requester)
{
	const AFDPlayerState* FDPS = Requester ? Requester->GetPlayerState<AFDPlayerState>() : nullptr;
	if (!FDPS || !FDPS->bIsHost)
		// FDPS가 null 이거나 FDPS가 ishost가 아니면 (즉 방장이 아니면 돌려보냄)
	{
		return;
	}

	if (GameState->PlayerArray.Num() < MinPlayersToStart)
		// 인원 부족하면 무시
	{
		return;
	}

	// 디버깅 로그
	UE_LOG(LogTemp, Warning, TEXT("[Lobby] 매치 시작! %s 로 이동"), *NextLevelName);
	GetWorld()->ServerTravel(NextLevelName);
}