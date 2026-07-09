// FDItemInventoryComponent.cpp

#include "Item/FDItemInventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/DataTable.h"

UFDItemInventoryComponent::UFDItemInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// HeldItemRow를 복제해야 하니 컴포넌트 자체 복제를 켬
	SetIsReplicatedByDefault(true);
}

void UFDItemInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UFDItemInventoryComponent, HeldItemRow);
}

void UFDItemInventoryComponent::ReceiveDrawnItem(FName ItemRow)
{
	// 서버만
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	if (!ItemDataTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("[아이템 인벤토리] ItemDataTable이 할당되지 않음"));
		return;
	}

	const FFDItemData* Data = ItemDataTable->FindRow<FFDItemData>(ItemRow, TEXT("뽑힌 아이템 조회"));
	if (!Data) return;

	if (Data->bStoredOnPickup)
	{
		// 보관형 인벤토리에 저장 - 이미 들고 있었으면 덮어씀 (개수 추가하는 거 나중에 구현해도 됨)
		HeldItemRow = ItemRow;

		// 서버니까 수동 호출
		OnRep_HeldItemRow();
	}
	else
	{
		// 즉시발동형 저장하지 않고 바로 실행
		ExecuteEffect(*Data);
	}
}

void UFDItemInventoryComponent::Server_UseHeldItem_Implementation()
{
	if (HeldItemRow.IsNone()) return;
	

	if (!ItemDataTable) return;

	const FFDItemData* Data = ItemDataTable->FindRow<FFDItemData>(HeldItemRow, TEXT("보관 아이템 사용"));
	if (!Data) return;

	// 소모 처리를 효과 실행보다 먼저 함
	// 효과 실행 도중 중복 사용 요청이 들어와도 이미 HeldItemRow가 비어있어서 씹히도록
	const FFDItemData UsedData = *Data; // ExecuteEffect에 넘길 값 복사 (아래서 HeldItemRow를 지우기 전에)
	HeldItemRow = NAME_None;
	OnRep_HeldItemRow();

	ExecuteEffect(UsedData);
}

// 보관중인 Item 업데이트 될 때마다 불림
void UFDItemInventoryComponent::OnRep_HeldItemRow()
{
	// 아이템 슬롯 UI 갱신
	UE_LOG(LogTemp, Warning, TEXT("Item 업데이트"));

}

void UFDItemInventoryComponent::ExecuteEffect(const FFDItemData& Data)
{
	UE_LOG(LogTemp, Warning, TEXT("효과 발동"));
}