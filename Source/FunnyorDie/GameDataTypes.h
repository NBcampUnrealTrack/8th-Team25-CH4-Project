// GameDataTypes.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameDataTypes.generated.h"

/**
 * 사물(Prop) 속성 데이터 구조체
 */

USTRUCT(BlueprintType)
struct FPropData : public FTableRowBase
{
	GENERATED_BODY()

public:
	// 사물의 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prop")
	FName Name;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prop")
	TSoftObjectPtr<UStaticMesh> StaticMesh;

	// 무게
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prop")
	float Weight;

	// 속도 페널티 (이 물건을 들고 있을 때 이동속도가 얼마나 느려지는지)
	// 예: 0.8이면 원래 속도의 80%로 느려짐
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prop")
	float SpeedPenalty;

	// 상호작용 가능한 최대 거리 (LineTrace로 이 거리 안에 있을 때만 잡을 수 있음)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prop")
	float InteractionDistance;

	// 기본값 설정 (생성자)
	FPropData()
		: Name(NAME_None)
		, Weight(1.0f)
		, SpeedPenalty(1.0f)
		, InteractionDistance(200.0f)
	{
	}
};

/**
 * 게임 전체 밸런스 설정 구조체
 * 게임 시간, 무적 시간 같은 수치들을 모아둠
 */
USTRUCT(BlueprintType)
struct FMatchBalanceSettings : public FTableRowBase
{
	GENERATED_BODY()

public:
	// 정찰(숨는) 단계 시간 - 기본 60초
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Balance")
	float ScoutPhaseTime;

	// 본게임 제한 시간 - 기본 300초 (5분) 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Balance")
	float MainGameTimeLimit;

	// 포획 판정 대기 시간 - 기본 15초
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Balance")
	float CaptureJudgeWaitTime;

	// 무르기(봐줌) 무적 시간 - 기본 7초
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Balance")
	float SpareInvincibleTime;

	// 무르기 속도 배율 - 기본 1.5배
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Balance")
	float SpareSpeedMultiplier;

	// 포획 판정 콜리전 반지름 - 기본 80
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Balance")
	float CaptureRadius;

	// 캐릭터 기본 이동속도 - 기본 600
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Balance")
	float DefaultWalkSpeed;

	// 술래 정찰(관전) 모드 이동속도 - 기본 1200
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Balance")
	float TaggerScoutSpeed;

	// 기본값 설정 (생성자)
	FMatchBalanceSettings()
		: ScoutPhaseTime(60.0f)
		, MainGameTimeLimit(300.0f)
		, CaptureJudgeWaitTime(15.0f)
		, SpareInvincibleTime(7.0f)
		, SpareSpeedMultiplier(1.5f)
		, CaptureRadius(80.0f)
		, DefaultWalkSpeed(600.0f)
		, TaggerScoutSpeed(1200.0f)
	{
	}
};