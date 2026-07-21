// FDCustomizationTypes.h
// 캐릭터 채색(커스터마이징) 관련 공용 구조체 모음
// Hider/Tagger 양쪽 캐릭터가 공통으로 사용하기 위해 별도 파일로 분리함

#pragma once
#include "CoreMinimal.h"
#include "FDCustomizationTypes.generated.h"

// 파츠별 색상 슬롯 (머티리얼 파라미터 스왑 방식)
// 에디터에서 캐릭터 머티리얼에 만들어둔 벡터 파라미터 이름과 순서를 맞춰서 사용
UENUM(BlueprintType)
enum class EFDColorSlot : uint8
{
	Body,       // 몸통
	Accent,     // 포인트 컬러
	Eyes,       // 눈
	MAX         // 개수 세는 용도, 실제 슬롯 아님
};

// 파라미터 스왑 방식 색상 데이터
// - 가볍고 즉시 반영 가능해서 매치 중 실시간으로 바꿔도 부담 없음
USTRUCT(BlueprintType)
struct FFDColorPreset
{
	GENERATED_BODY()

	// EFDColorSlot 개수만큼 배열로 관리 (인덱스 = EFDColorSlot 값)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FLinearColor> SlotColors;

	FFDColorPreset()
	{
		SlotColors.Init(FLinearColor::White, (int32)EFDColorSlot::MAX);
	}
};

// 자유 페인팅 결과를 압축해서 저장할 때 쓰는 스냅샷
// PlayerState에 Replicate 되어 재스폰/재접속/뒤늦은 관전 시 그림 복원용으로 사용됨
// (RenderTarget 자체는 Replicate가 안 되기 때문에, "완성된 결과 이미지"만 압축해서 들고 다니는 개념)
USTRUCT(BlueprintType)
struct FFDPaintSnapshot
{
	GENERATED_BODY()

	// PNG로 압축된 RenderTarget 결과 바이트
	UPROPERTY()
	TArray<uint8> CompressedPixels;

	// 압축 전 원본 RT 가로/세로 크기 (디코딩 시 필요)
	UPROPERTY()
	int32 Width = 0;

	UPROPERTY()
	int32 Height = 0;

	bool IsValid() const
	{
		return CompressedPixels.Num() > 0 && Width > 0 && Height > 0;
	}
};

// 실시간 브러시 스트로크 한 점 - RPC로 자주 오가는 최소 단위
// Server_StrokePoint는 Unreliable로 보낼 거라 최대한 가볍게 유지해야 함
USTRUCT(BlueprintType)
struct FFDStrokePoint
{
	GENERATED_BODY()

	// 캐릭터 메시 UV 좌표 (0~1 범위)
	UPROPERTY()
	FVector2D UV = FVector2D::ZeroVector;

	UPROPERTY()
	FLinearColor Color = FLinearColor::White;

	// 브러시 반경 (UV 공간 기준 0~1 사이 값, 예: 0.02 = RT 크기의 2%)
	UPROPERTY()
	float BrushRadius = 0.02f;
};
