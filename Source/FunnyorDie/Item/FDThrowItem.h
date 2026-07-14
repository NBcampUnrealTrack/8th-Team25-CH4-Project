// FDThrowItem.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FDThrowItem.generated.h"

class USphereComponent;
class UProjectileMovementComponent;

UCLASS()
class FUNNYORDIE_API AFDThrowItem : public AActor
{
	GENERATED_BODY()
	
public:	
	AFDThrowItem();
	
	// 스턴 지속시간 (인벤토리에서 데이터 테이블 Duration을 넘겨받아 설정)
	void SetStunDuration(float Duration) { StunDuration = Duration; }
	
	// 발사 속도 벡터를 직접 지정 (인벤토리가 궤적 계산에 쓴 것과 같은 값을 넘김)
	void LaunchWith(const FVector& LaunchVelocity);
	
protected:
	virtual void BeginPlay() override;

	// 충돌 판정용 구체
	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	USphereComponent* CollisionSphere;

	// 투사체 외형
	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	class UStaticMeshComponent* ProjectileMesh;

	// 포물선 이동 처리 (제공된 UProjectileMovementComponent 사용)
	// 초기 속도만 주면 중력 받아 알아서 날아감
	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	UProjectileMovementComponent* ProjectileMovement;

	// 맞았을 때 호출되는 콜백 (Hit 이벤트)
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

private:
	// 스턴 지속시간
	// 스폰 시 인벤토리가 SetStunDuration으로 설정
	float StunDuration = 5.f;
};