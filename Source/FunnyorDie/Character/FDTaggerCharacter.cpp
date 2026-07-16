// FDTaggerCharacter.cpp
// 술래용 캐릭터 

#include "Character/FDTaggerCharacter.h"
#include "Character/FDHiderCharacter.h"
#include "Controller/FDPlayerController.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/DataTable.h"
#include "GameMode/FDGameMode.h"
#include "GameState/FDGameState.h"
#include "Customization/FDCustomizationComponent.h"
#include "Emote/FDEmoteComponent.h"
#include "Net/UnrealNetwork.h"

AFDTaggerCharacter::AFDTaggerCharacter()
{
	// 정찰 단계 감지를 위해 Tick 사용 (Pawn 기본값이 true긴 하지만 명시적으로 표기)
	PrimaryActorTick.bCanEverTick = true;

	// 포획 판정용 구체 콜리전 생성 및 루트에 부착
	CaptureCollision = CreateDefaultSubobject<USphereComponent>(TEXT("CaptureCollision"));
	CaptureCollision->SetupAttachment(RootComponent);
	CaptureCollision->SetSphereRadius(80.f);
	// 기본적으로 비활성화, 공격 입력 시에만 활성화
	CaptureCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 채색 컴포넌트 생성 - Hider 쪽 생성자에도 동일하게 추가되어 있음
	CustomizationComp = CreateDefaultSubobject<UFDCustomizationComponent>(TEXT("CustomizationComp"));

	// 이모트 컴포넌트 생성 - 마찬가지로 Hider 쪽에도 동일하게 추가됨
	EmoteComp = CreateDefaultSubobject<UFDEmoteComponent>(TEXT("EmoteComp"));

	// 카메라 붐 - 눈높이에 부착, TargetArmLength를 보간시켜 1인칭↔3인칭을 자연스럽게 오갈 수 있게 함
	// 기본값은 3인칭(ThirdPersonArmLength)이고, IA_ToggleView 입력 시 0(1인칭)까지 부드럽게 줄어듦
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetCapsuleComponent());
	CameraBoom->SetRelativeLocation(FVector(0.f, 0.f, BaseEyeHeight));
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bDoCollisionTest = true; // 벽에 카메라 파고들지 않게 자동으로 당겨줌
	CameraBoom->TargetArmLength = ThirdPersonArmLength; // 기본 3인칭으로 시작

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false; // 붐이 이미 회전을 받았으니 카메라 자체는 추가 회전 불필요

	// 마우스 좌우 회전을 몸이 그대로 따라가야 함 (1인칭이든 3인칭이든 조준 방향과 몸 방향을 일치시키기 위함)
	bUseControllerRotationYaw = true;
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->bOrientRotationToMovement = false;
	}
}

void AFDTaggerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 기본 시점은 3인칭이라 메시를 미리 숨길 필요 없음
	// 1인칭에 가까워졌을 때 메시를 숨기는 처리는 Tick에서 카메라 붐 길이를 보고 동적으로 처리함
	// (SetOwnerNoSee는 본인 화면에서만 메시를 숨기고, 다른 클라이언트 화면에는 그대로 보임)

	// 서버에서만 Overlap 이벤트 바인딩 (포획 판정은 서버 권한으로만 실행)
	if (HasAuthority())
	{
		CaptureCollision->OnComponentBeginOverlap.AddDynamic(
			this, &AFDTaggerCharacter::OnCaptureCollisionOverlap);
	}
}

void AFDTaggerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 1인칭 ↔ 3인칭 시점 전환 - 로컬(본인) 화면에서만 의미 있는 연출이라 다른 클라이언트에 영향 없음
	// 카메라 붐 길이를 목표값까지 매 프레임 보간해서 순간적으로 튀지 않고 자연스럽게 줌 인/아웃되게 함
	if (IsLocallyControlled() && CameraBoom)
	{
		const float TargetArmLength = bIsFirstPersonView ? 0.f : ThirdPersonArmLength;
		CameraBoom->TargetArmLength = FMath::FInterpTo(
			CameraBoom->TargetArmLength, TargetArmLength, DeltaSeconds, ViewTransitionSpeed);

		// 1인칭에 거의 다 왔으면(붐 길이가 거의 0이면) 본인 시점에서만 메시를 숨김
		// - 다른 클라이언트 화면에는 영향 없음 (SetOwnerNoSee는 본인 전용)
		if (USkeletalMeshComponent* SkeletalMesh = GetMesh())
		{
			const bool bShouldHideMesh = CameraBoom->TargetArmLength < 10.f;
			if (SkeletalMesh->bOwnerNoSee != bShouldHideMesh)
			{
				SkeletalMesh->SetOwnerNoSee(bShouldHideMesh);
			}
		}
	}

	// GameMode/GameState 파일을 건드리지 않기 위해, 정찰 단계 진입·종료를 여기서 직접 감지함
	// (Hider 쪽과 동일한 패턴) - 이동 속도는 서버 권한에서만 바꿔야 정상적으로 반영되므로 서버에서만 체크
	if (!HasAuthority()) return;

	const AFDGameState* FDGameState = GetWorld()->GetGameState<AFDGameState>();
	if (!FDGameState) return;

	const bool bShouldBoost = FDGameState->CurrentPhase == EMatchPhase::Scouting;
	if (bShouldBoost == bScoutModeApplied) return; // 상태 변화 없으면 아무것도 안 함

	bScoutModeApplied = bShouldBoost;
	SetScoutingMode(bShouldBoost);
}


void AFDTaggerCharacter::Server_TryCapture_Implementation()
{
	// 스턴 중엔 포획 불가
	if (bIsStunned) return;

	// 공격 입력 시 포획 콜리전을 잠깐 활성화해 Overlap 감지
	CaptureCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	// 0.2초 후 콜리전 다시 비활성화 (연속 포획 방지)
	FTimerHandle TempHandle;
	GetWorldTimerManager().SetTimer(TempHandle, [this]()
	{
		CaptureCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}, 0.2f, false);
}

void AFDTaggerCharacter::OnCaptureCollisionOverlap(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	// 이미 포획 판정 진행 중이면 중복 실행 방지
	if (CapturedHider) return;

	// ACharacter가 아니라 AFDHiderCharacter로 좁혀서 캐스팅
	// (술래나 다른 액터가 콜리전에 들어와도 포획 판정 대상이 안 되도록)
	AFDHiderCharacter* HiderCharacter = Cast<AFDHiderCharacter>(OtherActor);
	if (!HiderCharacter) return;
	
	// 봐주기로 무적 상태인 하이더는 포획 판정 자체를 시작하지 않음
	if (HiderCharacter->IsInvincible())
	{
		UE_LOG(LogTemp, Log, TEXT("[술래] 무적 상태인 하이더라 포획 판정 스킵"));
		return;
	}
	
	// 판정은 GameMode에서 하도록 
	if (AFDGameMode* FDGameMode = GetWorld()->GetAuthGameMode<AFDGameMode>())
	{
		FDGameMode->RequestCaptureJudgement(this, HiderCharacter);
	}
	
	UE_LOG(LogTemp, Warning, TEXT("[술래] 콜리전 겹침 감지"));
}

void AFDTaggerCharacter::SetScoutingMode(bool bEnable)
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement) return;

	// 밸런스 테이블에서 속도 값 조회 (플라이 관전 방식은 삭제 - 이제 걷기 속도만 올려서 맵을 둘러봄)
	float ScoutSpeed = 1200.f;
	float NormalSpeed = 600.f;
	if (BalanceDataTable)
	{
		if (const FMatchBalanceSettings* Settings =
			BalanceDataTable->FindRow<FMatchBalanceSettings>(TEXT("Default"), TEXT("정찰 속도 조회")))
		{
			ScoutSpeed = Settings->TaggerScoutSpeed;
			NormalSpeed = Settings->DefaultWalkSpeed;
		}
	}

	Movement->MaxWalkSpeed = bEnable ? ScoutSpeed : NormalSpeed;
}

void AFDTaggerCharacter::ToggleViewMode()
{
	// 로컬(본인) 화면에서만 의미 있음 - 실제 보간 처리는 Tick에서 IsLocallyControlled() 체크 후 수행됨
	bIsFirstPersonView = !bIsFirstPersonView;

	UE_LOG(LogTemp, Log, TEXT("[술래] 시점 전환 - %s"), bIsFirstPersonView ? TEXT("1인칭") : TEXT("3인칭"));
}

void AFDTaggerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AFDTaggerCharacter, bIsStunned);
}

void AFDTaggerCharacter::StartCaptureSequence(ACharacter* TargetHider)
{
	// GameMode 등 외부에서 직접 포획 시퀀스를 시작할 때 사용
	if (!HasAuthority() || !TargetHider) return;
	Internal_StartCaptureSequence(TargetHider);
	
	UE_LOG(LogTemp, Warning, TEXT("RequestCaptureJudgement 불림"));
}

void AFDTaggerCharacter::Internal_StartCaptureSequence(ACharacter* TargetHider)
{
	// 밸런스 데이터 테이블에서 포획 대기 시간 조회
	float JudgeTime = 15.f;
	if (BalanceDataTable)
	{
		if (const FMatchBalanceSettings* Settings =
			BalanceDataTable->FindRow<FMatchBalanceSettings>(TEXT("Default"), TEXT("포획 판정 대기 시간 조회")))
		{
			JudgeTime = Settings->CaptureJudgeWaitTime;
		}
	}

	CapturedHider = TargetHider;

	// 두 플레이어 모두 입력 비활성화 (판정 단계 진입)
	if (APlayerController* TaggerPC = Cast<APlayerController>(GetController()))
	{
		TaggerPC->DisableInput(TaggerPC);
	}
	if (APlayerController* HiderPC = Cast<APlayerController>(TargetHider->GetController()))
	{
		HiderPC->DisableInput(HiderPC);
	}

	UE_LOG(LogTemp, Warning, TEXT("UI 뜰 거임"));

	// 술래 화면에 포획 팝업 UI 호출 (PlayerController 연동)
	if (AFDPlayerController* FDController = Cast<AFDPlayerController>(GetController()))
	{
		FDController->Client_ShowCapturePopup();
	}

	// 포획 판정 대기 타이머 구동 (만료 시 자동 아웃)
	GetWorldTimerManager().SetTimer(
		CaptureJudgeTimerHandle,
		this,
		&AFDTaggerCharacter::OnCaptureTimerExpired,
		JudgeTime,
		false);

	UE_LOG(LogTemp, Log, TEXT("[술래] 포획 판정 시작 - 대상: %s, 대기 시간: %.1f초"),
		*TargetHider->GetName(), JudgeTime);
}

void AFDTaggerCharacter::OnCaptureTimerExpired() 
// 타이머 만료 = 술래가 선택하지 않음 → 자동 아웃 처리

{
	if (!CapturedHider) return;

	Internal_ResolveCaptureLocally(true);
}

void AFDTaggerCharacter::OnSpareExpired(ACharacter* TargetHider)
{
	// 봐주기 무적·속도 버프 만료 시 원래 상태로 복구
	if (!TargetHider) return;

	if (AFDHiderCharacter* HiderChar = Cast<AFDHiderCharacter>(TargetHider))
		// 무적 끔 (RequestSpare에 구현해놓으신 거 반대로 설정)
	{
		HiderChar->SetInvincible(false); 
	}
	
	// 이동 속도 원래대로 복구
	float DefaultSpeed = 600.f;
	if (BalanceDataTable)
	{
		if (const FMatchBalanceSettings* Settings =
			BalanceDataTable->FindRow<FMatchBalanceSettings>(TEXT("Default"), TEXT("기본 속도 조회")))
		{
			DefaultSpeed = Settings->DefaultWalkSpeed;
		}
	}
	if (UCharacterMovementComponent* Movement = TargetHider->GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = DefaultSpeed;
	}

	GetWorldTimerManager().ClearTimer(SpareExpireTimerHandle);
	
	UE_LOG(LogTemp, Log, TEXT("[술래] 봐주기 만료 - 속도·무적 복구: %s"),
		*TargetHider->GetName());
}

void AFDTaggerCharacter::Internal_ResolveCaptureLocally(bool bWasCaptured) 
// 아웃이든 봐주기든 똑같이 실행되어야 하는건 하나로 묶음 
{
	// 타이머 끄기 (결과가 정해졌으니 자동 만료 타이머 필요 없음)
	GetWorldTimerManager().ClearTimer(CaptureJudgeTimerHandle);
	
	// 양쪽 입력 원복시킴
	if (APlayerController* TaggerPC = Cast<APlayerController>(GetController()))
	{
		TaggerPC->EnableInput(TaggerPC);
		
		// 팝업이 떠 있는 상태(타임아웃 케이스)라도 확실히 닫아줌
		if (AFDPlayerController* FDTaggerPC = Cast<AFDPlayerController>(TaggerPC))
		{
			FDTaggerPC->Client_HideCapturePopup();
		}
	}
	if (APlayerController* HiderPC = Cast<APlayerController>(CapturedHider->GetController()))
	{
		HiderPC->EnableInput(HiderPC);
	}
	
	// 게임모드에 알림
	if (AFDGameMode* FDGameMode = GetWorld()->GetAuthGameMode<AFDGameMode>())
	{
		FDGameMode->ResolveCapture(CapturedHider, bWasCaptured);
	}
	
	CapturedHider = nullptr; 
}

void AFDTaggerCharacter::ForceOut() // 아웃 누르면 호출될 함수
{
	if (!CapturedHider) return; // 판정 중인 게 없으면 무시
	
	Internal_ResolveCaptureLocally(true);
}

void AFDTaggerCharacter::RequestSpare() // 봐주기 누르면 호출될 함수
{
	if (!CapturedHider) return;

	ACharacter* SparedHider = CapturedHider;

	// 밸런스 테이블에서 무적 지속 시간 / 속도 배율 조회
	float InvincibleTime = 7.f;
	float SpeedMultiplier = 1.5f;
	if (BalanceDataTable)
	{
		if (const FMatchBalanceSettings* Settings =
			BalanceDataTable->FindRow<FMatchBalanceSettings>(TEXT("Default"), TEXT("봐주기 설정 조회")))
		{
			InvincibleTime = Settings->SpareInvincibleTime;
			SpeedMultiplier = Settings->SpareSpeedMultiplier;
		}
	}

	// 봐주기 버프: 속도 증가 (기본 속도 * 배율)
	float DefaultSpeed = 600.f;
	if (BalanceDataTable)
	{
		if (const FMatchBalanceSettings* Settings =
			BalanceDataTable->FindRow<FMatchBalanceSettings>(TEXT("Default"), TEXT("기본 속도 조회")))
		{
			DefaultSpeed = Settings->DefaultWalkSpeed;
		}
	}
	if (UCharacterMovementComponent* Movement = SparedHider->GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = DefaultSpeed * SpeedMultiplier;
	}

	// 무적 처리
	if (AFDHiderCharacter* HiderChar = Cast<AFDHiderCharacter>(SparedHider))
	{
		HiderChar->SetInvincible(true);
	}

	// 테이블에서 가져온 시간 뒤 버프 해제
	GetWorldTimerManager().SetTimer(SpareExpireTimerHandle, [this, SparedHider]()
		{
			OnSpareExpired(SparedHider);
		},
		InvincibleTime, false);

	Internal_ResolveCaptureLocally(false);
}

void AFDTaggerCharacter::ApplyStun(float Duration)
{
	// 서버 권한으로만 상태 변경
	if (!HasAuthority()) return;

	// 이미 스턴 중이면 무시 (연장 안 됨)
	// 하이더 여럿이 연속으로 던져서 술래를 계속 묶어두는 걸 방지
	if (bIsStunned)
	{
		UE_LOG(LogTemp, Log, TEXT("[술래] 이미 스턴 중 - 중복 스턴 무시"));
		return;
	}

	bIsStunned = true;
	OnRep_bIsStunned(); // 서버 수동 호출

	// 이동 입력 차단 (해당 클라에게만)
	if (AFDPlayerController* FDPC = Cast<AFDPlayerController>(GetController()))
	{
		FDPC->Client_LockMovement();
	}

	// Duration 후 자동 해제
	GetWorldTimerManager().SetTimer(
		StunExpireTimerHandle,
		this,
		&AFDTaggerCharacter::OnStunExpired,
		Duration,
		false);

}

void AFDTaggerCharacter::OnStunExpired()
{
	if (!HasAuthority()) return;

	bIsStunned = false;
	OnRep_bIsStunned();

	// 이동 입력 복구
	if (AFDPlayerController* FDPC = Cast<AFDPlayerController>(GetController()))
	{
		FDPC->Client_UnlockMovement();
	}

	GetWorldTimerManager().ClearTimer(StunExpireTimerHandle);

	UE_LOG(LogTemp, Warning, TEXT("[술래] 스턴 해제"));
}

void AFDTaggerCharacter::OnRep_bIsStunned()
{
	// 스턴 이펙트(별 빙빙, 머티리얼 변화 등) 켜고 끄는 비주얼 처리는 여기에 추가
}