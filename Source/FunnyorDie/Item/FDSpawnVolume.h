// FDSpawnVolume.h

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FDSpawnVolume.generated.h"

class UBoxComponent;
class AFDItemPickup;

UCLASS()
class FUNNYORDIE_API AFDSpawnVolume : public AActor
{
	GENERATED_BODY()

public:
	AFDSpawnVolume();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Spawning")
	USceneComponent* Scene;

	UPROPERTY(VisibleAnywhere, Category = "Spawning")
	UBoxComponent* SpawningBox;

	// 스폰할 아이템 박스 클래스 - 에디터에서 AFDItemPickup 설정해야함
	UPROPERTY(EditAnywhere, Category = "Spawning")
	TSubclassOf<AFDItemPickup> ItemPickupClass;

	// 동시에 존재할 수 있는 최대 박스 개수
	UPROPERTY(EditAnywhere, Category = "Spawning")
	int32 MaxActiveItems = 3;

	// 몇 초마다 스폰을 시도할지
	UPROPERTY(EditAnywhere, Category = "Spawning")
	float SpawnInterval = 10.f;

	// 구역 내부 랜덤 좌표 계산
	FVector GetRandomPointInVolume() const;
	
private:
	// 조건 확인 후 스폰 시도
	void TrySpawnItem();

	// 이 SpawnVolume이 둔 박스들 - 개수 추적용
	// UPROPERTY로 선언해야 박스가 Destroy될 때 배열 안 포인터가 자동으로 nullptr 처리됨 (GC 연동)
	UPROPERTY()
	TArray<AFDItemPickup*> SpawnedItems;

	FTimerHandle SpawnTimerHandle;
};