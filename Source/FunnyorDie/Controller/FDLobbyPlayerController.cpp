// FDLobbyPlayerController.cpp

#include "Controller/FDLobbyPlayerController.h"
#include "GameMode/FDLobbyGameMode.h"
#include "PlayerState/FDPlayerState.h"
#include "UI/FDLobbyWidget.h"
#include "GameFramework/GameStateBase.h"
#include "Camera/CameraActor.h"
#include "Kismet/GameplayStatics.h"

void AFDLobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController() && LobbyWidgetClass)
	{
		LobbyWidget = CreateWidget<UFDLobbyWidget>(this, LobbyWidgetClass);
		LobbyWidget->AddToViewport();

		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(LobbyWidget->TakeWidget());
		SetInputMode(InputMode);
		// 입력 모드를 UI 전용으로 전환
		// 이게 없으면 마우스 클릭이 게임 월드 조작용으로 해석되려고 해서 버튼 클릭이 씹히거나 캐릭터가 같이 움직이는 등 이상 동작이 생길 수 있음

		bShowMouseCursor = true;
		// 기본적으로 게임 화면엔 마우스 커서가 안 보이니 이걸 켜줘야 버튼을 누를 수 있음

		GetWorldTimerManager().SetTimer(RefreshTimerHandle, this, &AFDLobbyPlayerController::RefreshLobbyUI, 1.0f, true);
		// 인원이 바뀌는거 감지하는 로직으로 나중에 바꾸겠음 지금은 일단 1초마다 함수 호출
		
		TArray<AActor*> FoundCams;
		UGameplayStatics::GetAllActorsOfClassWithTag(
			this, ACameraActor::StaticClass(), FName("LobbyCam"), FoundCams);

		if (FoundCams.Num() > 0)
		{
			SetViewTarget(FoundCams[0]);
		}
		
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[Lobby] LobbyCam 태그 카메라를 못 찾음"));
		}
	}
}

void AFDLobbyPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsLocalController())
	{
		SetInputMode(FInputModeGameOnly());
		bShowMouseCursor = false;
	}
	
	GetWorldTimerManager().ClearTimer(RefreshTimerHandle);

	Super::EndPlay(EndPlayReason);
}

void AFDLobbyPlayerController::RefreshLobbyUI()
// 1초마다 호출되는 함수
{
	if (!LobbyWidget) return;

	const AFDPlayerState* FDPS = GetPlayerState<AFDPlayerState>();
	if (!FDPS) return;

	const AGameStateBase* GS = GetWorld()->GetGameState<AGameStateBase>();
	// PlayerController엔 GameMode처럼 GameState 멤버가 따로 없어서 GetWorld()를 거쳐서 직접 가져와야 함
	if (!GS) return;
	
	const int32 CurrentCount = GS->PlayerArray.Num();
	// GameState->PlayerArray: 현재 접속해있는 모든 플레이어의 PlayerState 목록

	const bool bCanStart = CurrentCount >= MinPlayersToStart;
	// 최소 인원 채웠는지 여부 -> 이 값으로 Start 버튼을 활성화할지 결정

	LobbyWidget->UpdateHostUI(FDPS->bIsHost, bCanStart);
	// 위젯한테 방장인지, 시작 가능한지 전달
	// 실제로 버튼 보이기/숨기기, 활성화/비활성화는 위젯 내부(LobbyWidget 파일의 UpdateHostUI 함수)에서 처리
}

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