// FDItemInventoryComponent.h

#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Item/FDItemTypes.h"
#include "FDItemInventoryComponent.generated.h"

UCLASS(ClassGroup = (Item), meta = (BlueprintSpawnableComponent))
class FUNNYORDIE_API UFDItemInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFDItemInventoryComponent();

	// 뽑은 결과를 넘겨줄 때 호출
	// bStoredOnPickup에 따라 보관하거나 즉시 발동
	void ReceiveDrawnItem(FName ItemRow);

	// 보관 아이템 사용 요청 - 어떤 종류를 쓸지 지정
	UFUNCTION(Server, Reliable)
	void Server_UseItem(EFDItemEffect Which);
	
	// 보관 한도 초과로 획득 실패 시 그 클라이언트에게만 알림 (UI 경고 문구용)
	UFUNCTION(Client, Reliable)
	void Client_NotifyInventoryFull(EFDItemEffect Which);

	// UI가 읽어갈 수 있게 개수 조회 함수 (BlueprintPure - WBP에서 바인딩)
	UFUNCTION(BlueprintPure, Category = "Item")
	int32 GetInvisibilityCount() const { return InvisibilityCount; }

	UFUNCTION(BlueprintPure, Category = "Item")
	int32 GetThrowItemCount() const { return ThrowItemCount; }
	
	// 조준 중 좌클릭 → 실제 발사 
	void FireThrowItem();
	
	// 투사체가 있을 때만 조준 진입 가능
	void ToggleAiming();

	// 조준 중인지 (로컬 판단용)
	bool IsAiming() const { return bIsAiming; }

	// 매 프레임 궤적 갱신용 (조준 중일 때만 Tick이 켜짐)
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 투명화 보관 개수 - UI 표시용으로 복제
	UPROPERTY(ReplicatedUsing = OnRep_Inventory)
	int32 InvisibilityCount = 0;

	// 투사체 보관 개수 - UI 표시용으로 복제
	UPROPERTY(ReplicatedUsing = OnRep_Inventory)
	int32 ThrowItemCount = 0;

	// 인벤토리 UI를 다시 그리게 하려고 만든 함수
	UFUNCTION()
	void OnRep_Inventory();

	// 아이템 데이터 테이블 (에디터에서 DT_ItemData 할당)
	UPROPERTY(EditDefaultsOnly, Category = "Item")
	class UDataTable* ItemDataTable;
	
	// 투사체 발사 속도 - AFDThrowItem의 InitialSpeed와 반드시 같아야 함
	// (다르면 화면에 보이는 궤적과 실제 날아가는 경로가 어긋남)
	UPROPERTY(EditDefaultsOnly, Category = "Item|Throw")
	float ThrowSpeed = 1200.f;

	// 투사체 중력 스케일 - AFDThrowItem의 ProjectileGravityScale과 같아야 함
	UPROPERTY(EditDefaultsOnly, Category = "Item|Throw")
	float ThrowGravityScale = 1.f;

	// 발사 시작점 오프셋 - 하이더 기준 앞으로/위로 얼마나 띄울지
	UPROPERTY(EditDefaultsOnly, Category = "Item|Throw")
	float ThrowForwardOffset = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Item|Throw")
	float ThrowUpOffset = 50.f;

private:
	// 실제 효과 실행 (서버 전용)
	void ExecuteEffect(const FFDItemData& Data);
	
	// 보관 한도 상수
	static constexpr int32 MaxInvisibility = 1;
	static constexpr int32 MaxThrowItem = 3;

	FTimerHandle EffectExpireTimerHandle;
	
	// 조준 상태 - 복제 안함
	bool bIsAiming = false;

	// 조준 중 궤적을 계산해서 화면에 표시
	void UpdateTrajectory();

	// 조준 시작/종료 내부 처리 (Tick 켜고 끄기 포함)
	void StartAiming();
	void StopAiming();
	
	// 발사 시작 위치/방향 계산 - 궤적 표시와 실제 발사가 같은 값을 쓰도록 공용화
	void GetThrowStartAndVelocity(FVector& OutStart, FVector& OutVelocity) const;
};