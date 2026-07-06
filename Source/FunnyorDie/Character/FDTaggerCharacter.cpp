// FDTaggerCharacter.cpp
// 술래용 캐릭터 

#include "Character/FDTaggerCharacter.h"
#include "Character/FDHiderCharacter.h"
#include "Controller/FDPlayerController.h"
#include "Components/SphereComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/DataTable.h"
#include "GameMode/FDGameMode.h"

AFDTaggerCharacter::AFDTaggerCharacter()
{
	// 포획 판정용 구체 콜리전 생성 및 루트에 부착
	CaptureCollision = CreateDefaultSubobject<USphereComponent>(TEXT("CaptureCollision"));
	CaptureCollision->SetupAttachment(RootComponent);
	CaptureCollision->SetSphereRadius(80.f);
	// 기본적으로 비활성화, 공격 입력 시에만 활성화
	CaptureCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AFDTaggerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 서버에서만 Overlap 이벤트 바인딩 (포획 판정은 서버 권한으로만 실행)
	if (HasAuthority())
	{
		CaptureCollision->OnComponentBeginOverlap.AddDynamic(
			this, &AFDTaggerCharacter::OnCaptureCollisionOverlap);
	}
}


void AFDTaggerCharacter::Server_TryCapture_Implementation()
{
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
	
	if (bEnable)
	{
		Movement->MaxWalkSpeed = 1200.f;
	}
	else
	{
		Movement->MaxWalkSpeed = 600.0f;
	}
	
	Multicast_SetMeshVisibility(!bEnable);
	// 관전 모드 진입하면 메시 숨겨야 하니까 반대값 전달 (false가 전달됨)
}

void AFDTaggerCharacter::Multicast_SetMeshVisibility_Implementation(bool bVisible)
{
	GetMesh()->SetVisibility(bVisible, true);
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
	if (UCharacterMovementComponent* Movement = TargetHider->GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = 600.f;
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
	if (UCharacterMovementComponent* Movement = SparedHider->GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = 600.f * SpeedMultiplier;
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
