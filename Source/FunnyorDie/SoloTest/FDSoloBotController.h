// FDSoloBotController.h
// 1인 테스트용 더미 봇 컨트롤러

/*
AAIController가 아니라 AController를 그대로 상속한다.
AAIController를 쓰면 Build.cs에 AIModule / NavigationSystem을 추가해야 하고
MoveToActor가 NavMesh 위에서만 동작해서 레벨에 NavMeshBoundsVolume도 깔아야 한다.
1인 테스트에 그만한 준비가 필요하지 않아서, 목표 방향으로 AddMovementInput만 넣는
직선 추격으로 대체했다. 벽 뒤로 돌아가지는 못하지만 게임 흐름 확인에는 충분하다.

그 대가로 AAIController가 제공하던 bWantsPlayerState 플래그를 못 쓰기 때문에
PlayerState 생성은 PostInitializeComponents에서 직접 처리한다.

술래 봇은 사람 하이더를 쫓아가 사거리에 들어오면 포획을 시도하고,
하이더 봇은 제자리에서 잡히기만 한다.
*/

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Controller.h"
#include "FDSoloBotController.generated.h"

UCLASS()
class FUNNYORDIE_API AFDSoloBotController : public AController
{
	GENERATED_BODY()

public:
	AFDSoloBotController();

	// PlayerState를 직접 만들어 PlayerArray에 등록시킴
	virtual void PostInitializeComponents() override;

	virtual void Tick(float DeltaSeconds) override;

protected:
	// 이 거리보다 가까워지면 더 다가가지 않음
	UPROPERTY(EditDefaultsOnly, Category = "SoloTest")
	float StopDistance = 120.f;

	// 이 거리 안에 들어오면 포획 입력을 넣음
	UPROPERTY(EditDefaultsOnly, Category = "SoloTest")
	float CaptureTryDistance = 200.f;

	// 포획 시도 간격 (초)
	UPROPERTY(EditDefaultsOnly, Category = "SoloTest")
	float CaptureTryInterval = 1.5f;

	/*
	봇 술래에게 잡히면 포획 팝업을 띄울 PlayerController가 없어서
	밸런스 테이블의 판정 대기 시간(기본 15초)이 다 흐른 뒤에야 자동 아웃된다.
	테스트에서 매번 15초를 기다리기 번거로우면 이 값을 켜서 즉시 아웃 처리한다.
	*/
	UPROPERTY(EditDefaultsOnly, Category = "SoloTest")
	bool bInstantOutOnCapture = true;

private:
	// 마지막 포획 시도 이후 누적 시간
	float CaptureCooldown = 0.f;

	// 사람이 조종 중인 폰 반환 (없으면 nullptr)
	APawn* FindHumanPawn() const;
};
