// FDThrowItem.cpp

#include "Item/FDThrowItem.h"
#include "Character/FDTaggerCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

AFDThrowItem::AFDThrowItem()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true; // 서버가 스폰하면 클라이언트에도 날아가는 게 보이게

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	SetRootComponent(CollisionSphere);
	CollisionSphere->SetSphereRadius(20.f);
	
	// 프리셋: 막히는 충돌이어야 Hit 이벤트가 발생함
	CollisionSphere->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	
	// 물리 시뮬레이션 콜리전에서 Hit 이벤트를 받겠다는 설정
	CollisionSphere->SetNotifyRigidBodyCollision(true);

	// 외형 메시 - 콜리전에 부착, 자체 충돌은 없음 (판정은 구체가 담당)
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 포물선 이동 컴포넌트
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = 2500.f;   // 발사 초기 속도
	ProjectileMovement->MaxSpeed = 2500.f;
	ProjectileMovement->ProjectileGravityScale = 0.5f; // 1이면 정상 중력 (포물선), 0이면 직선
	ProjectileMovement->bRotationFollowsVelocity = true; // 날아가는 방향으로 메시가 회전
}

void AFDThrowItem::LaunchWith(const FVector& LaunchVelocity)
{
	if (!ProjectileMovement) return;

	// 생성자에서 설정한 InitialSpeed 대신 인벤토리가 계산한 속도 벡터를 직접 적용
	// 이렇게 해야 조준 궤적과 실제 경로가 정확히 일치함
	ProjectileMovement->Velocity = LaunchVelocity;
}

void AFDThrowItem::BeginPlay()
{
	Super::BeginPlay();

	// Hit 판정은 서버 권한으로만
	if (HasAuthority())
	{
		CollisionSphere->OnComponentHit.AddDynamic(this, &AFDThrowItem::OnHit);
	}
}

void AFDThrowItem::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// 맞은 대상이 술래인지 확인
	AFDTaggerCharacter* Tagger = Cast<AFDTaggerCharacter>(OtherActor);
	if (Tagger)
	{
		// 스턴 적용은 술래 자신이 관리 (타이머 포함)
		// 투사체는 바로 Destroy되니까 타이머를 여기서 걸면 같이 사라져버림
		Tagger->ApplyStun(StunDuration);
		
		UE_LOG(LogTemp, Warning, TEXT("술래 맞춤]"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("못맞춤]"));
	}

	// 술래든 벽이든 한 번 맞으면 투사체는 사라짐
	Destroy();
}