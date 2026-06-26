// FDHiderCharacter.h
// 숨는사람용 캐릭터

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FDHiderCharacter.generated.h"

UCLASS()
class FUNNYORDIE_API AFDHiderCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AFDHiderCharacter();

	// 정찰 단계 시야 차단용
	virtual bool IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const override;

protected:
	virtual void BeginPlay() override;

	// Overlap 됐을 때 호출될 함수
	void OnCaptureOverlap();
};
