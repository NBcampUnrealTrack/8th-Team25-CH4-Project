// FDGameMode.cpp

#include "GameMode/FDGameMode.h"
#include "PlayerState/FDPlayerState.h"   
#include "GameFramework/Character.h"     
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameStateBase.h"
#include "GameState/FDGameState.h"
#include "Character/FDTaggerCharacter.h"
#include "Controller/FDPlayerController.h"
#include "Engine/DataTable.h"
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"    
#include "GameInstance/FDGameInstance.h"

AFDGameMode::AFDGameMode()
{
	PlayerStateClass = AFDPlayerState::StaticClass();
	PlayerControllerClass = AFDPlayerController::StaticClass(); // 인게임 컨트롤러 클래스 지정
	DefaultPawnClass = nullptr; 
	bUseSeamlessTravel = true;
}

void AFDGameMode::HandleSeamlessTravelPlayer(AController*& C)
{
	// 로비 컨트롤러 등 다른 타입의 PlayerController로 넘어온 경우, 인게임용 PlayerController로 Swap
	APlayerController* OldPC = Cast<APlayerController>(C);
	if (OldPC && !OldPC->IsA(PlayerControllerClass))
	{
		FActorSpawnParameters SpawnInfo;
		SpawnInfo.Instigator = OldPC->GetInstigator();
		
		APlayerController* NewPC = GetWorld()->SpawnActor<APlayerController>(PlayerControllerClass, SpawnInfo);
		if (NewPC)
		{
			SwapPlayerControllers(OldPC, NewPC);
			C = NewPC;
		}
	}

	Super::HandleSeamlessTravelPlayer(C);
}

void AFDGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
}

const FMatchBalanceSettings* AFDGameMode::GetBalanceSettings() const
{
	if (!BalanceDataTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] 밸런스 데이터 테이블이 할당되지 않음"));
		return nullptr;
	}

	return BalanceDataTable->FindRow<FMatchBalanceSettings>(
		TEXT("Default"), TEXT("게임모드 밸런스 설정 조회")
	);
}

UClass* AFDGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	if (const AFDPlayerState* FDPS = InController ? InController->GetPlayerState<AFDPlayerState>() : nullptr)
	{
		if (FDPS->RoleTag == EFDRole::Tagger && TaggerClass) return TaggerClass;
		if (FDPS->RoleTag == EFDRole::Hider && HiderClass) return HiderClass;
	}

	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

void AFDGameMode::StartPlay()
{
	Super::StartPlay();

	// 로비에서 실어보낸 목표 인원 회수
	if (const UFDGameInstance* GI = GetGameInstance<UFDGameInstance>())
	{
		ExpectedPlayerCount = GI->ExpectedPlayerCount;
	}
	UE_LOG(LogTemp, Warning, TEXT("[GameMode] 목표 인원 %d명 — 전원 재접속 대기 시작"), ExpectedPlayerCount);

	PlayerWaitElapsed = 0.f;

	GetWorldTimerManager().SetTimer(
		WaitTimerHandle, this, &AFDGameMode::WaitForPlayers,
		PlayerWaitInterval, true);
}

void AFDGameMode::WaitForPlayers()
{
	PlayerWaitElapsed += PlayerWaitInterval;

	const int32 Current = GameState->PlayerArray.Num();
	const bool bEveryoneHere = (ExpectedPlayerCount > 0) && (Current >= ExpectedPlayerCount);
	const bool bTimedOut     = (PlayerWaitElapsed >= MaxPlayerWaitTime);

	if (!bEveryoneHere && !bTimedOut)
	{
		UE_LOG(LogTemp, Verbose, TEXT("[GameMode] 대기 중... %d/%d"), Current, ExpectedPlayerCount);
		return;
	}
	
	if (bEveryoneHere)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] 전원 %d명 집합 완료 — 매치 시작"), Current);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] 대기 타임아웃 — %d/%d명으로 강제 시작"), Current, ExpectedPlayerCount);
	}

	bRosterLocked = true;
	GetWorldTimerManager().ClearTimer(WaitTimerHandle);
	
	AssignRoles();
}

void AFDGameMode::PreLogin(const FString& Options, const FString& Address,
						   const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	if (bRosterLocked)
	{
		ErrorMessage = TEXT("Match already in progress");
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] 매치 진행 중 — 난입 거부: %s"), *Address);
		return;
	}

	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
}

void AFDGameMode::AssignRoles()
{
	if (AFDGameState* FDGameState = GetGameState<AFDGameState>())
	{
		FDGameState->SetPhase(EMatchPhase::AssignRole);
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] AssignRole 단계 시작"));
	}

	TArray<APlayerState*> Players = GameState->PlayerArray;
	if (Players.Num() == 0) return;

	const int32 TaggerIndex = FMath::RandRange(0, Players.Num() - 1);

	UE_LOG(LogTemp, Warning, TEXT("[GameMode] AssignRoles 시작, 인원 %d"), Players.Num());

	for (int32 i = 0; i < Players.Num(); ++i)
	{
		AFDPlayerState* FDPS = Cast<AFDPlayerState>(Players[i]);
		if (!FDPS) continue;

		FDPS->RoleTag = (i == TaggerIndex) ? EFDRole::Tagger : EFDRole::Hider;
		UE_LOG(LogTemp, Warning, TEXT("[서버] %s RoleTag 설정: %d"), *FDPS->GetName(), (uint8)FDPS->RoleTag);

		UE_LOG(LogTemp, Warning, TEXT("[GameMode] %s -> %s"),
			*FDPS->GetPlayerName(), FDPS->RoleTag == EFDRole::Tagger ? TEXT("Tagger") : TEXT("Hider"));
	}

	for (APlayerState* PS : Players)
	{
		if (AController* Controller = PS->GetOwningController())
		{
			RestartPlayer(Controller);
		}
	}

	GetWorldTimerManager().SetTimer(PhaseTimerHandle, this, &AFDGameMode::StartScouting, 1.f, false);
}

void AFDGameMode::StartScouting()
{
	AFDGameState* FDGameState = GetGameState<AFDGameState>();
	if (!FDGameState) return;

	FDGameState->SetPhase(EMatchPhase::Scouting);
	UE_LOG(LogTemp, Warning, TEXT("[GameMode] Scouting 단계 시작"));

	for (APlayerState* PS : GameState->PlayerArray)
	{
		const AFDPlayerState* FDPS = Cast<AFDPlayerState>(PS);
		if (!FDPS || FDPS->RoleTag != EFDRole::Tagger) continue;

		if (AFDTaggerCharacter* TaggerChar = Cast<AFDTaggerCharacter>(FDPS->GetPawn()))
		{
			TaggerChar->SetScoutingMode(true);
		}
	}

	float ScoutTime = 60.f;
	if (const FMatchBalanceSettings* Settings = GetBalanceSettings())
	{
		ScoutTime = Settings->ScoutPhaseTime;
	}

	FDGameState->PhaseEndServerTime = FDGameState->GetServerWorldTimeSeconds() + ScoutTime;
	
	GetWorldTimerManager().SetTimer(PhaseTimerHandle, this, &AFDGameMode::StartInGame, ScoutTime, false);
}

void AFDGameMode::StartInGame()
{
	for (APlayerState* PS : GameState->PlayerArray)
	{
		const AFDPlayerState* FDPS = Cast<AFDPlayerState>(PS);
		if (!FDPS || FDPS->RoleTag != EFDRole::Tagger) continue;

		if (AFDTaggerCharacter* TaggerChar = Cast<AFDTaggerCharacter>(FDPS->GetPawn()))
		{
			TaggerChar->SetScoutingMode(false);
		}
	}

	AFDGameState* FDGameState = GetGameState<AFDGameState>();
	if (!FDGameState) return;

	TeleportPlayersToStarts();
	
	FDGameState->SetPhase(EMatchPhase::InGame);
	UE_LOG(LogTemp, Warning, TEXT("[GameMode] InGame 단계 시작"));

	int32 HiderCount = 0;
	for (APlayerState* PS : GameState->PlayerArray)
	{
		if (const AFDPlayerState* FDPS = Cast<AFDPlayerState>(PS))
		{
			if (FDPS->RoleTag == EFDRole::Hider)
			{
				++HiderCount;
			}
		}
	}
	FDGameState->AliveHiderCount = HiderCount;

	float GameTimeLimit = 300.f;
	if (const FMatchBalanceSettings* Settings = GetBalanceSettings())
	{
		GameTimeLimit = Settings->MainGameTimeLimit;
	}

	FDGameState->PhaseEndServerTime = FDGameState->GetServerWorldTimeSeconds() + GameTimeLimit;
	
	GetWorldTimerManager().SetTimer(PhaseTimerHandle, this, &AFDGameMode::EndMatch, GameTimeLimit, false);
}

void AFDGameMode::EndMatch()
{
	AFDGameState* FDGameState = GetGameState<AFDGameState>();
	if (!FDGameState) return;

	FDGameState->Winner = (FDGameState->AliveHiderCount <= 0)
		? EMatchWinner::Tagger
		: EMatchWinner::Hider;

	FinalizeHiderRanking();
	FDGameState->SetPhase(EMatchPhase::GameOver);
	
	for (APlayerState* PS : GameState->PlayerArray)
	{
		if (AFDPlayerController* FDPC = Cast<AFDPlayerController>(PS->GetOwningController()))
		{
			FDPC->Client_LockMovement();
		}
	}

	GetWorldTimerManager().ClearTimer(PhaseTimerHandle);

	UE_LOG(LogTemp, Warning, TEXT("[GameMode] 게임 종료 - 승자: %s"),
		FDGameState->Winner == EMatchWinner::Tagger ? TEXT("Tagger") : TEXT("Hider"));
}

void AFDGameMode::TeleportPlayersToStarts()
{
	TArray<APlayerStart*> TaggerStarts;
	TArray<APlayerStart*> HiderStarts;

	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		APlayerStart* Start = *It;
		if (!Start) continue;

		if (Start->PlayerStartTag == TEXT("Tagger"))
		{
			TaggerStarts.Add(Start);
		}
		else if (Start->PlayerStartTag == TEXT("Hider"))
		{
			HiderStarts.Add(Start);
		}
	}

	int32 TaggerIdx = 0;
	int32 HiderIdx = 0;

	for (APlayerState* PS : GameState->PlayerArray)
	{
		const AFDPlayerState* FDPS = Cast<AFDPlayerState>(PS);
		if (!FDPS) continue;

		APawn* Pawn = FDPS->GetPawn();
		if (!Pawn) continue;

		APlayerStart* Target = nullptr;

		if (FDPS->RoleTag == EFDRole::Tagger && TaggerStarts.Num() > 0)
		{
			Target = TaggerStarts[TaggerIdx % TaggerStarts.Num()];
			++TaggerIdx;
		}
		else if (FDPS->RoleTag == EFDRole::Hider && HiderStarts.Num() > 0)
		{
			Target = HiderStarts[HiderIdx % HiderStarts.Num()];
			++HiderIdx;
		}

		if (!Target) continue;

		Pawn->SetActorLocation(Target->GetActorLocation(), false);
		Pawn->SetActorRotation(Target->GetActorRotation());
	}
}

void AFDGameMode::RequestCaptureJudgement(class AFDTaggerCharacter* TaggerCharacter, ACharacter* HiderCharacter)
{
	UE_LOG(LogTemp, Warning, TEXT("RequestCaptureJudgement 불림"));

	if (!TaggerCharacter || !HiderCharacter) return;

	AFDGameState* FDGameState = GetGameState<AFDGameState>();
	if (!FDGameState || FDGameState->CurrentPhase != EMatchPhase::InGame) return;

	TaggerCharacter->StartCaptureSequence(HiderCharacter);
}

APawn* AFDGameMode::FindLivingViewTarget() const
{
	APawn* TaggerPawn = nullptr;

	for (APlayerState* PS : GameState->PlayerArray)
	{
		const AFDPlayerState* FDPS = Cast<AFDPlayerState>(PS);
		if (!FDPS) continue;

		if (FDPS->RoleTag == EFDRole::Tagger)
		{
			TaggerPawn = FDPS->GetPawn();
			continue;
		}

		if (FDPS->RoleTag == EFDRole::Hider && FDPS->bIsAlive)
		{
			if (APawn* HiderPawn = FDPS->GetPawn())
			{
				return HiderPawn;
			}
		}
	}

	return TaggerPawn;
}

void AFDGameMode::UpdateSpectators()
{
	APawn* ViewTarget = FindLivingViewTarget();
	if (!ViewTarget) return;

	for (APlayerState* PS : GameState->PlayerArray)
	{
		const AFDPlayerState* FDPS = Cast<AFDPlayerState>(PS);
		if (!FDPS) continue;

		if (FDPS->bIsAlive) continue;

		if (AFDPlayerController* FDPC = Cast<AFDPlayerController>(FDPS->GetOwningController()))
		{
			if (FDPC->GetPawn() == ViewTarget) continue;

			FDPC->Client_SetSpectateTarget(ViewTarget);
		}
	}
}

void AFDGameMode::ResolveCapture(ACharacter* HiderCharacter, bool bWasCaptured)
{
	if (!HiderCharacter || !bWasCaptured) return;

	AFDGameState* FDGameState = GetGameState<AFDGameState>();
	if (!FDGameState) return;

	if (APlayerState* HiderPS = HiderCharacter->GetPlayerState())
	{
		if (AFDPlayerState* FDHiderPS = Cast<AFDPlayerState>(HiderPS))
		{
			FDHiderPS->bIsAlive = false;
			FDHiderPS->DeathServerTime = FDGameState->GetServerWorldTimeSeconds();
		}
	}

	--FDGameState->AliveHiderCount;

	UpdateSpectators();

	HiderCharacter->SetActorHiddenInGame(true);
	HiderCharacter->SetActorEnableCollision(false);

	if (AFDPlayerController* DeadPC = Cast<AFDPlayerController>(HiderCharacter->GetController()))
	{
		DeadPC->Client_LockMovement();
	}

	if (FDGameState->AliveHiderCount <= 0)
	{
		EndMatch();
	}
}

void AFDGameMode::FinalizeHiderRanking()
{
	AFDGameState* FDGameState = GetGameState<AFDGameState>();
	if (!FDGameState) return;

	float GameTimeLimit = 300.f;
	if (const FMatchBalanceSettings* Settings = GetBalanceSettings())
	{
		GameTimeLimit = Settings->MainGameTimeLimit;
	}
	const float GameStartTime = FDGameState->PhaseEndServerTime - GameTimeLimit;

	TArray<AFDPlayerState*> Hiders;
	for (APlayerState* PS : GameState->PlayerArray)
	{
		if (AFDPlayerState* FDPS = Cast<AFDPlayerState>(PS))
		{
			if (FDPS->RoleTag == EFDRole::Hider)
			{
				Hiders.Add(FDPS);
			}
		}
	}

	Hiders.Sort([](const AFDPlayerState& A, const AFDPlayerState& B)
	{
		const bool bAAlive = (A.DeathServerTime < 0.f);
		const bool bBAlive = (B.DeathServerTime < 0.f);

		if (bAAlive != bBAlive) return bAAlive;
		if (bAAlive && bBAlive) return false;
		return A.DeathServerTime > B.DeathServerTime;
	});

	FDGameState->HiderRankings.Empty();
	for (int32 i = 0; i < Hiders.Num(); ++i)
	{
		AFDPlayerState* FDPS = Hiders[i];

		FHiderRankEntry Entry;
		Entry.PlayerName = FDPS->GetPlayerName();
		Entry.Rank = i + 1;

		Entry.SurvivalTime = (FDPS->DeathServerTime < 0.f)
			? GameTimeLimit
			: (FDPS->DeathServerTime - GameStartTime);

		FDGameState->HiderRankings.Add(Entry);
	}

	FDGameState->OnRep_HiderRankings();
}