// FDCustomizationTypes.h
// 캐릭터 색상 커스터마이징 공용 구조체

/*
자유 페인팅(RenderTarget 브러시 + PNG 스냅샷 복제) 구조는 이 버전에서 제거했다.
제거 사유는 세 가지다.

1) 스켈레탈 메시에서 FindCollisionUV로 UV를 얻으려면 per-poly 콜리전이 필요한데
   비용이 크고 스켈레탈에서는 UV 정보가 항상 쿠킹된다는 보장도 없다.
2) 완성된 그림을 PNG로 압축해 PlayerState 프로퍼티로 복제하는 구조라
   프로퍼티 하나가 수십에서 수백 KB가 된다. 파티 게임 대역폭으로 감당이 안 된다.
3) 스트로크가 끝날 때마다 ReadPixels로 GPU 리드백을 걸어 프레임이 멈춘다.

대신 확실히 동작하고 눈으로 검증 가능한 범위인 파츠 색상 스왑만 남겼다.
이전 구현은 git 히스토리에 그대로 있다.
*/

#pragma once
#include "CoreMinimal.h"
#include "FDCustomizationTypes.generated.h"

// 파츠별 색상 슬롯 - 캐릭터 머티리얼의 벡터 파라미터와 순서를 맞춰서 사용
UENUM(BlueprintType)
enum class EFDColorSlot : uint8
{
	Body,       // 몸통
	Accent,     // 포인트 컬러
	Eyes,       // 눈
	MAX         // 개수 세는 용도, 실제 슬롯 아님
};

// 색상 프리셋 하나 - 팔레트에 미리 정의해두고 인덱스로만 주고받는다
USTRUCT(BlueprintType)
struct FFDColorPreset
{
	GENERATED_BODY()

	// UI 버튼에 표시될 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Customization")
	FText DisplayName;

	// EFDColorSlot 개수만큼의 배열 (인덱스 = EFDColorSlot 값)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Customization")
	TArray<FLinearColor> SlotColors;

	FFDColorPreset()
	{
		SlotColors.Init(FLinearColor::White, (int32)EFDColorSlot::MAX);
	}

	FFDColorPreset(const FText& InName, const FLinearColor& Body,
	               const FLinearColor& Accent, const FLinearColor& Eyes)
		: DisplayName(InName)
	{
		SlotColors = { Body, Accent, Eyes };
	}

	// UI 스와치에 쓸 대표 색 (몸통 색)
	FLinearColor GetSwatchColor() const
	{
		return SlotColors.IsValidIndex(0) ? SlotColors[0] : FLinearColor::White;
	}
};
