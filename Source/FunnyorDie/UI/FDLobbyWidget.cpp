// FDLobbyWidget.cpp

#include "UI/FDLobbyWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Controller/FDLobbyPlayerController.h"

void UFDLobbyWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (StartButton)
	{
		StartButton->OnClicked.AddDynamic(this, &UFDLobbyWidget::OnStartButtonClicked);
	}
}

void UFDLobbyWidget::OnStartButtonClicked()
{
	// 이 버튼을 누르면 위젯을 들고 있는 PlayerController를 가져와서 RPC 호출
	if (AFDLobbyPlayerController* LobbyPC = Cast<AFDLobbyPlayerController>(GetOwningPlayer()))
	{
		LobbyPC->ServerRPC_RequestStartMatch();
	}
}

void UFDLobbyWidget::UpdateHostUI(bool bIsHost, bool bCanStart)
{
	if (StartButton)
	{
		StartButton->SetVisibility(bIsHost ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		StartButton->SetIsEnabled(bCanStart);
		// 방장이 아니면 버튼 자체를 안 보이게 함
		// 방장이어도 인원 부족하면 보이긴 하되 비활성화(클릭은 막되 위치는 유지)
	}

	if (WaitingText)
	{
		WaitingText->SetVisibility(bIsHost ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
}