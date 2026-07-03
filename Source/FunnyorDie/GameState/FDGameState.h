// FDGameState.h

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "GameMode/FDGameMode.h"
#include "FDGameState.generated.h"

UENUM(BlueprintType)
enum class EMatchWinner : uint8 { None, Tagger, Hider };

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
	
	// 승리 판정
	UPROPERTY(Replicated, BlueprintReadOnly)
	EMatchWinner Winner = EMatchWinner::None;
	
	UFUNCTION()
	void OnRep_CurrentPhase();

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};