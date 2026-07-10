// FDHiderCharacter.cpp
// 숨는사람용 캐릭터

#include "Character/FDHiderCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Animation/AnimInstance.h"
#include "Net/UnrealNetwork.h"
#include "GameState/FDGameState.h"
#include "PlayerState/FDPlayerState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Customization/FDCustomizationComponent.h"
#include "Emote/FDEmoteComponent.h"
#include "Item/FDItemInventoryComponent.h"

AFDHiderCharacter::AFDHiderCharacter()
{
	// 채색 컴포넌트 생성 - Tagger 쪽 생성자에도 동일하게 추가되어 있음
	CustomizationComp = CreateDefaultSubobject<UFDCustomizationComponent>(TEXT("CustomizationComp"));

	// 이모트 컴포넌트 생성 - 마찬가지로 Tagger 쪽에도 동일하게 추가됨
	EmoteComp = CreateDefaultSubobject<UFDEmoteComponent>(TEXT("EmoteComp"));
	
	// 인벤토리 
	ItemInventoryComp = CreateDefaultSubobject<UFDItemInventoryComponent>(TEXT("ItemInventoryComp"));
}

void AFDHiderCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// bIsDisguised를 모든 클라이언트에 복제 등록
	DOREPLIFETIME(AFDHiderCharacter, bIsDisguised);

	// bIsInvincible을 모든 클라이언트에 복제 등록
	DOREPLIFETIME(AFDHiderCharacter, bIsInvincible);
	
	// 투명화 아이템
	DOREPLIFETIME(AFDHiderCharacter, bIsItemInvisible);
}

bool AFDHiderCharacter::IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const
{
	// 액터를 컨트롤러로 캐스팅 (액터는 playerstate를 바로 호출 못하는 것 같음 에러 발생해서 변경)
	if (const AController* ViewerController = Cast<AController>(RealViewer))
	{
		if (const AFDPlayerState* FDViewerPS = ViewerController->GetPlayerState<AFDPlayerState>())
		{
			if (const AFDGameState* FDGameState = GetWorld()->GetGameState<AFDGameState>())
			{
				// 현재 Phase가 정찰 상태고 술래면 return false 한다는 얘기 (술래가 hider 캐릭터 못보게)
				if (FDGameState->CurrentPhase == EMatchPhase::Scouting &&
					FDViewerPS->RoleTag == EFDRole::Tagger)
				{
					return false;
				}
			}
		}
	}

	return Super::IsNetRelevantFor(RealViewer, ViewTarget, SrcLocation);
}

void AFDHiderCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 기본 캡슐 크기 저장 (위장 해제 시 복구에 사용)
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		OriginalCapsuleRadius = Capsule->GetUnscaledCapsuleRadius();
		OriginalCapsuleHalfHeight = Capsule->GetUnscaledCapsuleHalfHeight();
	}

	// 기본 이동속도 저장 (무르기 속도 버프 해제 시 복구에 사용)
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		DefaultWalkSpeed = MoveComp->MaxWalkSpeed;
	}

	// CustomizationComp/EmoteComp의 BeginPlay는 컴포넌트 자체 라이프사이클에서 자동 호출되니까
	// 여기서 따로 호출할 필요 없음
}

void AFDHiderCharacter::Server_EnterDisguise_Implementation(FName DisguiseRowName)
{
	// 데이터 테이블에서 위장 사물의 크기 데이터 조회
	if (!DisguiseDataTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("[숨는자] 위장 데이터 테이블이 할당되지 않음"));
		return;
	}

	const FDisguiseShapeData* ShapeData = DisguiseDataTable->FindRow<FDisguiseShapeData>(
		DisguiseRowName, TEXT("위장 크기 데이터 조회")
	);

	if (!ShapeData)
	{
		UE_LOG(LogTemp, Warning, TEXT("[숨는자] 행 이름 '%s'에 해당하는 위장 데이터 없음"), *DisguiseRowName.ToString());
		return;
	}

	// 서버에서 위장 상태 플래그 변경 (OnRep_bIsDisguised 복제로 클라이언트에 전파)
	bIsDisguised = true;

	// 애니메이션 정지를 모든 클라이언트에 동기화
	Multicast_FreezeAnimation(true);

	// 캡슐 크기 변경을 모든 클라이언트에 동기화
	Multicast_ResizeCapsule(ShapeData->CapsuleRadius, ShapeData->CapsuleHalfHeight);

	UE_LOG(LogTemp, Log, TEXT("[숨는자] 위장 진입 - 반지름: %.1f, 반높이: %.1f"),
		ShapeData->CapsuleRadius, ShapeData->CapsuleHalfHeight);
}

void AFDHiderCharacter::Server_ExitDisguise_Implementation()
{
	// 위장 상태 플래그 해제 (OnRep_bIsDisguised 복제로 클라이언트에 전파)
	bIsDisguised = false;

	// 애니메이션 정지 해제를 모든 클라이언트에 동기화
	Multicast_FreezeAnimation(false);

	// 원래 캡슐 크기로 복구를 모든 클라이언트에 동기화
	Multicast_ResizeCapsule(OriginalCapsuleRadius, OriginalCapsuleHalfHeight);

	UE_LOG(LogTemp, Log, TEXT("[숨는자] 위장 해제 - 원래 크기로 복구"));
}

void AFDHiderCharacter::OnRep_bIsDisguised()
{
	// 서버에서 bIsDisguised 복제 완료 시 클라이언트에서 자동 호출
	// Multicast로 이미 처리되므로 추가 비주얼 처리가 필요할 경우 여기에 작성
	UE_LOG(LogTemp, Log, TEXT("[숨는자] 위장 상태 복제 수신 - 현재 위장 중: %s"),
		bIsDisguised ? TEXT("true") : TEXT("false"));
}

void AFDHiderCharacter::SetInvincible(bool bNewInvincible)
{
	// 서버 권한으로만 상태 변경 (클라이언트가 직접 호출하면 안 됨)
	if (!HasAuthority()) return;

	bIsInvincible = bNewInvincible;

	// 서버 자기 자신은 OnRep이 자동 호출되지 않으므로 수동 호출
	OnRep_bIsInvincible();

	// 이동속도 컴포넌트 가져오기
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp) return;

	if (bNewInvincible)
	{
		// 무적 시작: 데이터 테이블에서 배율을 가져와 기본 속도에 곱해서 적용
		float SpeedMultiplier = 1.5f; // 데이터 테이블 조회 실패 시 사용할 기본값

		if (const FMatchBalanceSettings* Settings = GetBalanceSettings())
		{
			SpeedMultiplier = Settings->SpareSpeedMultiplier;
		}

		MoveComp->MaxWalkSpeed = DefaultWalkSpeed * SpeedMultiplier;

		UE_LOG(LogTemp, Log, TEXT("[숨는자] 무르기 속도 버프 적용 - 배율: %.2f, 최종 속도: %.1f"),
			SpeedMultiplier, MoveComp->MaxWalkSpeed);
	}
	else
	{
		// 무적 종료: 원래 속도로 복구
		MoveComp->MaxWalkSpeed = DefaultWalkSpeed;

		UE_LOG(LogTemp, Log, TEXT("[숨는자] 무르기 속도 버프 해제 - 원래 속도로 복구: %.1f"),
			DefaultWalkSpeed);
	}
}

void AFDHiderCharacter::OnRep_bIsInvincible()
{
	// 서버에서 bIsInvincible 복제 완료 시 클라이언트에서 자동 호출
	// TODO: 무적 이펙트(파티클, 머티리얼 아웃라인 등) 켜고 끄는 비주얼 처리는 여기에 추가
	UE_LOG(LogTemp, Log, TEXT("[숨는자] 무적 상태 변경 - 현재 무적: %s"),
		bIsInvincible ? TEXT("true") : TEXT("false"));
}

void AFDHiderCharacter::Multicast_FreezeAnimation_Implementation(bool bFreeze)
{
	// 위장 상태 진입·해제에 따라 애님 인스턴스 재생 속도 조절
	USkeletalMeshComponent* SkeletalMesh = GetMesh();
	if (!SkeletalMesh) return;

	UAnimInstance* AnimInstance = SkeletalMesh->GetAnimInstance();
	if (!AnimInstance) return;

	// bFreeze true: 위장 중 애니메이션 정지, false: 해제 후 정상 속도 복구
	SkeletalMesh->SetPlayRate(bFreeze ? 0.f : 1.f);

	UE_LOG(LogTemp, Log, TEXT("[숨는자] 애니메이션 재생 속도 변경 - PlayRate: %.1f"),
		bFreeze ? 0.f : 1.f);
}

void AFDHiderCharacter::Multicast_ResizeCapsule_Implementation(float NewRadius, float NewHalfHeight)
{
	// 위장 사물 크기에 맞춰 캡슐 콜리전 실시간 변경
	UCapsuleComponent* Capsule = GetCapsuleComponent();
	if (!Capsule) return;

	Capsule->SetCapsuleSize(NewRadius, NewHalfHeight);

	UE_LOG(LogTemp, Log, TEXT("[숨는자] 캡슐 크기 변경 - 반지름: %.1f, 반높이: %.1f"),
		NewRadius, NewHalfHeight);
}

void AFDHiderCharacter::OnCaptureOverlap()
{
	// 술래가 포획 범위 내에 들어왔을 때 처리
}

const FMatchBalanceSettings* AFDHiderCharacter::GetBalanceSettings() const
{
	if (!BalanceDataTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("[숨는자] 밸런스 데이터 테이블이 할당되지 않음"));
		return nullptr;
	}

	return BalanceDataTable->FindRow<FMatchBalanceSettings>(
		TEXT("Default"), TEXT("밸런스 설정 조회")
	);
}

void AFDHiderCharacter::SetItemInvisible(bool bNewInvisible)
{
	if (!HasAuthority()) return;

	bIsItemInvisible = bNewInvisible;

	// 서버니까 직접 호출
	OnRep_bIsItemInvisible();
}

void AFDHiderCharacter::OnRep_bIsItemInvisible()
{
	// 투명화 상태에 따라 메시 표시/숨김
	// SetVisibility(false)는 눈에만 안 보임 
	// 콜리전은 그대로라 술래가 부딪히면 위치 노출됨
	USkeletalMeshComponent* SkeletalMesh = GetMesh();
	if (!SkeletalMesh) return;

	// 투명화 상태니까 bIsItemInvisible은 true인 상태 
	// 근데 메시는 안보여야 하니까 ! 붙인 것
	SkeletalMesh->SetVisibility(!bIsItemInvisible, true);
}