// FDHiderCharacter.h
// 숨는사람용 캐릭터

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Engine/DataTable.h"
#include "FDHiderCharacter.generated.h"

// 위장 사물의 크기 정보를 데이터 테이블로 관리하기 위한 행 구조체
// 에디터에서 DataTable 에셋에 행을 추가해 사물별 캡슐 크기를 설정
USTRUCT(BlueprintType)
struct FDisguiseShapeData : public FTableRowBase
{
	GENERATED_BODY()

	// 캡슐 반지름 (가로 반경)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float CapsuleRadius = 34.f;

	// 캡슐 반높이 (세로 반경)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float CapsuleHalfHeight = 88.f;
};

UCLASS()
class FUNNYORDIE_API AFDHiderCharacter : public ACharacter
{
	GENERATED_BODY()
	
public:
	AFDHiderCharacter();

	// 정찰 단계 시야 차단용
	virtual bool IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const override;

	// 위장 상태 진입 요청 (클라이언트 → 서버)
	UFUNCTION(Server, Reliable)
	void Server_EnterDisguise(FName DisguiseRowName);

	// 위장 상태 해제 요청 (클라이언트 → 서버)
	UFUNCTION(Server, Reliable)
	void Server_ExitDisguise();
	
	// 서버 전용: 무적 상태 설정 (봐주기 등 TaggerCharacter/GameMode에서 호출)
	void SetInvincible(bool bNewInvincible);

	// 현재 무적 여부 조회 ( bIsInvincible이 복제되므로 클라이언트/서버 모두 사용 가능)
	bool IsInvincible() const { return bIsInvincible; }

protected:
	virtual void BeginPlay() override;

	// 위장 상태 여부, 서버에서 변경 시 클라이언트에 자동 전파
	UPROPERTY(ReplicatedUsing = OnRep_bIsDisguised)
	bool bIsDisguised = false;

	// 봐주기 등으로 인한 무적 상태, 서버에서 변경 시 클라이언트에 자동 전파
	UPROPERTY(ReplicatedUsing = OnRep_bIsInvincible)
	bool bIsInvincible = false;
	
	// bIsDisguised 복제 시 클라이언트에서 호출되는 콜백
	UFUNCTION()
	void OnRep_bIsDisguised();
	
	// bIsInvincible 복제 시 클라이언트에서 호출되는 콜백
	UFUNCTION()
	void OnRep_bIsInvincible();

	// 위장 진입 시 애님 재생 속도 0으로 고정 (모든 클라이언트에 동기화)
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_FreezeAnimation(bool bFreeze);

	// 캡슐 크기를 데이터 테이블 값에 맞춰 변경 (모든 클라이언트에 동기화)
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_ResizeCapsule(float NewRadius, float NewHalfHeight);

	// Overlap 됐을 때 호출될 함수
	void OnCaptureOverlap();

private:
	// 위장 사물 크기 데이터 테이블 (에디터에서 할당)
	UPROPERTY(EditDefaultsOnly, Category = "Disguise")
	UDataTable* DisguiseDataTable;

	// 위장 해제 전 원래 캡슐 크기 저장용
	float OriginalCapsuleRadius = 34.f;
	float OriginalCapsuleHalfHeight = 88.f;
};
