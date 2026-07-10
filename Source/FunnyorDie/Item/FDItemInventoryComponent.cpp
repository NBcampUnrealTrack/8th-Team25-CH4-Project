// FDItemInventoryComponent.cpp

#include "Item/FDItemInventoryComponent.h"
#include "Character/FDHiderCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Engine/DataTable.h"

UFDItemInventoryComponent::UFDItemInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// 복제해야 하니 컴포넌트 자체 복제를 켬
	SetIsReplicatedByDefault(true);
}

void UFDItemInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(UFDItemInventoryComponent, InvisibilityCount);
	DOREPLIFETIME(UFDItemInventoryComponent, ThrowItemCount);
}

void UFDItemInventoryComponent::ReceiveDrawnItem(FName ItemRow)
{
	// 서버만
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	if (!ItemDataTable) { return;}

	const FFDItemData* Data = ItemDataTable->FindRow<FFDItemData>(ItemRow, TEXT("뽑힌 아이템 조회"));
	if (!Data) return;

	// 즉시발동형은 저장하지 않고 바로 실행
	if (!Data->bStoredOnPickup)
	{
		ExecuteEffect(*Data);
		return;
	}

	// 나머지는 전부 보관형 
	// 종류별 슬롯에 개수 누적 (한도 넘으면 획득 실패 + 알림)
	switch (Data->Effect)
	{
	case EFDItemEffect::Invisibility:
		if (InvisibilityCount >= MaxInvisibility)
		{
			// 이미 가득 참 획득 실패
			Client_NotifyInventoryFull(EFDItemEffect::Invisibility);
			return;
		}
		++InvisibilityCount;
		OnRep_Inventory(); // 서버 수동 호출 (UI 갱신하는 함수)
		break;

	case EFDItemEffect::TaggerStun:
		if (ThrowItemCount >= MaxThrowItem)
		{
			Client_NotifyInventoryFull(EFDItemEffect::TaggerStun);
			return;
		}
		++ThrowItemCount;
		OnRep_Inventory();
		break;

	default:
		break;
	}
}

void UFDItemInventoryComponent::Server_UseItem_Implementation(EFDItemEffect Which)
{
	if (!ItemDataTable) return;

	switch (Which)
	{
	case EFDItemEffect::Invisibility:
		if (InvisibilityCount <= 0) return;

		// 소모 먼저 (중복 사용 방지)
		--InvisibilityCount;
		OnRep_Inventory();

		// 투명화 효과 실행
		if (const FFDItemData* Data = ItemDataTable->FindRow<FFDItemData>(TEXT("Invisibility"), TEXT("투명화 사용")))
		{
			ExecuteEffect(*Data);
		}
		break;

	case EFDItemEffect::TaggerStun:
		if (ThrowItemCount <= 0) return;

		--ThrowItemCount;
		OnRep_Inventory();

		if (const FFDItemData* Data = ItemDataTable->FindRow<FFDItemData>(TEXT("TaggerStun"), TEXT("투사체 사용")))
		{
			ExecuteEffect(*Data);
		}
		break;

	default:
		break;
	}
}

void UFDItemInventoryComponent::Client_NotifyInventoryFull_Implementation(EFDItemEffect Which)
{
	// UI에 "최대 n개까지만 보관할 수 있습니다" 경고 문구 만들어서 여기에 띄우기
}

void UFDItemInventoryComponent::OnRep_Inventory()
{
	// 인벤토리 UI 여기에서 갱신 (WBP 연동)
}

void UFDItemInventoryComponent::ExecuteEffect(const FFDItemData& Data)
{
	AFDHiderCharacter* Hider = Cast<AFDHiderCharacter>(GetOwner());
	if (!Hider) return;

	switch (Data.Effect)
	{
	case EFDItemEffect::Invisibility:
		{
			// 투명화 켜기
			Hider->SetItemInvisible(true);

			// Duration초 뒤 자동으로 끄기
			GetWorld()->GetTimerManager().SetTimer(EffectExpireTimerHandle,
				[Hider]()
				{
					if (IsValid(Hider))
					{
						Hider->SetItemInvisible(false);
					}
				},
				Data.Duration, false);
			break;
			
			case EFDItemEffect::TaggerStun:
			// 투사체
			break;

			case EFDItemEffect::ForcedEmote:
			// 강제 이모션
			break;

			case EFDItemEffect::Noise:
			// 소리
			break;

			default:
			break;
		}
	}
}