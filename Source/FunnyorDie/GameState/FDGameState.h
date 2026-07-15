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
	
	// 현재 페이즈가 끝날 서버 기준 시각 - 서버가 페이즈 시작 시 한 번만 세팅해서 복제
	// 클라는 이 값에서 현재 서버시각을 빼서 남은 시간을 스스로 계산
	UPROPERTY(Replicated, BlueprintReadOnly)
	float PhaseEndServerTime = 0.f;

	// 카운트다운 위젯이 읽어갈 창구
	UFUNCTION(BlueprintPure, Category = "Match")
	float GetRemainingPhaseTime() const;
	
	UFUNCTION()
	void OnRep_CurrentPhase();

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};