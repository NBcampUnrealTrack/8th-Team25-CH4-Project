// PlayerController.cpp

#include "Controller/FDPlayerController.h"
#include "Character/FDTaggerCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

void AFDPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 로컬 플레이어에게만 인풋 매핑 컨텍스트 등록
	if (!IsLocalController()) return;

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (DefaultMappingContext)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void AFDPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EIC) return;

	// 이동 입력 바인딩
	if (IA_Move)
	{
		EIC->BindAction(IA_Move, ETriggerEvent::Triggered, this, &AFDPlayerController::Input_Move);
	}
	
	// 점프 입력 바인딩
	if (IA_Jump)
	{
		EIC->BindAction(IA_Jump, ETriggerEvent::Started, this, &AFDPlayerController::Input_Jump);
	}

	// 카메라 회전 입력 바인딩
	if (IA_Look)
	{
		EIC->BindAction(IA_Look, ETriggerEvent::Triggered, this, &AFDPlayerController::Input_Look);
	}

	// 마우스 좌클릭 공격 입력 바인딩
	if (IA_Attack)
	{
		EIC->BindAction(IA_Attack, ETriggerEvent::Started, this, &AFDPlayerController::Input_Attack);
	}

	// TODO: 봐주기 키 확정 후 IA_Spare 바인딩 추가
}

void AFDPlayerController::Input_Move(const FInputActionValue& Value)
{
	// WASD 이동 입력을 캐릭터에 전달
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn) return;

	const FVector2D MoveInput = Value.Get<FVector2D>();

	// 컨트롤러 회전 기준으로 전후좌우 이동 방향 계산
	const FRotator YawRotation(0.f, GetControlRotation().Yaw, 0.f);
	const FVector ForwardDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	ControlledPawn->AddMovementInput(ForwardDir, MoveInput.Y);
	ControlledPawn->AddMovementInput(RightDir, MoveInput.X);
}

void AFDPlayerController::Input_Jump(const FInputActionValue& Value)
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn) return;
	
	// 스페이스바 입력 시 캐릭터 점프
	if (ACharacter* ControlledCharacter = Cast<ACharacter>(ControlledPawn))
	{
		ControlledCharacter->Jump();
	}
}

void AFDPlayerController::Input_Look(const FInputActionValue& Value)
{
	// 마우스 움직임으로 카메라 회전 (정찰 단계에서도 카메라는 허용)
	const FVector2D LookInput = Value.Get<FVector2D>();
	AddYawInput(LookInput.X);
	AddPitchInput(LookInput.Y);
}

void AFDPlayerController::Input_Attack(const FInputActionValue& Value)
{
	// 마우스 좌클릭 → 서버에 공격 요청 (술래 전용)
	AFDTaggerCharacter* Tagger = Cast<AFDTaggerCharacter>(GetPawn());
	if (!Tagger) return;

	Tagger->Server_TryCapture();
}

void AFDPlayerController::LockMovementForScouting()
{
	// 정찰 단계: 이동만 잠금, 카메라 회전은 허용
	SetIgnoreMoveInput(true);

	UE_LOG(LogTemp, Log, TEXT("[플레이어 컨트롤러] 정찰 단계 이동 잠금"));
}

void AFDPlayerController::UnlockMovement()
{
	// 본 게임 시작: 이동 잠금 해제
	SetIgnoreMoveInput(false);

	UE_LOG(LogTemp, Log, TEXT("[플레이어 컨트롤러] 이동 잠금 해제"));
}

void AFDPlayerController::Client_ShowCapturePopup_Implementation()
{
	// 서버 → 술래 클라이언트: 포획 팝업 UI 활성화
	// TODO: 위젯 블루프린트 연동 후 팝업 위젯 생성 및 뷰포트 추가
	UE_LOG(LogTemp, Log, TEXT("[플레이어 컨트롤러] 포획 팝업 UI 활성화"));
}

void AFDPlayerController::Server_RequestOut_Implementation()
{
	// 술래가 아웃 버튼 선택 → 서버에서 포획 타이머 즉시 만료 처리
	AFDTaggerCharacter* Tagger = Cast<AFDTaggerCharacter>(GetPawn());
	if (!Tagger) return;

	Tagger->ForceOut();
	UE_LOG(LogTemp, Log, TEXT("[플레이어 컨트롤러] 술래가 아웃 선택"));
}

bool AFDPlayerController::Server_RequestOut_Validate()
{
	// 실제로 술래 캐릭터를 가진 컨트롤러인지 검증
	return Cast<AFDTaggerCharacter>(GetPawn()) != nullptr;
}

void AFDPlayerController::Server_RequestSpare_Implementation()
{
	// 술래가 봐주기 선택 → 숨는 자에게 무적 + 속도 버프 부여
	AFDTaggerCharacter* Tagger = Cast<AFDTaggerCharacter>(GetPawn());
	if (!Tagger) return;

	Tagger->RequestSpare();
	UE_LOG(LogTemp, Log, TEXT("[플레이어 컨트롤러] 술래가 봐주기 선택"));
}

bool AFDPlayerController::Server_RequestSpare_Validate()
{
	// 실제로 술래 캐릭터를 가진 컨트롤러인지 검증
	return Cast<AFDTaggerCharacter>(GetPawn()) != nullptr;
}
