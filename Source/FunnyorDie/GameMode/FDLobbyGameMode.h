// FDLobbyGameMode.h

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FDLobbyGameMode.generated.h"

UCLASS()
class FUNNYORDIE_API AFDLobbyGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFDLobbyGameMode();

	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	
	// Requester가 진짜 방장인지 인원이 충분한지 여기서 검증함
	void TryStartMatch(APlayerController* Requester);

protected:
	// 최소 시작 인원 (술래 1 + 숨는사람 최소 2 = 기본 3명)
	UPROPERTY(EditDefaultsOnly, Category = "Lobby")
	int32 MinPlayersToStart = 3;

	// Start 성공 시 이동할 다음 레벨 이름
	UPROPERTY(EditDefaultsOnly, Category = "Lobby")
	FString NextLevelName = TEXT("DemoMaps");

	// 현재 방장 - 서버에서만 의미 있음 (클라이언트는 PlayerState.bIsHost로 판단)
	TWeakObjectPtr<APlayerState> HostPlayerState;

	// 방장이 나갔을 때 남은 사람 중 한명에게 방장 위임
	void PromoteNewHost();
};