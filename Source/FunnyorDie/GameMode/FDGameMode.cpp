// FDGameMode.cpp

#include "GameMode/FDGameMode.h"
#include "PlayerState/FDPlayerState.h"   
#include "GameFramework/Character.h"     
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameStateBase.h"

AFDGameMode::AFDGameMode()
{
	PlayerStateClass = AFDPlayerState::StaticClass();
	DefaultPawnClass = nullptr; // PostLogin 내부 함수보면 처음에 정해진 게 없을 때 pawn 스폰할 때 defaultpawnclass로 설정하는데 
	// tag 배정되지 않았을 땐 character 배정 안 되도록 nullptr로 막아둠 (tag별로 캐릭터가 다르니까)
}

void AFDGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	// 디버깅 로그
	UE_LOG(LogTemp, Warning, TEXT("[GameMode] PostLogin: %s 접속, 현재 인원 %d"),
		*NewPlayer->GetName(), GameState->PlayerArray.Num());
	
	// 디버그용임 실제 start 버튼 만들기 전까지 자동으로 start (5초 후 자동 시작)
	GetWorldTimerManager().SetTimer(PhaseTimerHandle, this, &AFDGameMode::AssignRoles, 5.f, false);
}

UClass* AFDGameMode::GetDefaultPawnClassForController_Implementation(AController* InController) 
// 이건 어떤 pawn을 default로 할지 정하는 언리얼 제공 함수임 
{
	if (const AFDPlayerState* FDPS = InController ? InController->GetPlayerState<AFDPlayerState>() : nullptr)
		// InController가 유효하면 그 컨트롤러가 가진 playerState를 AFDPlayerState 타입으로 꺼내옴
	{
		if (FDPS->RoleTag == EFDRole::Tagger && TaggerClass) return TaggerClass;
		// RoleTag가 Tagger로 정해져 있고 TaggerClass도 에디터에서 할당되어 있으면 그 클래스를 리턴
		
		if (FDPS->RoleTag == EFDRole::Hider && HiderClass) return HiderClass;
		// 마찬가지
	}
	
	return Super::GetDefaultPawnClassForController_Implementation(InController);
	// 위 조건 둘 다 해당 안 되면 부모 클래스의 기본 동작을 따름
	// DefaultPawnClass(=nullptr)를 리턴해서 스폰 안됨
}

void AFDGameMode::StartWarmup()
{
}

void AFDGameMode::AssignRoles()
{
	TArray<APlayerState*> Players = GameState->PlayerArray;
	if (Players.Num() == 0) return;

	const int32 TaggerIndex = FMath::RandRange(0, Players.Num() - 1);
	// 0 ~ (인원수-1) 중 랜덤 인덱스 하나 뽑음 -> 이 인덱스 위치 사람이 술래

	UE_LOG(LogTemp, Warning, TEXT("[GameMode] AssignRoles 시작, 인원 %d"), Players.Num());
	
	for (int32 i = 0; i < Players.Num(); ++i)
		// RoleTag 복제
	{
		AFDPlayerState* FDPS = Cast<AFDPlayerState>(Players[i]);
		if (!FDPS) continue;

		FDPS->RoleTag = (i == TaggerIndex) ? EFDRole::Tagger : EFDRole::Hider;
		UE_LOG(LogTemp, Warning, TEXT("[서버] %s RoleTag 설정: %d"), *FDPS->GetName(), (uint8)FDPS->RoleTag);

		UE_LOG(LogTemp, Warning, TEXT("[GameMode] %s -> %s"),
			*FDPS->GetPlayerName(), FDPS->RoleTag == EFDRole::Tagger ? TEXT("Tagger") : TEXT("Hider"));
	}
	
	// Tag 정해진 뒤에 spawn 재요청
	for (APlayerState* PS : Players)
	{
		if (AController* Controller = PS->GetOwningController())
		{
			RestartPlayer(Controller); // postlogin 내부에 있음
		}
	}
}

void AFDGameMode::StartScouting()
{
}

void AFDGameMode::StartInGame()
{
}

void AFDGameMode::EndMatch()
{
}

void AFDGameMode::RequestCaptureJudgement()
{
}

void AFDGameMode::ResolveCapture()
{
}
