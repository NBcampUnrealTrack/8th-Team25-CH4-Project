// FDGameMode.h

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FDGameMode.generated.h"

class ACharacter;

// 게임 단계 enum
UENUM(BlueprintType)
enum class EMatchPhase : uint8 { Warmup, AssignRole, Scouting, InGame, GameOver };

UCLASS()
class FUNNYORDIE_API AFDGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	AFDGameMode();
	
	virtual void PostLogin(APlayerController* NewPlayer) override; // 접속 처리
	virtual void StartPlay() override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Spawning")
	TSubclassOf<ACharacter> TaggerClass;

	UPROPERTY(EditDefaultsOnly, Category = "Spawning")
	TSubclassOf<ACharacter> HiderClass;
	
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;
	
	void AssignRoles();     // 역할 분배
	void StartScouting();   // 정찰 60초
	void StartInGame();     // 본게임 300초
	void EndMatch();        // 종료

public:
	void RequestCaptureJudgement(class AFDTaggerCharacter* TaggerCharacter, ACharacter* HiderCharacter); // 포획 판정 시작
	// 태그가 Hider, Tagger라서 character 변수명은 뒤에 character 붙임
	
	void ResolveCapture(ACharacter* HiderCharacter, bool bWasCaptured); // 포획 최종 확인

	FTimerHandle PhaseTimerHandle; // 단계 전환용 타이머
};