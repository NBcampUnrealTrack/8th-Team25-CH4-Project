// FDGameState.h

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "FDGameState.generated.h"

UCLASS()
class FUNNYORDIE_API AFDGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	// 현재 단계
	UPROPERTY(Replicated, BlueprintReadOnly)
	uint8 CurrentPhase;

	// 생존자 수
	UPROPERTY(Replicated, BlueprintReadOnly)
	int32 AliveHiderCount;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};