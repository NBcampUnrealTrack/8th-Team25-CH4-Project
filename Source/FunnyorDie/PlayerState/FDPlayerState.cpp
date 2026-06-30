// FDPlayerState.cpp

#include "PlayerState/FDPlayerState.h"
#include "Net/UnrealNetwork.h"

void AFDPlayerState::OnRep_RoleTag() 
// roletag가 replication 될 때 호출되는 함수 
// gamemode에서 assignrole이 실행되고 roletag가 배정되면 자동으로 replication이 될테고
// 그게 각자 클라이언트로 잘 복제가 됐는지 확인해보려고 작성함 (로그가 많아서 화면에 띄움)
{
	if (GEngine)
	{
		FString RoleStr = RoleTag == EFDRole::Tagger ? TEXT("Tagger") :
				   RoleTag == EFDRole::Hider ? TEXT("Hider") : TEXT("None");

		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
			FString::Printf(TEXT("RoleTag 복제 수신: %s"), *RoleStr));
	}
}

void AFDPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AFDPlayerState, RoleTag);
	DOREPLIFETIME(AFDPlayerState, bIsAlive);
	DOREPLIFETIME(AFDPlayerState, bIsInvincible);
	DOREPLIFETIME(AFDPlayerState, bIsHost);
}
