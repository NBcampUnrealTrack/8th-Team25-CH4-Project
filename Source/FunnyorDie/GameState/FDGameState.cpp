// FDGameState.cpp

#include "GameState/FDGameState.h"
#include "Net/UnrealNetwork.h"


void AFDGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AFDGameState, CurrentPhase);
	DOREPLIFETIME(AFDGameState, AliveHiderCount);
	DOREPLIFETIME(AFDGameState, Winner);
	DOREPLIFETIME(AFDGameState, PhaseEndServerTime);
}

void AFDGameState::OnRep_CurrentPhase()
{
	UE_LOG(LogTemp, Warning, TEXT("[GameState] CurrentPhase 복제 수신: %d"), (uint8)CurrentPhase);
}

float AFDGameState::GetRemainingPhaseTime() const
{
	// 서버에선 실제 월드 시간을, 클라에선 복제 보정된 서버 기준 시각을 돌려줌
	// 서버가 PhaseEndServerTime을 이 시계로 찍었으니 클라도 같은 시계로 빼면 기준이 맞음
	const float Remaining = PhaseEndServerTime - GetServerWorldTimeSeconds();

	// 페이즈가 이미 끝났거나(음수) 아직 세팅 전(0)이면 0으로
	return FMath::Max(Remaining, 0.f);
}