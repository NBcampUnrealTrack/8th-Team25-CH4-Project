// PlayerController.cpp

#include "Controller/FDPlayerController.h"
#include "Character/FDTaggerCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Blueprint/UserWidget.h"
#include "UI/FDCapturePopupWidget.h"

void AFDPlayerController::Client_LockMovement_Implementation()
{
	SetIgnoreMoveInput(true);
	UE_LOG(LogTemp, Log, TEXT("[플레이어 컨트롤러] 이동 잠금 (클라이언트)"));

}

void AFDPlayerController::Client_UnlockMovement_Implementation()
{
	SetIgnoreMoveInput(false);
	UE_LOG(LogTemp, Log, TEXT("[플레이어 컨트롤러] 이동 잠금 해제 (클라이언트)"));
}

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

	// 봐주기 키 입력 바인딩 (술래 전용)
	if (IA_Spare)
	{
		EIC->BindAction(IA_Spare, ETriggerEvent::Started, this, &AFDPlayerController::Input_Spare);
	}
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

void AFDPlayerController::Input_Spare(const FInputActionValue& Value)
{
	// 포획 판정 UI가 떠 있는 상태에서만 의미 있는 입력이라
	// 실제 유효성 검증은 Server_RequestSpare_Validate에서 처리됨
	Server_RequestSpare();
}

void AFDPlayerController::Client_ShowCapturePopup_Implementation()
{
	// 서버 → 술래 클라이언트: 포획 팝업 UI 활성화
	if (!CaptureWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[플레이어 컨트롤러] CaptureWidgetClass가 할당되지 않음"));
		return;
	}

	// 이미 떠있는 팝업이 있으면 중복 생성 방지
	if (CaptureWidgetInstance && CaptureWidgetInstance->IsInViewport())
	{
		return;
	}

	CaptureWidgetInstance = CreateWidget<UFDCapturePopupWidget>(this, CaptureWidgetClass);
	if (!CaptureWidgetInstance) return;

	CaptureWidgetInstance->AddToViewport();

	// 팝업이 떠있는 동안 마우스 커서로 버튼 클릭 가능하게 처리
	SetShowMouseCursor(true);
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(CaptureWidgetInstance->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);

	UE_LOG(LogTemp, Log, TEXT("[플레이어 컨트롤러] 포획 팝업 UI 활성화"));
}

void AFDPlayerController::Client_HideCapturePopup_Implementation()
{
	if (CaptureWidgetInstance && CaptureWidgetInstance->IsInViewport())
	{
		CaptureWidgetInstance->RemoveFromParent();
	}

	SetShowMouseCursor(false);
	SetInputMode(FInputModeGameOnly());

	UE_LOG(LogTemp, Log, TEXT("[플레이어 컨트롤러] 포획 팝업 UI 제거"));
}

void AFDPlayerController::Server_RequestOut_Implementation()
{
	// 술래가 아웃 버튼 선택 → 서버에서 포획 타이머 즉시 만료 처리
	AFDTaggerCharacter* Tagger = Cast<AFDTaggerCharacter>(GetPawn());
	if (!Tagger) return;

	Tagger->ForceOut();
	Client_HideCapturePopup();
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
	Client_HideCapturePopup();
	UE_LOG(LogTemp, Log, TEXT("[플레이어 컨트롤러] 술래가 봐주기 선택"));
}

bool AFDPlayerController::Server_RequestSpare_Validate()
{
	// 실제로 술래 캐릭터를 가진 컨트롤러인지 검증
	return Cast<AFDTaggerCharacter>(GetPawn()) != nullptr;
}
