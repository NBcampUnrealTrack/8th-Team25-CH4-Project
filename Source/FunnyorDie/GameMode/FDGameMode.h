// FDGameMode.h

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FDGameMode.generated.h"

// 게임 단계 enum
UENUM(BlueprintType)
enum class EMatchPhase : uint8 { Warmup, AssignRole, Scouting, InGame, GameOver };

UCLASS()
class FUNNYORDIE_API AFDGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	virtual void PostLogin(APlayerController* NewPlayer) override; // 접속 처리

protected:
	void StartWarmup();     // 대기
	void AssignRoles();     // 역할 분배
	void StartScouting();   // 정찰 60초
	void StartInGame();     // 본게임 300초
	void EndMatch();        // 종료

	void RequestCaptureJudgement(); // 포획 판정 시작
	void ResolveCapture();          // 포획 최종 확인

	FTimerHandle PhaseTimerHandle; // 단계 전환용 타이머
};