// PlayerController.cpp

#include "Controller/FDPlayerController.h"
#include "Character/FDTaggerCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Blueprint/UserWidget.h"
#include "UI/FDCapturePopupWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Customization/FDCustomizationComponent.h"
#include "Emote/FDEmoteComponent.h"
#include "Emote/FDEmoteMenuWidget.h"
#include "Character/FDHiderCharacter.h"
#include "Item/FDItemInventoryComponent.h"

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

	// 페인팅 입력 바인딩 - Started(스트로크 시작) / Triggered(드래그 중) / Completed(뗌)
	if (IA_Paint)
	{
		EIC->BindAction(IA_Paint, ETriggerEvent::Started, this, &AFDPlayerController::Input_PaintStart);
		EIC->BindAction(IA_Paint, ETriggerEvent::Triggered, this, &AFDPlayerController::Input_PaintOngoing);
		EIC->BindAction(IA_Paint, ETriggerEvent::Completed, this, &AFDPlayerController::Input_PaintEnd);
	}

	// 이모트 메뉴 - 키를 누르고 있는 동안만 표시 (떼면 자동으로 닫힘)
	if (IA_EmoteMenu)
	{
		EIC->BindAction(IA_EmoteMenu, ETriggerEvent::Started, this, &AFDPlayerController::Input_EmoteMenuHoldStart);
		EIC->BindAction(IA_EmoteMenu, ETriggerEvent::Completed, this, &AFDPlayerController::Input_EmoteMenuHoldEnd);
	}
	
	// 투명화 아이템 사용 입력 바인딩 (하이더 전용)
	if (IA_UseInvisibility)
	{
		EIC->BindAction(IA_UseInvisibility, ETriggerEvent::Started, this, &AFDPlayerController::Input_UseInvisibility);
	}
	
	// 투사체 사용 입력 바인딩 (하이더 전용)
	if (IA_UseThrowItem)
	{
		EIC->BindAction(IA_UseThrowItem, ETriggerEvent::Started, this, &AFDPlayerController::Input_UseThrowItem);
	}

	// 시점 전환 입력 바인딩 (1인칭 ↔ 3인칭, 술래/하이더 공용)
	if (IA_ToggleView)
	{
		EIC->BindAction(IA_ToggleView, ETriggerEvent::Started, this, &AFDPlayerController::Input_ToggleView);
	}
}

void AFDPlayerController::Input_Move(const FInputActionValue& Value)
{
	// WASD 이동 입력을 캐릭터에 전달
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn) return;

	const FVector2D MoveInput = Value.Get<FVector2D>();

	// Yaw만 써서 카메라 위/아래를 봐도 걷는 속도가 안 변하게 함 (정찰/본게임 공통, 둘 다 걷기 모드)
	const FRotator CurrentControlRotation = GetControlRotation();
	const FRotator MoveRotation = FRotator(0.f, CurrentControlRotation.Yaw, 0.f);

	const FVector ForwardDir = FRotationMatrix(MoveRotation).GetUnitAxis(EAxis::X);
	const FVector RightDir = FRotationMatrix(MoveRotation).GetUnitAxis(EAxis::Y);

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
	if (AFDTaggerCharacter* Tagger = Cast<AFDTaggerCharacter>(GetPawn()))
	{
		Tagger->Server_TryCapture();
		return;
	}
	
	// 하이더: 조준 중이면 좌클릭 → 투사체 발사
	if (AFDHiderCharacter* Hider = Cast<AFDHiderCharacter>(GetPawn()))
	{
		UFDItemInventoryComponent* Inventory =
			Hider->FindComponentByClass<UFDItemInventoryComponent>();
		if (!Inventory) return;

		// 조준 중이 아니면 좌클릭은 아무 의미 없음
		if (!Inventory->IsAiming()) return;

		Inventory->FireThrowItem();

		// 던지자마자 바로 3인칭으로 전환하면 시점이 확 바뀌면서 멀미를 유발한다는 피드백이 있어서
		// 2초 정도 텀을 두고 자연스럽게 3인칭으로 복귀시킴
		TWeakObjectPtr<AFDHiderCharacter> WeakHider(Hider);
		GetWorldTimerManager().SetTimer(
			ThrowCameraReturnTimerHandle,
			[WeakHider]()
			{
				if (AFDHiderCharacter* HiderPtr = WeakHider.Get())
				{
					HiderPtr->SetAimCameraMode(false);
				}
			},
			2.f,
			false);
	}
}

void AFDPlayerController::Input_Spare(const FInputActionValue& Value)
{
	// 포획 판정 UI가 떠 있는 상태에서만 의미 있는 입력이라
	// 실제 유효성 검증은 Server_RequestSpare_Validate에서 처리됨
	Server_RequestSpare();
}

void AFDPlayerController::Input_UseInvisibility(const FInputActionValue& Value)
{
	// 내가 조종하는 게 하이더인지 확인
	AFDHiderCharacter* Hider = Cast<AFDHiderCharacter>(GetPawn());
	if (!Hider) return;

	// 하이더에 붙어있는 인벤토리 컴포넌트 찾기
	UFDItemInventoryComponent* Inventory =
		Hider->FindComponentByClass<UFDItemInventoryComponent>();
	if (!Inventory) return;

	// 서버에 투명화 사용 요청
	UE_LOG(LogTemp, Warning, TEXT("[컨트롤러] Server_UseItem 호출"));
	Inventory->Server_UseItem(EFDItemEffect::Invisibility);
}

void AFDPlayerController::Input_UseThrowItem(const FInputActionValue& Value)
{
	AFDHiderCharacter* Hider = Cast<AFDHiderCharacter>(GetPawn());
	if (!Hider) return;

	UFDItemInventoryComponent* Inventory =
		Hider->FindComponentByClass<UFDItemInventoryComponent>();
	if (!Inventory) return;

	// 던진 뒤 대기 중이던 3인칭 복귀 타이머가 있다면 취소 (다시 조준을 시작/해제하는 거라 이전 예약은 무효)
	GetWorldTimerManager().ClearTimer(ThrowCameraReturnTimerHandle);

	Inventory->ToggleAiming();

	// 3인칭 카메라 위치 때문에 화면 중앙(크로스헤어)이랑 실제 발사 방향이 어긋나 보이는 문제 →
	// 조준 중엔 1인칭으로 줌인해서 눈높이 = 크로스헤어 = 발사 방향이 일치하게 함
	Hider->SetAimCameraMode(Inventory->IsAiming());
}

void AFDPlayerController::Input_ToggleView(const FInputActionValue& Value)
{
	// 술래: 1인칭 ↔ 3인칭 전환
	if (AFDTaggerCharacter* Tagger = Cast<AFDTaggerCharacter>(GetPawn()))
	{
		Tagger->ToggleViewMode();
		return;
	}

	// 하이더: 1인칭 ↔ 3인칭 전환 (조준 중엔 내부에서 무시됨)
	if (AFDHiderCharacter* Hider = Cast<AFDHiderCharacter>(GetPawn()))
	{
		Hider->ToggleViewMode();
	}
}

void AFDPlayerController::Input_PaintStart(const FInputActionValue& Value)
{
	TryPaintAtCursor(true, false);
}

void AFDPlayerController::Input_PaintOngoing(const FInputActionValue& Value)
{
	TryPaintAtCursor(false, false);
}

void AFDPlayerController::Input_PaintEnd(const FInputActionValue& Value)
{
	TryPaintAtCursor(false, true);
}

void AFDPlayerController::TryPaintAtCursor(bool bStrokeStart, bool bStrokeEnd)
{
	ACharacter* MyCharacter = Cast<ACharacter>(GetPawn());
	if (!MyCharacter) return;

	// 커스터마이징 화면(캐릭터를 정면으로 비추고 마우스로 칠하는 상황)을 가정한 트레이스
	FHitResult Hit;
	GetHitResultUnderCursor(ECC_Visibility, true, Hit);

	if (!Hit.bBlockingHit || Hit.GetActor() != MyCharacter)
	{
		return; // 자기 캐릭터가 아닌 곳을 클릭했으면 무시 (남의 캐릭터에 낙서 못 하게)
	}

	FVector2D UV;
	// ※ 확인 필요: 프로젝트 세팅(Project Settings -> Physics)에서
	// "Support UV From Hit Results" 옵션을 켜야 하고, 메시 콜리전이 Complex Collision을 사용해야
	// FindCollisionUV가 정상적으로 UV를 반환함.
	if (!UGameplayStatics::FindCollisionUV(Hit, 0, UV))
	{
		return;
	}

	if (UFDCustomizationComponent* CustomComp = MyCharacter->FindComponentByClass<UFDCustomizationComponent>())
	{
		CustomComp->RequestLocalPaint(UV, bStrokeStart, bStrokeEnd);
	}
}

void AFDPlayerController::SetCustomizationInputMode(bool bEnable)
{
	UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (!Subsystem) return;

	if (bEnable)
	{
		if (DefaultMappingContext)
		{
			Subsystem->RemoveMappingContext(DefaultMappingContext);
		}
		if (CustomizationMappingContext)
		{
			Subsystem->AddMappingContext(CustomizationMappingContext, 0);
		}
	}
	else
	{
		if (CustomizationMappingContext)
		{
			Subsystem->RemoveMappingContext(CustomizationMappingContext);
		}
		if (DefaultMappingContext)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void AFDPlayerController::Input_EmoteMenuHoldStart(const FInputActionValue& Value)
{
	ToggleEmoteMenu(true);
}

void AFDPlayerController::Input_EmoteMenuHoldEnd(const FInputActionValue& Value)
{
	ToggleEmoteMenu(false);
}

void AFDPlayerController::ToggleEmoteMenu(bool bOpen)
{
	if (bOpen)
	{
		if (!EmoteMenuWidgetInstance && EmoteMenuWidgetClass)
		{
			EmoteMenuWidgetInstance = CreateWidget<UFDEmoteMenuWidget>(this, EmoteMenuWidgetClass);
		}

		if (EmoteMenuWidgetInstance)
		{
			// 캐릭터에 붙어있는 이모트 컴포넌트를 넘겨줘서, 버튼 클릭 시 바로 서버 요청을 보낼 수 있게 함
			if (ACharacter* MyCharacter = Cast<ACharacter>(GetPawn()))
			{
				if (UFDEmoteComponent* EmoteComp = MyCharacter->FindComponentByClass<UFDEmoteComponent>())
				{
					EmoteMenuWidgetInstance->InitializeMenu(EmoteComp);
				}
			}

			if (!EmoteMenuWidgetInstance->IsInViewport())
			{
				EmoteMenuWidgetInstance->AddToViewport();
			}
		}

		// ※ 중요: SetInputMode/SetShowMouseCursor를 지금 이 프레임에서 바로 호출하면
		// Enhanced Input이 "지금 누르고 있던 IA_EmoteMenu 키가 떼졌다"고 오인해서
		// 곧바로 Completed 이벤트를 쏴버리는 문제가 있음 (뷰포트 포커스 변화로 입력 상태가 리셋됨)
		// 그래서 입력 모드 전환만 한 프레임 뒤로 미뤄서, 지금 처리 중인 Started 이벤트가
		// 끝난 뒤에 안전하게 실행되게 함
		GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
		{
			SetShowMouseCursor(true);
			FInputModeGameAndUI InputMode;
			InputMode.SetHideCursorDuringCapture(false);
			SetInputMode(InputMode);
		});
	}
	else
	{
		if (EmoteMenuWidgetInstance && EmoteMenuWidgetInstance->IsInViewport())
		{
			EmoteMenuWidgetInstance->RemoveFromParent();
		}

		SetShowMouseCursor(false);
		SetInputMode(FInputModeGameOnly());
	}

	UE_LOG(LogTemp, Log, TEXT("[플레이어 컨트롤러] 이모트 메뉴 %s"), bOpen ? TEXT("열림") : TEXT("닫힘"));
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
