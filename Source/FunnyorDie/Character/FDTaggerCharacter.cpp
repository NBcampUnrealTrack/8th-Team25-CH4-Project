// FDTaggerCharacter.cpp
// 술래용 캐릭터 

#include "Character/FDTaggerCharacter.h"
#include "Character/FDHiderCharacter.h"
#include "Controller/FDPlayerController.h"
#include "Components/SphereComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/DataTable.h"

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

	ACharacter* HiderCharacter = Cast<ACharacter>(OtherActor);
	if (!HiderCharacter) return;

	// TODO: Gameplay Tag로 숨는 자 진영 확인 후 포획 시퀀스 시작
	Internal_StartCaptureSequence(HiderCharacter);
}

void AFDTaggerCharacter::StartCaptureSequence(ACharacter* TargetHider)
{
	// GameMode 등 외부에서 직접 포획 시퀀스를 시작할 때 사용
	if (!HasAuthority() || !TargetHider) return;
	Internal_StartCaptureSequence(TargetHider);
}

void AFDTaggerCharacter::Internal_StartCaptureSequence(ACharacter* TargetHider)
{
	// 밸런스 데이터 테이블에서 포획 대기 시간 조회
	float JudgeTime = 3.f;
	if (BalanceDataTable)
	{
		if (const FMatchBalanceSettings* Settings =
			BalanceDataTable->FindRow<FMatchBalanceSettings>(TEXT("Default"), TEXT("포획 판정 대기 시간 조회")))
		{
			JudgeTime = Settings->CaptureJudgeTime;
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
{
	// 타이머 만료 = 술래가 선택하지 않음 → 자동 아웃 처리
	if (!CapturedHider) return;

	// 두 플레이어 입력 복구
	if (APlayerController* TaggerPC = Cast<APlayerController>(GetController()))
	{
		TaggerPC->EnableInput(TaggerPC);
	}
	if (APlayerController* HiderPC = Cast<APlayerController>(CapturedHider->GetController()))
	{
		HiderPC->EnableInput(HiderPC);
	}

	// TODO: GameMode 생존자 카운트 감소 및 숨는 자 탈락 처리 연동
	UE_LOG(LogTemp, Log, TEXT("[술래] 포획 판정 타이머 만료 - 자동 아웃 처리: %s"),
		*CapturedHider->GetName());

	CapturedHider = nullptr;
}

void AFDTaggerCharacter::OnSpareExpired(ACharacter* TargetHider)
{
	// 봐주기 무적·속도 버프 만료 시 원래 상태로 복구
	if (!TargetHider) return;

	// TODO: bIsInvincible = false 처리 (HiderCharacter 연동)

	// 이동 속도 원래대로 복구
	if (UCharacterMovementComponent* Movement = TargetHider->GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = 600.f;
	}

	UE_LOG(LogTemp, Log, TEXT("[술래] 봐주기 만료 - 속도·무적 복구: %s"),
		*TargetHider->GetName());
}
