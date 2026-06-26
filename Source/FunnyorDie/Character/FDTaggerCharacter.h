// FDTaggerCharacter.h
// 술래용 캐릭터

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FDTaggerCharacter.generated.h"

UCLASS()
class FUNNYORDIE_API AFDTaggerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AFDTaggerCharacter();

protected:
	virtual void BeginPlay() override;
	
	void TryCapture(); // 공격 시도 함수
};