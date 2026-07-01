// FDGameState.cpp

#include "GameState/FDGameState.h"
#include "Net/UnrealNetwork.h"


void AFDGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AFDGameState, CurrentPhase);
	DOREPLIFETIME(AFDGameState, AliveHiderCount);
}

void AFDGameState::OnRep_CurrentPhase()
{
	UE_LOG(LogTemp, Warning, TEXT("[GameState] CurrentPhase 복제 수신: %d"), (uint8)CurrentPhase);
}
