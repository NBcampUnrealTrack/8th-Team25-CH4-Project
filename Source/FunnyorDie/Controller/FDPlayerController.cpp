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
#include "GameInstance/FDGameInstance.h"
#include "GameState/FDGameState.h"

AFDPlayerController::AFDPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AFDPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (IsLocalController())
	{
		SetShowMouseCursor(false);
		SetInputMode(FInputModeGameOnly());

		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			if (DefaultMappingContext && !Subsystem->HasMappingContext(DefaultMappingContext))
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}
}

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

void AFDPlayerController::Client_SetSpectateTarget_Implementation(AActor* NewViewTarget)
{
	if (!NewViewTarget) return;

	bAutoManageActiveCameraTarget = false;
	SetViewTargetWithBlend(NewViewTarget, 0.5f);
}

void AFDPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController()) return;

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (DefaultMappingContext)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	SetShowMouseCursor(false);
	SetInputMode(FInputModeGameOnly());

	if (UFDGameInstance* GI = GetGameInstance<UFDGameInstance>())
	{
		GI->StopMusic();
	}
}

void AFDPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!IsLocalController()) return;

	const AFDGameState* FDGameState = GetWorld()->GetGameState<AFDGameState>();
	if (!FDGameState) return;

	if (FDGameState->CurrentPhase == LastPhase) return;

	LastPhase = FDGameState->CurrentPhase;
	UpdatePhaseMusic(LastPhase);
}

void AFDPlayerController::UpdatePhaseMusic(EMatchPhase NewPhase)
{
	UFDGameInstance* GI = GetGameInstance<UFDGameInstance>();
	if (!GI) return;

	switch (NewPhase)
	{
	case EMatchPhase::Warmup:
	case EMatchPhase::AssignRole:
		GI->StopMusic();
		break;

	case EMatchPhase::Scouting:
		GI->PlayMusic(GI->ScoutMusic);
		break;

	case EMatchPhase::InGame:
		GI->PlayMusic(GI->InGameMusic);
		break;

	case EMatchPhase::GameOver:
		GI->PlayMusic(GI->EndingMusic);
		break;
	}
}

void AFDPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EIC) return;

	if (IA_Move)
	{
		EIC->BindAction(IA_Move, ETriggerEvent::Triggered, this, &AFDPlayerController::Input_Move);
	}
	
	if (IA_Jump)
	{
		EIC->BindAction(IA_Jump, ETriggerEvent::Started, this, &AFDPlayerController::Input_Jump);
	}

	if (IA_Look)
	{
		EIC->BindAction(IA_Look, ETriggerEvent::Triggered, this, &AFDPlayerController::Input_Look);
	}

	if (IA_Attack)
	{
		EIC->BindAction(IA_Attack, ETriggerEvent::Started, this, &AFDPlayerController::Input_Attack);
	}

	if (IA_Spare)
	{
		EIC->BindAction(IA_Spare, ETriggerEvent::Started, this, &AFDPlayerController::Input_Spare);
	}

	if (IA_Paint)
	{
		EIC->BindAction(IA_Paint, ETriggerEvent::Started, this, &AFDPlayerController::Input_PaintStart);
		EIC->BindAction(IA_Paint, ETriggerEvent::Triggered, this, &AFDPlayerController::Input_PaintOngoing);
		EIC->BindAction(IA_Paint, ETriggerEvent::Completed, this, &AFDPlayerController::Input_PaintEnd);
	}

	if (IA_EmoteMenu)
	{
		EIC->BindAction(IA_EmoteMenu, ETriggerEvent::Started, this, &AFDPlayerController::Input_EmoteMenuHoldStart);
		EIC->BindAction(IA_EmoteMenu, ETriggerEvent::Completed, this, &AFDPlayerController::Input_EmoteMenuHoldEnd);
	}
	
	if (IA_UseInvisibility)
	{
		EIC->BindAction(IA_UseInvisibility, ETriggerEvent::Started, this, &AFDPlayerController::Input_UseInvisibility);
	}
	
	if (IA_UseThrowItem)
	{
		EIC->BindAction(IA_UseThrowItem, ETriggerEvent::Started, this, &AFDPlayerController::Input_UseThrowItem);
	}

	if (IA_ToggleView)
	{
		EIC->BindAction(IA_ToggleView, ETriggerEvent::Started, this, &AFDPlayerController::Input_ToggleView);
	}
}

void AFDPlayerController::Input_Move(const FInputActionValue& Value)
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn) return;

	const FVector2D MoveInput = Value.Get<FVector2D>();

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

	if (ACharacter* ControlledCharacter = Cast<ACharacter>(ControlledPawn))
	{
		ControlledCharacter->Jump();
	}
}

void AFDPlayerController::Input_Look(const FInputActionValue& Value)
{
	const FVector2D LookInput = Value.Get<FVector2D>();
	AddYawInput(LookInput.X);
	AddPitchInput(LookInput.Y);
}

void AFDPlayerController::Input_Attack(const FInputActionValue& Value)
{
	if (AFDTaggerCharacter* Tagger = Cast<AFDTaggerCharacter>(GetPawn()))
	{
		Tagger->Server_TryCapture();
		return;
	}
	
	if (AFDHiderCharacter* Hider = Cast<AFDHiderCharacter>(GetPawn()))
	{
		UFDItemInventoryComponent* Inventory =
			Hider->FindComponentByClass<UFDItemInventoryComponent>();
		if (!Inventory) return;

		if (!Inventory->IsAiming()) return;

		Inventory->FireThrowItem();

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
			1.f,
			false);
	}
}

void AFDPlayerController::Input_Spare(const FInputActionValue& Value)
{
	Server_RequestSpare();
}

void AFDPlayerController::Input_UseInvisibility(const FInputActionValue& Value)
{
	AFDHiderCharacter* Hider = Cast<AFDHiderCharacter>(GetPawn());
	if (!Hider) return;

	UFDItemInventoryComponent* Inventory =
		Hider->FindComponentByClass<UFDItemInventoryComponent>();
	if (!Inventory) return;

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

	GetWorldTimerManager().ClearTimer(ThrowCameraReturnTimerHandle);

	Inventory->ToggleAiming();
	Hider->SetAimCameraMode(Inventory->IsAiming());
}

void AFDPlayerController::Input_ToggleView(const FInputActionValue& Value)
{
	if (AFDTaggerCharacter* Tagger = Cast<AFDTaggerCharacter>(GetPawn()))
	{
		Tagger->ToggleViewMode();
		return;
	}

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

	FHitResult Hit;
	GetHitResultUnderCursor(ECC_Visibility, true, Hit);

	if (!Hit.bBlockingHit || Hit.GetActor() != MyCharacter)
	{
		return;
	}

	FVector2D UV;
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
	if (!CaptureWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[플레이어 컨트롤러] CaptureWidgetClass가 할당되지 않음"));
		return;
	}

	if (CaptureWidgetInstance && CaptureWidgetInstance->IsInViewport())
	{
		return;
	}

	CaptureWidgetInstance = CreateWidget<UFDCapturePopupWidget>(this, CaptureWidgetClass);
	if (!CaptureWidgetInstance) return;

	CaptureWidgetInstance->AddToViewport();

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
	AFDTaggerCharacter* Tagger = Cast<AFDTaggerCharacter>(GetPawn());
	if (!Tagger) return;

	Tagger->ForceOut();
	Client_HideCapturePopup();
	UE_LOG(LogTemp, Log, TEXT("[플레이어 컨트롤러] 술래가 아웃 선택"));
}

bool AFDPlayerController::Server_RequestOut_Validate()
{
	return Cast<AFDTaggerCharacter>(GetPawn()) != nullptr;
}

void AFDPlayerController::Server_RequestSpare_Implementation()
{
	AFDTaggerCharacter* Tagger = Cast<AFDTaggerCharacter>(GetPawn());
	if (!Tagger) return;

	Tagger->RequestSpare();
	Client_HideCapturePopup();
	UE_LOG(LogTemp, Log, TEXT("[플레이어 컨트롤러] 술래가 봐주기 선택"));
}

bool AFDPlayerController::Server_RequestSpare_Validate()
{
	return Cast<AFDTaggerCharacter>(GetPawn()) != nullptr;
}
