// FDItemInventoryComponent.cpp

#include "Item/FDItemInventoryComponent.h"
#include "Character/FDHiderCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Engine/DataTable.h"
#include "Item/FDThrowItem.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

UFDItemInventoryComponent::UFDItemInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	
	// 복제해야 하니 컴포넌트 자체 복제를 켬
	SetIsReplicatedByDefault(true);
}

void UFDItemInventoryComponent::FireThrowItem()
{
	// 조준 중이 아니면 무시
	if (!bIsAiming) return;

	// 조준 해제를 먼저
	StopAiming();

	// 서버에 발사 요청
	Server_UseItem(EFDItemEffect::TaggerStun);
}

void UFDItemInventoryComponent::ToggleAiming()
{
	if (bIsAiming)
	{
		StopAiming();
		return;
	}

	// 투사체가 없으면 조준 진입 불가
	if (ThrowItemCount <= 0)
	{
		return;
	}

	StartAiming();
}

void UFDItemInventoryComponent::StartAiming()
{
	bIsAiming = true;

	// Tick 켜기
	SetComponentTickEnabled(true);

	UE_LOG(LogTemp, Warning, TEXT("조준 시작"));
}

void UFDItemInventoryComponent::StopAiming()
{
	bIsAiming = false;

	SetComponentTickEnabled(false);

	UE_LOG(LogTemp, Warning, TEXT("조준 종료"));
}

void UFDItemInventoryComponent::GetThrowStartAndVelocity(FVector& OutStart, FVector& OutVelocity) const
{
	OutStart = FVector::ZeroVector;
	OutVelocity = FVector::ZeroVector;

	const AFDHiderCharacter* Hider = Cast<AFDHiderCharacter>(GetOwner());
	if (!Hider) return;

	// 하이더가 보는 방향
	const FRotator ControlRot = Hider->GetControlRotation();
	const FVector ThrowDir = ControlRot.Vector(); // 회전값을 방향 벡터로 변환 (제공된 함수)

	// 발사 시작점
	OutStart = Hider->GetActorLocation()
		+ ThrowDir * ThrowForwardOffset
		+ FVector(0.f, 0.f, ThrowUpOffset);

	// 발사 속도 벡터 = 방향 × 속력
	OutVelocity = ThrowDir * ThrowSpeed;
}

void UFDItemInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                              FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsAiming) return;

	UpdateTrajectory();
}

void UFDItemInventoryComponent::UpdateTrajectory()
{
	FVector StartLoc;
	FVector LaunchVelocity;
	GetThrowStartAndVelocity(StartLoc, LaunchVelocity);

	if (LaunchVelocity.IsNearlyZero()) return;

	// 궤적 예측 파라미터 설정 (제공된 함수)
	FPredictProjectilePathParams PathParams;
	PathParams.StartLocation = StartLoc;
	PathParams.LaunchVelocity = LaunchVelocity;
	PathParams.ProjectileRadius = 20.f;              // ThrowItem의 구체 반지름과 맞춤
	PathParams.OverrideGravityZ = 0.f;               // 0이면 월드 기본 중력 사용
	PathParams.bTraceWithCollision = true;           // 벽에 부딪히면 거기서 궤적 종료
	PathParams.TraceChannel = ECC_Visibility;
	PathParams.MaxSimTime = 3.f;                     // 최대 3초까지만 시뮬레이션
	PathParams.SimFrequency = 15.f;                  // 초당 15개 점으로 경로 계산
	PathParams.DrawDebugType = EDrawDebugTrace::None; // 자동 그리기 끔

	// 자기 자신은 궤적 충돌 대상에서 제외
	PathParams.ActorsToIgnore.Add(GetOwner());

	// 궤적 예측 결과를 담을 구조체
	FPredictProjectilePathResult PathResult;

	// 실제 예측 실행 (제공 함수)
	const bool bHit = UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);

	// 계산된 경로 점들을 디버그 선으로 이어서 그리기
	const TArray<FPredictProjectilePathPointData>& Points = PathResult.PathData;
	for (int32 i = 0; i < Points.Num() - 1; ++i)
	{
		DrawDebugLine(
			GetWorld(),
			Points[i].Location,
			Points[i + 1].Location,
			FColor::Cyan,
			false,      // 영구 지속 안 함
			-1.f,       // 다음 프레임에 사라짐 (매 프레임 다시 그림)
			0,
			3.f);       // 선 두께
	}

	// 착지 지점 표시 (벽/바닥에 맞은 경우)
	if (bHit)
	{
		DrawDebugSphere(GetWorld(), PathResult.HitResult.Location, 20.f, 12, FColor::Red, false, -1.f);
	}
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
		// 투명화
	case EFDItemEffect::Invisibility:
		{
			Hider->SetItemInvisible(true);

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
		}

		// 투사체
	case EFDItemEffect::TaggerStun:
		{
			if (!Data.ThrowItemClass) break;

			// 궤적 표시와 정확히 같은 계산을 사용 - 보이는 대로 날아가게 보장
			FVector StartLoc;
			FVector LaunchVelocity;
			GetThrowStartAndVelocity(StartLoc, LaunchVelocity);

			if (LaunchVelocity.IsNearlyZero()) break;

			// 발사 방향으로 회전값 설정 (속도 벡터를 회전으로 변환)
			const FRotator SpawnRot = LaunchVelocity.Rotation();

			FActorSpawnParameters Params;
			Params.Instigator = Hider;
			Params.Owner = Hider;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

			AFDThrowItem* Throw = GetWorld()->SpawnActor<AFDThrowItem>(
				Data.ThrowItemClass, StartLoc, SpawnRot, Params);

			if (Throw)
			{
				Throw->SetStunDuration(Data.Duration);
				// 궤적 계산에 쓴 것과 동일한 속도 벡터를 실제 투사체에 적용
				Throw->LaunchWith(LaunchVelocity);
			}
			break;
		}

		// 강제 이모션
	case EFDItemEffect::ForcedEmote:
		break;

		// 소리
	case EFDItemEffect::Noise:
		break;

	default:
		break;
	}
}