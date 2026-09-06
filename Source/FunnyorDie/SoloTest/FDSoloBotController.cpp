// FDSoloBotController.cpp

#include "SoloTest/FDSoloBotController.h"
#include "PlayerState/FDPlayerState.h"
#include "Character/FDTaggerCharacter.h"
#include "GameState/FDGameState.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

AFDSoloBotController::AFDSoloBotController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AFDSoloBotController::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	/*
	AAIController에는 bWantsPlayerState 플래그가 있어서 켜두면 엔진이 알아서
	PlayerState를 만들어주지만, 이 클래스는 AIModule 의존성을 피하려고
	AController를 직접 상속했기 때문에 그 플래그가 없다.
	그래서 같은 일을 여기서 직접 한다.

	봇이 PlayerState를 들고 GameState의 PlayerArray에 등록되는 게 이 클래스의 핵심이다.
	그래야 역할 배정, 폰 클래스 선택, 역할별 PlayerStart 배치, AliveHiderCount 집계,
	순위표, 관전 대상 탐색이 전부 사람과 같은 경로로 처리되고 봇 전용 분기가 필요 없어진다.

	InitPlayerState는 GameMode의 PlayerStateClass(=AFDPlayerState)로 PlayerState를 스폰하므로
	GameMode가 준비된 뒤여야 한다. 봇은 GameMode의 StartPlay 안에서 스폰되니 이 조건은 충족된다.
	*/
	if (!PlayerState && GetWorld() && GetWorld()->GetAuthGameMode())
	{
		InitPlayerState();
	}
}

APawn* AFDSoloBotController::FindHumanPawn() const
{
	const UWorld* World = GetWorld();
	if (!World) return nullptr;

	// 1인 테스트라 사람은 항상 0번 하나뿐
	if (const APlayerController* HumanPC = World->GetFirstPlayerController())
	{
		return HumanPC->GetPawn();
	}

	return nullptr;
}

void AFDSoloBotController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HasAuthority()) return;

	const AFDPlayerState* MyPS = GetPlayerState<AFDPlayerState>();
	if (!MyPS) return;

	// 하이더 봇은 잡히는 역할이라 아무것도 안 함
	if (MyPS->RoleTag != EFDRole::Tagger) return;

	// 본게임 전에는 쫓지 않음 (정찰 단계에 미리 잡히면 흐름 확인이 안 됨)
	const AFDGameState* FDGameState = GetWorld() ? GetWorld()->GetGameState<AFDGameState>() : nullptr;
	if (!FDGameState || FDGameState->CurrentPhase != EMatchPhase::InGame) return;

	AFDTaggerCharacter* MyTagger = Cast<AFDTaggerCharacter>(GetPawn());
	if (!MyTagger) return;

	APawn* TargetPawn = FindHumanPawn();
	if (!TargetPawn) return;

	FVector ToTarget = TargetPawn->GetActorLocation() - MyTagger->GetActorLocation();
	ToTarget.Z = 0.f;

	const float Distance = ToTarget.Size();
	if (Distance < KINDA_SMALL_NUMBER) return;

	const FVector Direction = ToTarget / Distance;

	// 술래 캐릭터는 bUseControllerRotationYaw가 켜져 있어서 컨트롤 로테이션을 몸이 따라감
	SetControlRotation(Direction.Rotation());

	if (Distance > StopDistance)
	{
		MyTagger->AddMovementInput(Direction, 1.f);
	}

	// 포획 시도는 일정 간격으로만
	CaptureCooldown -= DeltaSeconds;
	if (CaptureCooldown > 0.f) return;
	if (Distance > CaptureTryDistance) return;

	CaptureCooldown = CaptureTryInterval;

	/*
	Server_TryCapture는 Server RPC지만 지금은 서버에서 부르는 것이라
	RPC로 나가지 않고 _Implementation이 그 자리에서 바로 실행된다.
	포획 콜리전을 0.2초 켰다가 끄는 원래 흐름을 그대로 탄다.
	*/
	MyTagger->Server_TryCapture();

	if (!bInstantOutOnCapture) return;

	/*
	봇에게는 포획 팝업을 띄울 PlayerController가 없어서 놔두면 판정 대기 시간이
	다 흐른 뒤에야 아웃 처리된다. 콜리전 오버랩이 잡히고 판정이 시작될 틈을 준 뒤
	아웃을 확정한다. ForceOut은 판정 중인 대상이 없으면 스스로 빠져나오므로
	헛스윙이어도 안전하다.
	*/
	TWeakObjectPtr<AFDTaggerCharacter> WeakTagger(MyTagger);
	FTimerHandle OutHandle;
	GetWorldTimerManager().SetTimer(
		OutHandle,
		FTimerDelegate::CreateLambda([WeakTagger]()
		{
			if (AFDTaggerCharacter* TaggerPtr = WeakTagger.Get())
			{
				TaggerPtr->ForceOut();
			}
		}),
		0.4f, false);
}
