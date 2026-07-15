// FDPlayerState.cpp
// ※ 원본 파일 부재로 복원된 버전임 - 상단 FDPlayerState.h 주석 참고

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
	DOREPLIFETIME(AFDPlayerState, CustomizationColors);
	DOREPLIFETIME(AFDPlayerState, PaintSnapshot);
	DOREPLIFETIME(AFDPlayerState, DeathServerTime);

}

void AFDPlayerState::OnRep_CustomizationColors()
{
	// 주의: 이 시점에 아직 Pawn이 스폰 안 됐을 수 있음
	// (롤 배정 직후처럼 PlayerState 복제가 캐릭터 스폰보다 먼저 도착하는 경우가 있어서 nullptr 체크 필수)
	if (ACharacter* MyPawn = GetPawn<ACharacter>())
	{
		if (UFDCustomizationComponent* CustomComp = MyPawn->FindComponentByClass<UFDCustomizationComponent>())
		{
			CustomComp->ApplyColorPreset(CustomizationColors);
		}
	}
}

void AFDPlayerState::OnRep_PaintSnapshot()
{
	if (ACharacter* MyPawn = GetPawn<ACharacter>())
	{
		if (UFDCustomizationComponent* CustomComp = MyPawn->FindComponentByClass<UFDCustomizationComponent>())
		{
			CustomComp->ApplyPaintSnapshot(PaintSnapshot);
		}
	}
}
