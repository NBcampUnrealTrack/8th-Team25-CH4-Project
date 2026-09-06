// FDPlayerState.h
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "FDPlayerState.generated.h"

// 기존 코드에서 참조되던 역할 enum (FDGameMode.cpp 등에서 이미 사용 중)
UENUM(BlueprintType)
enum class EFDRole : uint8
{
	None,
	Tagger,
	Hider
};

UCLASS()
class FUNNYORDIE_API AFDPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(Replicated)
	EFDRole RoleTag = EFDRole::None;

	UPROPERTY(Replicated)
	bool bIsHost = false;

	UPROPERTY(Replicated)
	bool bIsAlive = true;
	
	// 잡힌 서버 시각 
	// -1이면 아직 안 잡힘 (=생존 중)
	UPROPERTY(Replicated)
	float DeathServerTime = -1.f;

	// ===== 커스터마이징 =====

	/*
	색상 프리셋을 통째로 복제하지 않고 팔레트 인덱스 하나만 복제한다.
	팔레트는 컴포넌트의 클래스 기본값이라 서버와 모든 클라이언트가 동일하고,
	인덱스만 오가므로 복제량이 4바이트로 끝난다.
	클라이언트가 임의의 색상 값을 서버에 밀어넣을 수 없다는 이점도 있다.
	-1은 아직 아무것도 고르지 않은 상태.
	*/
	UPROPERTY(ReplicatedUsing = OnRep_ColorPresetIndex)
	int32 ColorPresetIndex = -1;

	// 인덱스 복제 수신 시 호출 - 소유 Pawn의 커스터마이징 컴포넌트에 적용을 위임
	// 리슨 서버 자신은 OnRep이 자동 호출되지 않아 서버 쪽에서 수동으로도 부른다
	UFUNCTION()
	void OnRep_ColorPresetIndex();
};
