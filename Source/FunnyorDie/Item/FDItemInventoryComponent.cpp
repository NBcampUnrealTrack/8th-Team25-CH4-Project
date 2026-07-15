// FDItemInventoryComponent.cpp

#include "Item/FDItemInventoryComponent.h"
#include "Character/FDHiderCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Engine/DataTable.h"
#include "Item/FDThrowItem.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Components/DecalComponent.h"
#include "Blueprint/UserWidget.h"

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
	
	// 조준 데칼 생성
	if (!ActiveAimDecal && AimDecalMaterial)
	{
		ActiveAimDecal = UGameplayStatics::SpawnDecalAtLocation(
			GetWorld(),
			AimDecalMaterial,
			AimDecalSize,
			FVector::ZeroVector,                // 위치는 매 프레임 갱신할 거라 일단 원점
			FRotator(-90.f, 0.f, 0.f),          // 아래를 향하게 (바닥에 투영)
			0.f                                  // LifeSpan 0 = 무한 (직접 제거)
		);
	}
	
	// 조준선 위젯 표시
	AFDHiderCharacter* Hider = Cast<AFDHiderCharacter>(GetOwner());
	if (Hider && Hider->IsLocallyControlled() && !ActiveCrosshair && AimCrosshairWidgetClass)
	{
		if (APlayerController* PC = Cast<APlayerController>(Hider->GetController()))
		{
			ActiveCrosshair = CreateWidget<UUserWidget>(PC, AimCrosshairWidgetClass);
			if (ActiveCrosshair)
			{
				ActiveCrosshair->AddToViewport();
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("조준 시작"));
}

void UFDItemInventoryComponent::StopAiming()
{
	bIsAiming = false;
	SetComponentTickEnabled(false);
	
	// 조준 데칼 제거
	if (ActiveAimDecal)
	{
		ActiveAimDecal->DestroyComponent();
		ActiveAimDecal = nullptr;
	}

	// 조준선 위젯 제거
	if (ActiveCrosshair)
	{
		ActiveCrosshair->RemoveFromParent();
		ActiveCrosshair = nullptr;
	}
	
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

	// 착지 지점에 조준 데칼 표시
	if (bHit && ActiveAimDecal)
	{
		// 표면에서 살짝 띄워서 배치
		const FVector DecalLoc = PathResult.HitResult.Location
			+ PathResult.HitResult.ImpactNormal * 50.f;

		ActiveAimDecal->SetWorldLocation(DecalLoc);

		// 표면 법선을 따라 눕히기 (경사면에도 자연스럽게 붙음)
		const FRotator DecalRot = (-PathResult.HitResult.ImpactNormal).Rotation();
		ActiveAimDecal->SetWorldRotation(DecalRot);

		ActiveAimDecal->SetVisibility(true);
	}
	else if (ActiveAimDecal)
	{
		// 아무데도 안 맞으면 (허공을 향할 때) 데칼 숨김
		ActiveAimDecal->SetVisibility(false);
	}
}

void UFDItemInventoryComponent::Client_NotifyItemAcquired_Implementation(EFDItemEffect Which)
{
	// 획득 알림 방송
	OnItemAcquired.Broadcast(Which);
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
			Client_NotifyInventoryFull(EFDItemEffect::Invisibility);
			return;
		}
		++InvisibilityCount;
		OnRep_Inventory();
		Client_NotifyItemAcquired(EFDItemEffect::Invisibility);
		break;

	case EFDItemEffect::TaggerStun:
		if (ThrowItemCount >= MaxThrowItem)
		{
			Client_NotifyInventoryFull(EFDItemEffect::TaggerStun);
			return;
		}
		++ThrowItemCount;
		OnRep_Inventory();
		Client_NotifyItemAcquired(EFDItemEffect::TaggerStun);
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
	// 한도 초과 방송
	OnInventoryFull.Broadcast(Which);
}

void UFDItemInventoryComponent::OnRep_Inventory()
{
	// 개수가 바뀌었다고 방송
	OnInventoryChanged.Broadcast();
}

void UFDItemInventoryComponent::SetupLocalUI()
{
	// 이미 만들어져 있으면 중복 생성 방지
	// (PossessedBy/OnRep_PlayerState 두 경로가 겹쳐 불릴 수 있어서 필요)
	if (InventoryWidget) return;

	// 로컬 플레이어 화면에만 UI 생성
	AFDHiderCharacter* Hider = Cast<AFDHiderCharacter>(GetOwner());
	if (!Hider || !Hider->IsLocallyControlled()) return;

	APlayerController* PC = Cast<APlayerController>(Hider->GetController());
	if (!PC) return;

	// 인벤토리 UI - 계속 떠있음
	if (InventoryWidgetClass)
	{
		InventoryWidget = CreateWidget<UUserWidget>(PC, InventoryWidgetClass);
		if (InventoryWidget)
		{
			InventoryWidget->AddToViewport();
		}
	}

	// 알림 UI - 계속 떠있되 평소엔 숨김 (WBP 안에서 제어)
	if (NotifyWidgetClass)
	{
		NotifyWidget = CreateWidget<UUserWidget>(PC, NotifyWidgetClass);
		if (NotifyWidget)
		{
			NotifyWidget->AddToViewport();
		}
	}
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

		// 소리
	case EFDItemEffect::Noise:
		{
			Hider->Multicast_PlayNoise(Data.Duration);
			break;
		}

	default:
		break;
	}
}
