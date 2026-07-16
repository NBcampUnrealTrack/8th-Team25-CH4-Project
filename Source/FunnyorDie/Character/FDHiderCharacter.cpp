// FDHiderCharacter.cpp
// 숨는사람용 캐릭터

#include "Character/FDHiderCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Animation/AnimInstance.h"
#include "Net/UnrealNetwork.h"
#include "GameState/FDGameState.h"
#include "PlayerState/FDPlayerState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Customization/FDCustomizationComponent.h"
#include "Emote/FDEmoteComponent.h"
#include "Item/FDItemInventoryComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Components/AudioComponent.h" 

AFDHiderCharacter::AFDHiderCharacter()
{
	// 정찰 단계 감지를 위해 Tick 사용 (Pawn 기본값이 true긴 하지만 명시적으로 표기)
	PrimaryActorTick.bCanEverTick = true;

	// 채색 컴포넌트 생성 - Tagger 쪽 생성자에도 동일하게 추가되어 있음
	CustomizationComp = CreateDefaultSubobject<UFDCustomizationComponent>(TEXT("CustomizationComp"));

	// 이모트 컴포넌트 생성 - 마찬가지로 Tagger 쪽에도 동일하게 추가됨
	EmoteComp = CreateDefaultSubobject<UFDEmoteComponent>(TEXT("EmoteComp"));
	
	// 인벤토리 
	ItemInventoryComp = CreateDefaultSubobject<UFDItemInventoryComponent>(TEXT("ItemInventoryComp"));

	// 동상 머리 장비 메시 생성 - 머리 소켓에 붙여두고 평소엔 숨겨둠 (장착 전까지 비어있는 상태)
	HeadEquipMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadEquipMesh"));
	HeadEquipMesh->SetupAttachment(GetMesh(), HeadSocketName);
	HeadEquipMesh->SetVisibility(false);
	HeadEquipMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 장식용이라 콜리전 불필요

	// 3인칭 카메라 - SpringArm이 마우스 회전을 받고, 카메라는 그 끝에 그대로 매달림
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetCapsuleComponent());
	CameraBoom->TargetArmLength = 350.f;
	CameraBoom->SetRelativeLocation(FVector(0.f, 0.f, 60.f));
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bDoCollisionTest = true; // 벽에 카메라 파고들지 않게 자동으로 당겨줌

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false; // 붐이 이미 회전을 받았으니 카메라 자체는 추가 회전 불필요

	// 조준용 1인칭 카메라 - 캡슐 눈높이에 직결, 평소엔 비활성화 상태로 대기
	AimCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("AimCamera"));
	AimCamera->SetupAttachment(GetCapsuleComponent());
	AimCamera->SetRelativeLocation(FVector(0.f, 0.f, BaseEyeHeight));
	AimCamera->bUsePawnControlRotation = true;
	AimCamera->SetActive(false);

	// 3인칭이라 몸은 이동 방향으로 자연스럽게 도는 게 자연스러움 (기본값 유지, 명시적으로 적어둠)
	bUseControllerRotationYaw = false;
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement = true;
		MoveComp->RotationRate = FRotator(0.f, 540.f, 0.f);
	}
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

	// 동상 머리 장비
	DOREPLIFETIME(AFDHiderCharacter, EquippedHeadRow);
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

void AFDHiderCharacter::Multicast_PlayNoise_Implementation(float Duration)
{
	if (!NoiseSound)
	{
		UE_LOG(LogTemp, Warning, TEXT("NoiseSound가 할당되지 않음"));
		return;
	}

	// 이미 소리가 나고 있으면 먼저 정지 (중복 재생 방지)
	StopNoise();

	// 리턴값을 붙잡아둬야 나중에 Stop()을 부를 수 있음
	// bAutoDestroy를 false로
	ActiveNoiseAudio = UGameplayStatics::SpawnSoundAttached(
		NoiseSound,
		GetRootComponent(),
		NAME_None,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		EAttachLocation::SnapToTarget,
		true,       // bStopWhenAttachedToDestroyed
		1.f,        // VolumeMultiplier
		1.f,        // PitchMultiplier
		0.f,        // StartTime
		nullptr,    // AttenuationSettings
		nullptr,    // ConcurrencySettings
		false       // bAutoDestroy ← false! 우리가 Stop을 부를 때까지 살아있어야 함
	);

	if (!ActiveNoiseAudio)
	{
		return;
	}

	// Duration 뒤 자동 정지
	GetWorldTimerManager().SetTimer(
		NoiseStopTimerHandle,
		this,
		&AFDHiderCharacter::StopNoise,
		Duration,
		false);

}

void AFDHiderCharacter::StopNoise()
{
	if (ActiveNoiseAudio)
	{
		ActiveNoiseAudio->Stop();
		ActiveNoiseAudio = nullptr;

		UE_LOG(LogTemp, Warning, TEXT("소리 정지"));
	}

	GetWorldTimerManager().ClearTimer(NoiseStopTimerHandle);
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

	// 늦은 조인 등으로 EquippedHeadRow가 BeginPlay 이전에 이미 복제돼있을 수 있어서 한 번 수동 적용
	if (!EquippedHeadRow.IsNone())
	{
		OnRep_EquippedHeadRow();
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

void AFDHiderCharacter::SetAimCameraMode(bool bAiming)
{
	// 로컬(본인) 클라이언트 화면에서만 의미 있는 연출 - 남의 화면에서 내가 어떻게 보이는지랑은 무관
	if (!IsLocallyControlled()) return;

	if (!FollowCamera || !AimCamera) return;

	FollowCamera->SetActive(!bAiming);
	AimCamera->SetActive(bAiming);

	// 1인칭으로 줌인했는데 자기 머리/몸이 화면에 걸리면 어색하니까 본인 시점에서만 메시 숨김
	// SetOwnerNoSee는 로컬 소유 시점에만 적용되고 다른 클라이언트 화면엔 영향 없음
	if (USkeletalMeshComponent* SkeletalMesh = GetMesh())
	{
		SkeletalMesh->SetOwnerNoSee(bAiming);
	}
}

void AFDHiderCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// GameMode/GameState 파일을 건드리지 않기 위해, 정찰 단계 진입·종료를 여기서 직접 감지함
	// 이동 속도는 서버 권한에서만 바꿔야 정상적으로 반영되므로 서버에서만 체크
	if (!HasAuthority()) return;

	const AFDGameState* FDGameState = GetWorld()->GetGameState<AFDGameState>();
	if (!FDGameState) return;

	const bool bShouldBoost = FDGameState->CurrentPhase == EMatchPhase::Scouting;
	if (bShouldBoost == bScoutSpeedBoostApplied) return; // 상태 변화 없으면 아무것도 안 함

	bScoutSpeedBoostApplied = bShouldBoost;
	SetScoutSpeedBoost(bShouldBoost);
}

void AFDHiderCharacter::SetScoutSpeedBoost(bool bEnable)
{
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp) return;

	// 밸런스 테이블에서 정찰 속도 조회 (Tagger의 정찰 속도 필드를 그대로 공유해서 사용)
	float ScoutSpeed = 1200.f;
	if (const FMatchBalanceSettings* Settings = GetBalanceSettings())
	{
		ScoutSpeed = Settings->TaggerScoutSpeed;
	}

	// 정찰 단계: 속도 버프 적용 / 본게임 시작: BeginPlay에서 저장해둔 기본 속도로 복귀
	MoveComp->MaxWalkSpeed = bEnable ? ScoutSpeed : DefaultWalkSpeed;

	UE_LOG(LogTemp, Log, TEXT("[숨는자] 정찰 속도 버프 %s - 속도: %.1f"),
		bEnable ? TEXT("적용") : TEXT("해제"), MoveComp->MaxWalkSpeed);
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

	// TODO: 투명화 중엔 동상 머리 장비(HeadEquipMesh)도 같이 숨겨야 할지 기획 확인 필요
	// 지금은 몸은 안 보이는데 머리 장비만 둥둥 떠있는 것처럼 보일 수 있음
}

void AFDHiderCharacter::Server_RequestEquipHead_Implementation(FName HeadRowName)
{
	// TODO: 위장 중(bIsDisguised)이면 장비 불가 처리할지 기획 확인 필요
	// - 위장 캡슐 상태에서 머리 장비까지 같이 보이면 어색할 수 있음

	if (!HeadEquipDataTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("[숨는자] 동상 머리 데이터 테이블이 할당되지 않음"));
		return;
	}

	// 유효한 행인지 서버에서 먼저 검증 (클라이언트가 존재하지 않는 이름을 보내는 경우 방어)
	if (!HeadEquipDataTable->FindRow<FFDHeadEquipData>(HeadRowName, TEXT("동상 머리 유효성 검사")))
	{
		UE_LOG(LogTemp, Warning, TEXT("[숨는자] 존재하지 않는 동상 머리 요청: %s"), *HeadRowName.ToString());
		return;
	}

	EquippedHeadRow = HeadRowName;

	// 서버 자기 자신은 OnRep이 자동 호출되지 않으므로 수동 호출 (기존 패턴과 동일)
	OnRep_EquippedHeadRow();

	UE_LOG(LogTemp, Log, TEXT("[숨는자] 동상 머리 장착 - %s"), *HeadRowName.ToString());
}

void AFDHiderCharacter::OnRep_EquippedHeadRow()
{
	if (!HeadEquipMesh) return;

	if (EquippedHeadRow.IsNone() || !HeadEquipDataTable)
	{
		HeadEquipMesh->SetVisibility(false);
		return;
	}

	const FFDHeadEquipData* Data = HeadEquipDataTable->FindRow<FFDHeadEquipData>(EquippedHeadRow, TEXT("동상 머리 조회"));
	if (!Data)
	{
		HeadEquipMesh->SetVisibility(false);
		return;
	}

	// TSoftObjectPtr라 동기 로드 필요 - 자주 안 바뀌는 리소스라 비동기 스트리밍까지는 안 해도 될 듯
	if (UStaticMesh* LoadedHeadMesh = Data->HeadMesh.LoadSynchronous())
	{
		HeadEquipMesh->SetStaticMesh(LoadedHeadMesh);
		HeadEquipMesh->SetRelativeTransform(Data->AttachOffset);
		HeadEquipMesh->SetVisibility(true);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[숨는자] 동상 머리 메시 로드 실패: %s"), *EquippedHeadRow.ToString());
		HeadEquipMesh->SetVisibility(false);
	}
}
