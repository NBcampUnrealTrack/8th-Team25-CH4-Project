// FDSoloGameMode.h
// 1인 테스트 전용 게임모드

/*
AFDGameMode를 상속만 하고 기존 함수는 하나도 오버라이드하지 않는다.
부모의 AssignRoles는 WaitForPlayers에서만 호출되는데, 이 클래스는 StartPlay에서
부모를 건너뛰고 조부모(AGameModeBase)를 직접 부르기 때문에 그 경로 자체를 안 탄다.
그래서 팀이 쓰는 FDGameMode.h / .cpp는 한 글자도 수정할 필요가 없다.

바꾸는 건 두 가지다.

1) 인원 대기 스킵
   부모는 로비에서 넘어온 인원이 다 모일 때까지 0.5초 간격으로 폴링하고
   최대 15초 뒤에 강제 시작한다. 혼자 하는 판에는 기다릴 상대가 없다.

2) 역할 고정
   부모는 무작위로 술래를 뽑는다. 혼자면 항상 본인이 술래가 되어 하이더를 못 만져본다.
   여기서는 URL 옵션(?role=)으로 받은 역할을 사람에게 주고 봇에게 반대를 준다.

에디터 세팅을 안 하고도 바로 돌아가도록 캐릭터/컨트롤러/GameState/밸런스 테이블을
생성자에서 직접 물어온다. 테스트 전용 클래스라 애셋 경로를 코드에 박아도 되는 자리다.
*/

#pragma once
#include "CoreMinimal.h"
#include "GameMode/FDGameMode.h"
#include "PlayerState/FDPlayerState.h" // EFDRole
#include "FDSoloGameMode.generated.h"

class AFDSoloBotController;

UCLASS()
class FUNNYORDIE_API AFDSoloGameMode : public AFDGameMode
{
	GENERATED_BODY()

public:
	AFDSoloGameMode();

	// URL 옵션에서 ?role=tagger / ?role=hider 를 읽음
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

	virtual void StartPlay() override;

protected:
	// URL에 role 옵션이 없을 때 쓸 역할
	UPROPERTY(EditDefaultsOnly, Category = "SoloTest")
	EFDRole DefaultRole = EFDRole::Hider;

	// 플레이어가 술래일 때 채워 넣을 하이더 봇 수
	UPROPERTY(EditDefaultsOnly, Category = "SoloTest")
	int32 HiderBotCount = 3;

	// 플레이어가 하이더일 때 채워 넣을 술래 봇 수
	UPROPERTY(EditDefaultsOnly, Category = "SoloTest")
	int32 TaggerBotCount = 1;

	UPROPERTY(EditDefaultsOnly, Category = "SoloTest")
	TSubclassOf<AFDSoloBotController> BotControllerClass;

private:
	// 이번 판에서 사람 플레이어가 맡을 역할
	EFDRole LocalPlayerRole = EFDRole::Hider;

	// 컨트롤러만 생성 - 폰은 AssignSoloRoles의 RestartPlayer가 붙여줌
	void SpawnBots(int32 Count);

	// 부모의 AssignRoles를 대체하는 자체 역할 배정 (오버라이드 아님)
	void AssignSoloRoles();

	UPROPERTY()
	TArray<AController*> SpawnedBots;
};
