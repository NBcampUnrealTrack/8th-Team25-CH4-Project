// FDItemPickup.cpp

#include "Item/FDItemPickup.h"
#include "Item/FDItemInventoryComponent.h"
#include "Character/FDHiderCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DataTable.h"

AFDItemPickup::AFDItemPickup()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true; // 서버가 스폰하면 클라이언트에도 자동으로 보이게

	PickupCollision = CreateDefaultSubobject<USphereComponent>(TEXT("PickupCollision"));
	SetRootComponent(PickupCollision);
	PickupCollision->SetSphereRadius(100.f);
	PickupCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	BoxMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoxMesh"));
	BoxMesh->SetupAttachment(RootComponent);
	BoxMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AFDItemPickup::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		PickupCollision->OnComponentBeginOverlap.AddDynamic(
			this, &AFDItemPickup::OnPickupOverlap);
	}
}

void AFDItemPickup::OnPickupOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bIsActive) return;

	// 하이더만 주울 수 있음
	AFDHiderCharacter* Hider = Cast<AFDHiderCharacter>(OtherActor);
	if (!Hider) return;

	UFDItemInventoryComponent* Inventory =
		Hider->FindComponentByClass<UFDItemInventoryComponent>();
	if (!Inventory) return;

	// 랜덤 뽑기 (아래 구현) - 뭐 뽑았는지 정해져서 DrawnRow에 저장됨
	const FName DrawnRow = DrawRandomItemRow();
	if (DrawnRow.IsNone()) return;

	bIsActive = false;

	// 결과 처리는 인벤토리 컴포넌트에 (보관이냐 즉시발동이냐 판단도 거기서)
	// 그걸 inventory에 보내는 것
	Inventory->ReceiveDrawnItem(DrawnRow);

	// 이 박스는 역할이 끝났으니 파괴
	// 리스폰(새 박스를 어디에 놓을지)은 ASpawnVolume이 타이머로 관리
	Destroy();
}

FName AFDItemPickup::DrawRandomItemRow() const
{
	if (!ItemDataTable) return NAME_None;

	// 전체 가중치 합산
	TArray<FName> RowNames = ItemDataTable->GetRowNames();
	float TotalWeight = 0.f;
	for (const FName& RowName : RowNames)
	{
		if (const FFDItemData* Data = ItemDataTable->FindRow<FFDItemData>(RowName, TEXT("뽑기 가중치 합산")))
		{
			TotalWeight += Data->DrawWeight;
		}
	}
	if (TotalWeight <= 0.f) return NAME_None;

	// 0~합계 사이 랜덤값이 어느 구간에 떨어지는지 찾기
	float Roll = FMath::FRandRange(0.f, TotalWeight);
	for (const FName& RowName : RowNames)
	{
		if (const FFDItemData* Data = ItemDataTable->FindRow<FFDItemData>(RowName, TEXT("뽑기 실행")))
		{
			Roll -= Data->DrawWeight;
			if (Roll <= 0.f) return RowName;
		}
	}
	return RowNames.Last();
}