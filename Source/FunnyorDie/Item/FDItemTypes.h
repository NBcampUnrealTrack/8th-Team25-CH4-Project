// FDItemTypes.h

#pragma once
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "FDItemTypes.generated.h"

// 아이템 종류
UENUM(BlueprintType)
enum class EFDItemEffect : uint8
{
	None,
	Invisibility,   // 하이더 투명화
	TaggerStun,     // 술래 멈추기 - 던져서 맞춤
	ForcedEmote,    // 강제 이모션
	Noise     // 소리 발생
};


USTRUCT(BlueprintType)
struct FFDItemData : public FTableRowBase
{
	GENERATED_BODY()

public:
	// 실제로 발동시킬 효과 종류
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	EFDItemEffect Effect;

	// true: 주우면 인벤토리에 보관 (1,2번) / false: 줍는 즉시 발동 (3,4번)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	bool bStoredOnPickup;

	// 효과 지속 시간
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	float Duration;

	// 뽑기 가중치
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	float DrawWeight;

	// 투척형(TaggerStun)일 때 스폰할 투척 아이템 클래스
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TSubclassOf<class AFDThrowItem> ThrowItemClass;

	// 기본값 설정
	FFDItemData()
		: Effect(EFDItemEffect::None)
		, bStoredOnPickup(true)
		, Duration(5.0f)
		, DrawWeight(10.0f)
		, ThrowItemClass(nullptr)
	{
	}
};