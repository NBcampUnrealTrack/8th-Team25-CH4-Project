// FDHiderCharacter.cpp
// 숨는사람용 캐릭터

#include "Character/FDHiderCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Animation/AnimInstance.h"
#include "Net/UnrealNetwork.h"
#include "GameState/FDGameState.h"
#include "PlayerState/FDPlayerState.h"

AFDHiderCharacter::AFDHiderCharacter()
{
}

void AFDHiderCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// bIsDisguised를 모든 클라이언트에 복제 등록
	DOREPLIFETIME(AFDHiderCharacter, bIsDisguised);
}

bool AFDHiderCharacter::IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget,
	const FVector& SrcLocation) const
{
	if (const AController* ViewerController = Cast<AController>(RealViewer))
		// 액터를 컨트롤러로 캐스팅 (액터는 playerstate를 바로 호출 못하는 것 같음 에러 발생해서 변경)
	{
		if (const AFDPlayerState* FDViewerPS = ViewerController->GetPlayerState<AFDPlayerState>())
		{
			if (const AFDGameState* FDGameState = GetWorld()->GetGameState<AFDGameState>())
			{
				if (FDGameState->CurrentPhase == EMatchPhase::Scouting &&
					FDViewerPS->RoleTag == EFDRole::Tagger) 
					// 현재 Phase가 정찰 상태고 술래면 return false 한다는 얘기 (술래가 hider 캐릭터 못보게)
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
