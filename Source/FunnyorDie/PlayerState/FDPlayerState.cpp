// FDPlayerState.cpp

#include "PlayerState/FDPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"
#include "Customization/FDCustomizationComponent.h"

void AFDPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFDPlayerState, RoleTag);
	DOREPLIFETIME(AFDPlayerState, bIsHost);
	DOREPLIFETIME(AFDPlayerState, bIsAlive);
	DOREPLIFETIME(AFDPlayerState, ColorPresetIndex);
	DOREPLIFETIME(AFDPlayerState, DeathServerTime);

}

void AFDPlayerState::OnRep_ColorPresetIndex()
{
	/*
	주의: 이 시점에 아직 Pawn이 스폰 안 됐을 수 있다.
	역할 배정 직후처럼 PlayerState 복제가 캐릭터 스폰보다 먼저 도착하는 경우가 있어서
	nullptr 체크가 필수다. 그 경우는 컴포넌트의 BeginPlay가 PlayerState를 역으로 읽어
	스스로 적용하므로 여기서 놓쳐도 결과는 같다.
	*/
	if (ACharacter* MyPawn = GetPawn<ACharacter>())
	{
		if (UFDCustomizationComponent* CustomComp = MyPawn->FindComponentByClass<UFDCustomizationComponent>())
		{
			CustomComp->ApplyPresetIndex(ColorPresetIndex);
		}
	}
}
