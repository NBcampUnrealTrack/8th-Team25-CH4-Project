// FDPlayerState.cpp

#include "PlayerState/FDPlayerState.h"
#include "Net/UnrealNetwork.h"

void AFDPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AFDPlayerState, RoleTag);
	DOREPLIFETIME(AFDPlayerState, bIsAlive);
	DOREPLIFETIME(AFDPlayerState, bIsInvincible);
}
