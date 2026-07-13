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

	FPredictProjectilePathParams PathParams;
	PathParams.StartLocation = StartLoc;
	PathParams.LaunchVelocity = LaunchVelocity;
	PathParams.ProjectileRadius = 20.f;
	PathParams.OverrideGravityZ = GetWorld()->GetGravityZ() * ThrowGravityScale;
	PathParams.bTraceWithCollision = true;
	PathParams.TraceChannel = ECC_Visibility;
	PathParams.MaxSimTime = 3.f;
	PathParams.SimFrequency = 15.f;
	PathParams.DrawDebugType = EDrawDebugTrace::None;
	PathParams.ActorsToIgnore.Add(GetOwner());

	FPredictProjectilePathResult PathResult;
	const bool bHit = UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);

	// 착지 지점만 표시
	if (bHit)
	{
		DrawDebugSphere(
			GetWorld(),
			PathResult.HitResult.Location,
			30.f,               // 반지름
			16,                 // 세그먼트
			FColor::Cyan,
			false,
			-1.f);              // 매 프레임 다시 그림
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