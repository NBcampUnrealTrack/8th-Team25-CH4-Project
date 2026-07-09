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

private:
	// 실제 효과 실행 (서버 전용)
	void ExecuteEffect(const FFDItemData& Data);
	
	// 보관 한도 상수
	static constexpr int32 MaxInvisibility = 1;
	static constexpr int32 MaxThrowItem = 3;

	FTimerHandle EffectExpireTimerHandle;
};