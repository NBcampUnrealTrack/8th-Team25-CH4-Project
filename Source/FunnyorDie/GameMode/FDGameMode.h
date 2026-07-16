// FDGameMode.h
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameDataTypes.h"
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
	// PreLogin : PostLogin보다 먼저 불리는 입장 심사대
	// ErrorMessage에 값을 채우면 엔진이 그 접속을 튕겨냄 -> 난입 차단
	virtual void PreLogin(const FString& Options, const FString& Address,
						  const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Spawning")
	TSubclassOf<ACharacter> TaggerClass;
	UPROPERTY(EditDefaultsOnly, Category = "Spawning")
	TSubclassOf<ACharacter> HiderClass;

	// 밸런스 수치 데이터 테이블 (에디터에서 DT_MatchBalanceSettings 할당)
	UPROPERTY(EditDefaultsOnly, Category = "Balance")
	class UDataTable* BalanceDataTable;

	// 데이터 테이블에서 밸런스 설정값을 가져오는 헬퍼 함수
	const FMatchBalanceSettings* GetBalanceSettings() const;

	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

	void AssignRoles();     // 역할 분배
	void StartScouting();   // 정찰 60초
	void StartInGame();     // 본게임 300초
	void EndMatch();        // 종료
	
	// 하이더 순위 계산 후 GameState 배열에 채움
	void FinalizeHiderRanking();
	
	// 본게임 진입 시 역할별 PlayerStart로 캐릭터를 텔레포트 (술래/하이더 분리)
	void TeleportPlayersToStarts();
	
	// 게임 맵이 기다려야 할 목표 인원 (로비에서 GameInstance 통해 건너온 값)
	int32 ExpectedPlayerCount = 0;

	// 로스터 확정 플래그 — true가 되면 PreLogin에서 새 접속 거부
	bool bRosterLocked = false;

	// 전원 재접속을 주기적으로 확인하는 폴링 함수
	void WaitForPlayers();

	float PlayerWaitElapsed = 0.f;      // 대기 누적 시간
	FTimerHandle WaitTimerHandle;        // 대기 폴링 전용 타이머 (PhaseTimerHandle과 분리)

	// 아무리 늦어도 이 시간 넘으면 모인 인원으로 강제 시작
	UPROPERTY(EditDefaultsOnly, Category = "Match")
	float MaxPlayerWaitTime = 15.f;

	// 폴링 간격(초)
	static constexpr float PlayerWaitInterval = 0.5f;
	
public:
	void RequestCaptureJudgement(class AFDTaggerCharacter* TaggerCharacter, ACharacter* HiderCharacter); // 포획 판정 시작
	// 태그가 Hider, Tagger라서 character 변수명은 뒤에 character 붙임

	void ResolveCapture(ACharacter* HiderCharacter, bool bWasCaptured); // 포획 최종 확인
	FTimerHandle PhaseTimerHandle; // 단계 전환용 타이머
};