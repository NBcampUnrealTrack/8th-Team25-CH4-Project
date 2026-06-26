// FDGameState.cpp

#include "GameState/FDGameState.h"
#include "Net/UnrealNetwork.h"

void AFDGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AFDGameState, CurrentPhase);
	DOREPLIFETIME(AFDGameState, AliveHiderCount);
}
