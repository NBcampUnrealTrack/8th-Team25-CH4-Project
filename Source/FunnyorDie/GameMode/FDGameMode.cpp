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

AFDGameMode::AFDGameMode()
{
	PlayerStateClass = AFDPlayerState::StaticClass();
	DefaultPawnClass = nullptr; // PostLogin 내부 함수보면 처음에 정해진 게 없을 때 pawn 스폰할 때 defaultpawnclass로 설정하는데 
	// tag 배정되지 않았을 땐 character 배정 안 되도록 nullptr로 막아둠 (tag별로 캐릭터가 다르니까)
	bUseSeamlessTravel = true;
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

void AFDGameMode::StartPlay() // 게임 시작
{
	Super::StartPlay();

	// 맵 옮겨오고 5초뒤에 Role 배정 시작
	GetWorldTimerManager().SetTimer(PhaseTimerHandle, this, &AFDGameMode::AssignRoles, 5.f, false);
}

void AFDGameMode::AssignRoles() // 롤 배정
{
	if (AFDGameState* FDGameState = GetGameState<AFDGameState>())
	{
		FDGameState->SetPhase(EMatchPhase::AssignRole);
		// GameState에 현재 Phase 설정
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] AssignRole 단계 시작"));
	}

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

	// 다음 단계로 넘어감 (1초뒤 다음 함수 예약)
	GetWorldTimerManager().SetTimer(PhaseTimerHandle, this, &AFDGameMode::StartScouting, 1.f, false);
}

void AFDGameMode::StartScouting() // 정찰 모드
{
	AFDGameState* FDGameState = GetGameState<AFDGameState>();
	if (!FDGameState) return;

	FDGameState->SetPhase(EMatchPhase::Scouting);
	UE_LOG(LogTemp, Warning, TEXT("[GameMode] Scouting 단계 시작"));

	// 캐릭터 이동 관련 코드는 헷갈릴 것 같아서 캐릭터쪽에 구현

	for (APlayerState* PS : GameState->PlayerArray)
	{
		const AFDPlayerState* FDPS = Cast<AFDPlayerState>(PS);
		if (!FDPS || FDPS->RoleTag != EFDRole::Tagger) continue; // 술래 태그인 사람 찾기

		if (AFDTaggerCharacter* TaggerChar = Cast<AFDTaggerCharacter>(FDPS->GetPawn()))
		{
			TaggerChar->SetScoutingMode(true); // 캐릭터 쪽에 구현해놓은 술래 정찰모드 켜기
		}
	}

	// DataTable -> ScoutPhaseTime
	float ScoutTime = 60.f;
	if (const FMatchBalanceSettings* Settings = GetBalanceSettings())
	{
		ScoutTime = Settings->ScoutPhaseTime;
	}

	// 정찰 페이즈가 끝날 서버 시각을 찍어 복제 -> 클라 카운트다운이 이 값을 읽음
	FDGameState->PhaseEndServerTime = FDGameState->GetServerWorldTimeSeconds() + ScoutTime;
	
	GetWorldTimerManager().SetTimer(PhaseTimerHandle, this, &AFDGameMode::StartInGame, ScoutTime, false);
}

void AFDGameMode::StartInGame() // 본게임 시작
{
	// 정찰 모드 종료 — Scouting에서 켰던 걸 대칭으로 끔
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

	// 생존한 숨는사람 수 저장해두기 
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
	FDGameState->AliveHiderCount = HiderCount; // 그걸 GameState의 replication 되는 alivehidercount에 넣음

	// DataTable -> MainGameTimeLimit
	float GameTimeLimit = 300.f;
	if (const FMatchBalanceSettings* Settings = GetBalanceSettings())
	{
		GameTimeLimit = Settings->MainGameTimeLimit;
	}

	// 본게임 페이즈가 끝날 서버 시각을 찍어 복제
	FDGameState->PhaseEndServerTime = FDGameState->GetServerWorldTimeSeconds() + GameTimeLimit;
	
	GetWorldTimerManager().SetTimer(PhaseTimerHandle, this, &AFDGameMode::EndMatch, GameTimeLimit, false);
}

void AFDGameMode::EndMatch() // 게임 끝
{
	AFDGameState* FDGameState = GetGameState<AFDGameState>();
	if (!FDGameState) return;

	// AliveHiderCount로 승패 판별
	// 0이면 술래 승, 0보다 크면 시간 종료(하이더 승)
	FDGameState->Winner = (FDGameState->AliveHiderCount <= 0)
		? EMatchWinner::Tagger
		: EMatchWinner::Hider;

	// 순위부터 채우고 phase를 바꿔야 순위표 위젯이 뜰 때 데이터가 이미 있음
	FinalizeHiderRanking();
	FDGameState->SetPhase(EMatchPhase::GameOver);
	
	// 게임 끝났으니 모든 플레이어 이동 잠금 (카메라는 허용)
	for (APlayerState* PS : GameState->PlayerArray)
	{
		if (AFDPlayerController* FDPC = Cast<AFDPlayerController>(PS->GetOwningController()))
		{
			FDPC->Client_LockMovement();   // RPC로
		}
	}

	GetWorldTimerManager().ClearTimer(PhaseTimerHandle);

	UE_LOG(LogTemp, Warning, TEXT("[GameMode] 게임 종료 - 승자: %s"),
		FDGameState->Winner == EMatchWinner::Tagger ? TEXT("Tagger") : TEXT("Hider"));
}

void AFDGameMode::TeleportPlayersToStarts()
{
	// --- 1단계: 레벨의 PlayerStart들을 태그별로 수집 ---
	TArray<APlayerStart*> TaggerStarts;
	TArray<APlayerStart*> HiderStarts;

	// TActorIterator: 레벨에 배치된 특정 타입 액터를 전부 훑는 엔진 제공 반복자
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		APlayerStart* Start = *It;
		if (!Start) continue;

		// PlayerStartTag: APlayerStart가 기본 제공하는 이름표 필드 (에디터에서 지정)
		if (Start->PlayerStartTag == TEXT("Tagger"))
		{
			TaggerStarts.Add(Start);
		}
		else if (Start->PlayerStartTag == TEXT("Hider"))
		{
			HiderStarts.Add(Start);
		}
	}

	// --- 2단계: 각 캐릭터를 역할에 맞는 자리로 라운드로빈 배치 ---
	// 같은 태그가 여러 명(하이더 다수)이면 % 연산으로 번갈아 나눠 넣는다
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

		// 텔레포트: 서버에서 옮기면 복제로 전 클라에 동기화된다.
		// Sweep=false → 경로 충돌 검사 없이 목적지에 바로 꽂는다.
		// (텔레포트는 순간이동이라 경로를 훑을 필요가 없음. true면 오히려 목적지 못 감)
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
	// 본게임 중이 아니면 판정 안함

	TaggerCharacter->StartCaptureSequence(HiderCharacter);
}

void AFDGameMode::ResolveCapture(ACharacter* HiderCharacter, bool bWasCaptured)
{
	if (!HiderCharacter || !bWasCaptured) return;

	AFDGameState* FDGameState = GetGameState<AFDGameState>();
	if (!FDGameState) return;

	// 잡힌 하이더 본인의 PlayerState에 기록
	if (APlayerState* HiderPS = HiderCharacter->GetPlayerState())
	{
		if (AFDPlayerState* FDHiderPS = Cast<AFDPlayerState>(HiderPS))
		{
			FDHiderPS->bIsAlive = false;
			FDHiderPS->DeathServerTime = FDGameState->GetServerWorldTimeSeconds();
		}
	}

	--FDGameState->AliveHiderCount;

	UE_LOG(LogTemp, Warning, TEXT("[GameMode] 포획 확정 - 생존 하이더 %d명 남음"), FDGameState->AliveHiderCount);

	if (FDGameState->AliveHiderCount <= 0)
	{
		EndMatch();
	}
}

void AFDGameMode::FinalizeHiderRanking()
{
	AFDGameState* FDGameState = GetGameState<AFDGameState>();
	if (!FDGameState) return;

	// 본게임 시작 시각 역산: 종료시각 - 제한시간 = 시작시각
	float GameTimeLimit = 300.f;
	if (const FMatchBalanceSettings* Settings = GetBalanceSettings())
	{
		GameTimeLimit = Settings->MainGameTimeLimit;
	}
	const float GameStartTime = FDGameState->PhaseEndServerTime - GameTimeLimit;

	// 하이더만 수집
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

	// 생존 시간 내림차순 정렬
	Hiders.Sort([](const AFDPlayerState& A, const AFDPlayerState& B)
	{
		const bool bAAlive = (A.DeathServerTime < 0.f); // 안 잡힘 = 끝까지 생존
		const bool bBAlive = (B.DeathServerTime < 0.f);

		if (bAAlive != bBAlive) return bAAlive;          // 생존자가 항상 앞
		if (bAAlive && bBAlive) return false;            // 둘 다 생존이면 순서 무관
		return A.DeathServerTime > B.DeathServerTime;    // 둘 다 잡혔으면 늦게 잡힌 쪽이 앞
	});

	// 순위표 배열 채우기
	FDGameState->HiderRankings.Empty();
	for (int32 i = 0; i < Hiders.Num(); ++i)
	{
		AFDPlayerState* FDPS = Hiders[i];

		FHiderRankEntry Entry;
		Entry.PlayerName = FDPS->GetPlayerName();
		Entry.Rank = i + 1;

		// 생존 시간: 안 잡혔으면 전체 시간, 잡혔으면 사망시각 - 시작시각
		Entry.SurvivalTime = (FDPS->DeathServerTime < 0.f)
			? GameTimeLimit
			: (FDPS->DeathServerTime - GameStartTime);

		FDGameState->HiderRankings.Add(Entry);
	}

	// 서버(리슨 호스트)는 OnRep이 자동으로 안 불리므로 수동 방송
	FDGameState->OnRep_HiderRankings();
}