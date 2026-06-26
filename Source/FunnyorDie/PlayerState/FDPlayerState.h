// FDPlayerState.h

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "FDPlayerState.generated.h"

UCLASS()
class FUNNYORDIE_API AFDPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	UPROPERTY(Replicated, BlueprintReadOnly)
	uint8 RoleTag; // 술래/숨는사람 구분용 태그

	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bIsAlive = true; // 생존여부

	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bIsInvincible = false; // 무적여부 (봐주기 버프)

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};