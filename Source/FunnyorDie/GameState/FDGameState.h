// FDGameState.h

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "GameMode/FDGameMode.h"
#include "FDGameState.generated.h"

class UUserWidget;

UENUM(BlueprintType)
enum class EMatchWinner : uint8 { None, Tagger, Hider };

// 하이더 순위 한 줄 - 순위표 UI가 읽어 표시
USTRUCT(BlueprintType)
struct FHiderRankEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString PlayerName;

	UPROPERTY(BlueprintReadOnly)
	int32 Rank = 0;

	UPROPERTY(BlueprintReadOnly)
	float SurvivalTime = 0.f;
};

// 순위 배열이 갱신됐을 때 방송 (순위표 WBP가 구독해서 다시 그림)
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRankingsUpdated);

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
	
	// 서버 전용 페이즈 변경 진입점
	// 값 대입 + 서버 자신의 수동 OnRep을 한 곳에 묶기
	void SetPhase(EMatchPhase NewPhase);
	
	// 하이더 순위표 (서버가 EndMatch에서 채움) TArray라 복제 가능
	UPROPERTY(ReplicatedUsing = OnRep_HiderRankings, BlueprintReadOnly)
	TArray<FHiderRankEntry> HiderRankings;

	UFUNCTION()
	void OnRep_HiderRankings();

	// 순위 갱신 방송 - WBP가 바인딩
	UPROPERTY(BlueprintAssignable)
	FOnRankingsUpdated OnRankingsUpdated;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	// 정찰 안내 배너 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> ScoutingBannerWidgetClass;

	// 페이즈 카운트다운 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> CountdownWidgetClass;

	// 현재 떠있는 위젯 인스턴스 (생성/제거 시 붙잡아둠)
	UPROPERTY()
	UUserWidget* ScoutingBannerWidget;

	UPROPERTY()
	UUserWidget* CountdownWidget;
	
	// 게임 시작 팝업 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> StartBannerWidgetClass;

	// 현재 떠있는 게임 시작 팝업 인스턴스
	UPROPERTY()
	UUserWidget* StartBannerWidget;
	
	// 순위표 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> RankingWidgetClass;

	UPROPERTY()
	UUserWidget* RankingWidget;

private:
	// 현재 페이즈에 맞춰 phase UI를 켜고 끄는 스위치보드
	// OnRep_CurrentPhase에서 호출
	void UpdatePhaseUI();
	
	// "게임 시작!" 팝업 자동 제거 타이머
	FTimerHandle StartBannerTimerHandle;
};