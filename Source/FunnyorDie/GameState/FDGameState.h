// FDGameState.h

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "GameMode/FDGameMode.h"
#include "FDGameState.generated.h"

UCLASS()
class FUNNYORDIE_API AFDGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	// 현재 단계
	UPROPERTY(ReplicatedUsing = OnRep_CurrentPhase, BlueprintReadOnly)
	EMatchPhase CurrentPhase = EMatchPhase::Warmup;

	// 생존자 수
	UPROPERTY(Replicated, BlueprintReadOnly)
	int32 AliveHiderCount;
	
	UFUNCTION()
	void OnRep_CurrentPhase();

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};