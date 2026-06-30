// FDPlayerState.h

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "FDPlayerState.generated.h"

UENUM(BlueprintType)
enum class EFDRole : uint8 { None, Tagger, Hider };

UCLASS()
class FUNNYORDIE_API AFDPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	UPROPERTY(ReplicatedUsing = OnRep_RoleTag)
	EFDRole RoleTag = EFDRole::None; 

	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bIsAlive = true; // 생존여부

	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bIsInvincible = false; // 무적여부 (봐주기 버프)
	
	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bIsHost = false; // 방장 여부 - 로비에서만 의미 있음 Server에서만 true로 세팅됨(lobbygamemode)

	UFUNCTION()
	void OnRep_RoleTag();

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};