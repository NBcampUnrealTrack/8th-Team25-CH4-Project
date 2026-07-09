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

	// 보관 중인 아이템 사용 요청 (클라이언트 → 서버, 아이템 사용 입력에서 호출)
	UFUNCTION(Server, Reliable)
	void Server_UseHeldItem();

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 현재 보관 중인 아이템의 Row Name
	// 소유 클라이언트가 UI에 표시해야 하므로 복제함
	UPROPERTY(ReplicatedUsing = OnRep_HeldItemRow)
	FName HeldItemRow = NAME_None;

	UFUNCTION()
	void OnRep_HeldItemRow();

	// 아이템 데이터 테이블 (에디터에서 DT_ItemData 할당)
	UPROPERTY(EditDefaultsOnly, Category = "Item")
	class UDataTable* ItemDataTable;

private:
	// 실제 효과 실행 (서버 전용)
	void ExecuteEffect(const FFDItemData& Data);
};