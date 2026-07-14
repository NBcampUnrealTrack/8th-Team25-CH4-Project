// FDSpawnVolume.cpp

#include "Item/FDSpawnVolume.h"
#include "Item/FDItemPickup.h"
#include "Components/BoxComponent.h"

AFDSpawnVolume::AFDSpawnVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	Scene = CreateDefaultSubobject<USceneComponent>(TEXT("Scene"));
	SetRootComponent(Scene);

	SpawningBox = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawningBox"));
	SpawningBox->SetupAttachment(Scene);
	SpawningBox->SetCollisionEnabled(ECollisionEnabled::NoCollision); 
	// 범위 표시용 (충돌 판정 없음)
}

void AFDSpawnVolume::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority()) return;

	GetWorldTimerManager().SetTimer(
		SpawnTimerHandle,
		this,
		&AFDSpawnVolume::TrySpawnItem,
		SpawnInterval,
		true); // 반복 실행
}

FVector AFDSpawnVolume::GetRandomPointInVolume() const
{
	const FVector BoxExtent = SpawningBox->GetScaledBoxExtent();
	const FVector BoxOrigin = SpawningBox->GetComponentLocation();

	return BoxOrigin + FVector(
		FMath::FRandRange(-BoxExtent.X, BoxExtent.X),
		FMath::FRandRange(-BoxExtent.Y, BoxExtent.Y),
		FMath::FRandRange(-BoxExtent.Z, BoxExtent.Z)
	);
}

void AFDSpawnVolume::TrySpawnItem()
{
	if (!ItemPickupClass) return;

	// 현재 살아있는 박스 개수 세기 (Destroy된 것들은 nullptr이라 자동 제외됨)
	int32 AliveCount = 0;
	for (AFDItemPickup* Item : SpawnedItems)
	{
		if (IsValid(Item))
		{
			++AliveCount;
		}
	}

	// 이미 가득 찼으면 건너뜀
	if (AliveCount >= MaxActiveItems) return;

	// 액터를 어떻게 스폰할지?
	FActorSpawnParameters SpawnParams;
	// 스폰하려는 위치에 다른 물체가 있다면 
	// 겹치면 살짝 위치를 밀어서라도 시도해보고 그래도 안 되면 그냥 억지로라도 스폰은 시켜라
	SpawnParams.SpawnCollisionHandlingOverride 
	= ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// 실제 스폰시킴
	AFDItemPickup* NewItem = GetWorld()->SpawnActor<AFDItemPickup>(
		ItemPickupClass,
		GetRandomPointInVolume(),
		FRotator::ZeroRotator,
		SpawnParams);

	// TrySpawnItem에서 내가 얼마나 박스를 생성했는지 알아야하니까 만든 박스가 유효하면 spawnitem에 등록하는 거
	if (NewItem)
	{
		SpawnedItems.Add(NewItem);
	}
}