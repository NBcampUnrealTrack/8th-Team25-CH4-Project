// FDGameState.cpp

#include "GameState/FDGameState.h"
#include "Net/UnrealNetwork.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "PlayerState/FDPlayerState.h"

void AFDGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AFDGameState, CurrentPhase);
	DOREPLIFETIME(AFDGameState, AliveHiderCount);
	DOREPLIFETIME(AFDGameState, Winner);
	DOREPLIFETIME(AFDGameState, PhaseEndServerTime);
	DOREPLIFETIME(AFDGameState, HiderRankings);

}

void AFDGameState::OnRep_CurrentPhase()
{
	UE_LOG(LogTemp, Warning, TEXT("[GameState] CurrentPhase 복제 수신: %d"), (uint8)CurrentPhase);
	
	// 페이즈가 바뀔 때마다 phase UI를 현재 상태에 맞춰 갱신
	UpdatePhaseUI();
}

void AFDGameState::SetPhase(EMatchPhase NewPhase)
{
	CurrentPhase = NewPhase;
	OnRep_CurrentPhase();
}

void AFDGameState::OnRep_HiderRankings()
{
	OnRankingsUpdated.Broadcast();
}

float AFDGameState::GetRemainingPhaseTime() const
{
	// 서버에선 실제 월드 시간을, 클라에선 복제 보정된 서버 기준 시각을 돌려줌
	// 서버가 PhaseEndServerTime을 이 시계로 찍었으니 클라도 같은 시계로 빼면 기준이 맞음
	const float Remaining = PhaseEndServerTime - GetServerWorldTimeSeconds();

	// 페이즈가 이미 끝났거나(음수) 아직 세팅 전(0)이면 0으로
	return FMath::Max(Remaining, 0.f);
}

void AFDGameState::UpdatePhaseUI()
{
	// 위젯은 로컬 플레이어 화면에만
	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PC || !PC->IsLocalController()) return;

	// 정찰 안내 배너: Scouting일 때만 존재
	const bool bWantBanner = (CurrentPhase == EMatchPhase::Scouting);
	if (bWantBanner && !ScoutingBannerWidget && ScoutingBannerWidgetClass)
	{
		ScoutingBannerWidget = CreateWidget<UUserWidget>(PC, ScoutingBannerWidgetClass);
		if (ScoutingBannerWidget) ScoutingBannerWidget->AddToViewport();
	}
	else if (!bWantBanner && ScoutingBannerWidget)
	{
		ScoutingBannerWidget->RemoveFromParent();
		ScoutingBannerWidget = nullptr;
	}
	
	// -게임 시작 팝업
	// phase 조건으로 켜고 끄는 게 아니라 뜨는 순간 타이머로 스스로 제거
	if (CurrentPhase == EMatchPhase::InGame && !StartBannerWidget && StartBannerWidgetClass)
	{
		StartBannerWidget = CreateWidget<UUserWidget>(PC, StartBannerWidgetClass);
		if (StartBannerWidget)
		{
			StartBannerWidget->AddToViewport();

			// 3초 뒤 자동 제거
			GetWorldTimerManager().SetTimer(
				StartBannerTimerHandle,
				[this]()
				{
					if (StartBannerWidget)
					{
						StartBannerWidget->RemoveFromParent();
						StartBannerWidget = nullptr;
					}
				},
				3.f, false);
		}
	}

	// 카운트다운: Scouting 또는 InGame일 때 존재
	const bool bWantCountdown = (CurrentPhase == EMatchPhase::Scouting || CurrentPhase == EMatchPhase::InGame);
	if (bWantCountdown && !CountdownWidget && CountdownWidgetClass)
	{
		CountdownWidget = CreateWidget<UUserWidget>(PC, CountdownWidgetClass);
		if (CountdownWidget) CountdownWidget->AddToViewport();
	}
	else if (!bWantCountdown && CountdownWidget)
	{
		CountdownWidget->RemoveFromParent();
		CountdownWidget = nullptr;
	}
	
	// 순위표: GameOver일 때만 존재
	const bool bWantRanking = (CurrentPhase == EMatchPhase::GameOver);
	if (bWantRanking && !RankingWidget && RankingWidgetClass)
	{
		RankingWidget = CreateWidget<UUserWidget>(PC, RankingWidgetClass);
		if (RankingWidget) RankingWidget->AddToViewport();
	}
	else if (!bWantRanking && RankingWidget)
	{
		RankingWidget->RemoveFromParent();
		RankingWidget = nullptr;
	}
}