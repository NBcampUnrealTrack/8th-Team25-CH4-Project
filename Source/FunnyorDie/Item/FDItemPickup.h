// FDItemPickup.h

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Item/FDItemTypes.h"
#include "FDItemPickup.generated.h"

UCLASS()
class FUNNYORDIE_API AFDItemPickup : public AActor
{
	GENERATED_BODY()

public:
	AFDItemPickup();

protected:
	virtual void BeginPlay() override;

	// 줍기 판정용 콜리전 (루트)
	UPROPERTY(VisibleAnywhere, Category = "Pickup")
	class USphereComponent* PickupCollision;
	
	UPROPERTY(VisibleAnywhere, Category = "Pickup")
	class UStaticMeshComponent* BoxMesh;

	// 아이템 데이터 테이블
	UPROPERTY(EditDefaultsOnly, Category = "Pickup")
	class UDataTable* ItemDataTable;
	
	UFUNCTION()
	void OnPickupOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

private:
	// 가중치 기반 랜덤 뽑기
	FName DrawRandomItemRow() const;

	// 현재 주울 수 있는 상태인지
	bool bIsActive = true;
};