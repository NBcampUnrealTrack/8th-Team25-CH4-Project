// FDSoloGameMode.cpp

#include "SoloTest/FDSoloGameMode.h"
#include "SoloTest/FDSoloBotController.h"
#include "GameState/FDGameState.h"
#include "PlayerState/FDPlayerState.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameStateBase.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AFDSoloGameMode::AFDSoloGameMode()
{
	bUseSeamlessTravel = false;

	BotControllerClass = AFDSoloBotController::StaticClass();

	/*
	팀의 BP_FDGameMode에서 손으로 지정하던 값들을 여기서 코드로 물어온다.
	이 클래스 전용 BP를 따로 만들 필요가 없어진다.
	경로가 하나라도 틀리면 아래 로그가 뜨니 그때 실제 경로로 고치면 된다.
	*/

	static ConstructorHelpers::FClassFinder<AGameStateBase>
		GameStateBP(TEXT("/Game/FunnyorDie/Blueprints/GameState/BP_GameState"));
	if (GameStateBP.Succeeded())
	{
		GameStateClass = GameStateBP.Class;
	}
	else
	{
		// GameState가 AFDGameState가 아니면 페이즈 전환이 통째로 죽는다
		GameStateClass = AFDGameState::StaticClass();
		UE_LOG(LogTemp, Error, TEXT("[SoloTest] BP_GameState를 못 찾음 - 페이즈 UI가 안 뜰 수 있음"));
	}

	static ConstructorHelpers::FClassFinder<APlayerController>
		PlayerControllerBP(TEXT("/Game/FunnyorDie/Blueprints/Controller/BP_FDPlayerController"));
	if (PlayerControllerBP.Succeeded())
	{
		PlayerControllerClass = PlayerControllerBP.Class;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[SoloTest] BP_FDPlayerController를 못 찾음 - 조작이 안 됨"));
	}

	static ConstructorHelpers::FClassFinder<ACharacter>
		TaggerBP(TEXT("/Game/FunnyorDie/Blueprints/Character/BP_FDTaggerCharacter"));
	if (TaggerBP.Succeeded())
	{
		TaggerClass = TaggerBP.Class;
	}

	static ConstructorHelpers::FClassFinder<ACharacter>
		HiderBP(TEXT("/Game/FunnyorDie/Blueprints/Character/BP_FDHiderCharacter"));
	if (HiderBP.Succeeded())
	{
		HiderClass = HiderBP.Class;
	}

	if (!TaggerBP.Succeeded() || !HiderBP.Succeeded())
	{
		UE_LOG(LogTemp, Error, TEXT("[SoloTest] 캐릭터 BP를 못 찾음 - 폰이 안 나옴"));
	}

	// 없어도 코드 기본값(정찰 60초, 본게임 300초 등)으로 굴러가긴 함
	static ConstructorHelpers::FObjectFinder<UDataTable>
		BalanceDT(TEXT("/Game/FunnyorDie/Data/DT_MatchBalanceSettings"));
	if (BalanceDT.Succeeded())
	{
		BalanceDataTable = BalanceDT.Object;
	}
}

void AFDSoloGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	// open DemoMaps?game=...?role=tagger 처럼 넘어온 값을 읽음
	const FString RoleOption = UGameplayStatics::ParseOption(Options, TEXT("role"));

	if (RoleOption.Equals(TEXT("tagger"), ESearchCase::IgnoreCase))
	{
		LocalPlayerRole = EFDRole::Tagger;
	}
	else if (RoleOption.Equals(TEXT("hider"), ESearchCase::IgnoreCase))
	{
		LocalPlayerRole = EFDRole::Hider;
	}
	else
	{
		LocalPlayerRole = (DefaultRole == EFDRole::None) ? EFDRole::Hider : DefaultRole;
	}

	UE_LOG(LogTemp, Warning, TEXT("[SoloTest] 이번 판 역할: %s"),
		LocalPlayerRole == EFDRole::Tagger ? TEXT("술래") : TEXT("하이더"));
}

void AFDSoloGameMode::StartPlay()
{
	/*
	Super가 아니라 AGameModeBase를 직접 부른다.
	부모 AFDGameMode::StartPlay는 인원 대기 폴링 타이머를 거는데 혼자 하는 판에서는
	그 15초가 통째로 낭비된다. 조부모까지만 올라가서 기본 초기화만 돌린다.
	*/
	AGameModeBase::StartPlay();

	// 부모와 상태를 맞춰둠 (테스트 중 난입은 없지만 PreLogin 분기와 일관성 유지)
	bRosterLocked = true;

	const int32 BotCount = (LocalPlayerRole == EFDRole::Tagger) ? HiderBotCount : TaggerBotCount;
	SpawnBots(BotCount);

	/*
	봇 PlayerState는 컨트롤러 스폰 중에 PlayerArray에 등록되지만 등록 시점이
	초기화 순서에 따라 미묘하게 달라질 수 있다.
	한 틱 미뤄서 PlayerArray가 확정된 뒤에 역할을 배정한다.
	*/
	GetWorldTimerManager().SetTimerForNextTick(this, &AFDSoloGameMode::AssignSoloRoles);
}

void AFDSoloGameMode::SpawnBots(int32 Count)
{
	if (Count <= 0 || !BotControllerClass) return;

	UWorld* World = GetWorld();
	if (!World) return;

	for (int32 i = 0; i < Count; ++i)
	{
		FActorSpawnParameters SpawnInfo;
		SpawnInfo.ObjectFlags |= RF_Transient;
		SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AController* Bot = World->SpawnActor<AController>(
			BotControllerClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnInfo);

		if (!Bot) continue;

		// bWantsPlayerState 경로가 안 탔을 때를 위한 보험
		if (!Bot->PlayerState)
		{
			Bot->InitPlayerState();
		}

		if (APlayerState* BotPS = Bot->PlayerState)
		{
			// 순위표에 그대로 찍히는 이름
			BotPS->SetPlayerName(FString::Printf(TEXT("BOT_%d"), i + 1));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[SoloTest] 봇 PlayerState 생성 실패"));
		}

		SpawnedBots.Add(Bot);
	}

	UE_LOG(LogTemp, Warning, TEXT("[SoloTest] 봇 %d기 생성"), SpawnedBots.Num());
}

void AFDSoloGameMode::AssignSoloRoles()
{
	if (AFDGameState* FDGameState = GetGameState<AFDGameState>())
	{
		FDGameState->SetPhase(EMatchPhase::AssignRole);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[SoloTest] GameState가 AFDGameState가 아님 - 진행 불가"));
		return;
	}

	if (!GameState || GameState->PlayerArray.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("[SoloTest] PlayerArray가 비어 있음"));
		return;
	}

	// 사람은 고른 역할, 봇은 그 반대
	const EFDRole BotRole = (LocalPlayerRole == EFDRole::Tagger) ? EFDRole::Hider : EFDRole::Tagger;

	for (APlayerState* PS : GameState->PlayerArray)
	{
		AFDPlayerState* FDPS = Cast<AFDPlayerState>(PS);
		if (!FDPS) continue;

		// PlayerController가 붙어 있으면 사람, 아니면 봇
		const bool bIsHuman = Cast<APlayerController>(FDPS->GetOwningController()) != nullptr;

		FDPS->RoleTag = bIsHuman ? LocalPlayerRole : BotRole;

		UE_LOG(LogTemp, Warning, TEXT("[SoloTest] %s -> %s"),
			*FDPS->GetPlayerName(),
			FDPS->RoleTag == EFDRole::Tagger ? TEXT("Tagger") : TEXT("Hider"));
	}

	/*
	역할이 정해진 뒤에 폰을 스폰한다.
	부모의 GetDefaultPawnClassForController_Implementation이 PlayerState의 RoleTag를 보고
	TaggerClass / HiderClass 중 하나를 고르기 때문에 이 순서가 바뀌면 폰이 안 나온다.
	봇 컨트롤러도 PlayerState를 들고 있어서 같은 경로를 그대로 탄다.
	*/
	for (APlayerState* PS : GameState->PlayerArray)
	{
		if (AController* OwningController = PS->GetOwningController())
		{
			RestartPlayer(OwningController);
		}
	}

	/*
	StartScouting은 부모의 protected 함수라 그대로 재사용한다.
	멤버 함수 포인터를 SetTimer에 바로 넘기면 UserClass 추론이 부모와 자식으로 갈려
	컴파일이 깨지므로 람다로 한 겹 감쌌다.
	*/
	GetWorldTimerManager().SetTimer(
		PhaseTimerHandle,
		FTimerDelegate::CreateLambda([this]() { StartScouting(); }),
		1.f, false);
}
