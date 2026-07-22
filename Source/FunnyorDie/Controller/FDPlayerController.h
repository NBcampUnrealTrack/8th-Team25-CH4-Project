// PlayerController.h

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "GameMode/FDGameMode.h" // EMatchPhase enum 사용
#include "FDPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UFDCapturePopupWidget;
class UFDEmoteMenuWidget;

UCLASS()
class FUNNYORDIE_API AFDPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AFDPlayerController();

	// 캐릭터 Possess 시 입력 모드 및 IMC 보장 처리
	virtual void OnPossess(APawn* InPawn) override;

	// 서버 → 술래 클라이언트: 포획 팝업 UI 활성화
	UFUNCTION(Client, Reliable)
	void Client_ShowCapturePopup();
	
	// 서버 → 술래 클라이언트: 포획 팝업 UI 제거
	UFUNCTION(Client, Reliable)
	void Client_HideCapturePopup();

	// 술래 → 서버: 아웃 요청
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestOut();

	// 술래 → 서버: 봐주기 요청
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestSpare();
	
	UFUNCTION(Client, Reliable) // 이동 잠금 RPC
	void Client_LockMovement();

	UFUNCTION(Client, Reliable) // 이동 잠금 해제 RPC
	void Client_UnlockMovement();
	
	// 관전 시점을 지정한 액터로 이동
	UFUNCTION(Client, Reliable)
	void Client_SetSpectateTarget(AActor* NewViewTarget);

	// 페인팅 모드 진입/해제 시 매핑 컨텍스트 스위칭 (Default <-> Customization)
	void SetCustomizationInputMode(bool bEnable);

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	virtual void Tick(float DeltaSeconds) override;

private:
	// 인풋 매핑 컨텍스트 (에디터에서 할당)
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	// 페인팅 전용 매핑 컨텍스트 (에디터에서 할당)
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputMappingContext* CustomizationMappingContext;

	// 마우스 좌클릭 공격 인풋 액션
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* IA_Attack;

	// 이동 인풋 액션
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* IA_Move;
	
	// 점프 인풋 액션
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* IA_Jump;

	// 카메라 회전 인풋 액션
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* IA_Look;
	
	// 봐주기 키 인풋 액션 — 술래 전용
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* IA_Spare;

	// 채색(페인팅) 인풋 액션
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* IA_Paint;

	// 이모트 메뉴 홀드 인풋 액션
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* IA_EmoteMenu;
	
	// 투명화 아이템 사용 키 (Z) - 하이더 전용
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* IA_UseInvisibility;
	
	// 투사체 사용 키 (X) - 하이더 전용
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* IA_UseThrowItem;

	// 시점 전환 키 (1인칭 ↔ 3인칭) - 술래/하이더 공용
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* IA_ToggleView;
	
	// 포획 팝업 위젯 블루프린트 클래스
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UFDCapturePopupWidget> CaptureWidgetClass;
	
	// 현재 뷰포트에 떠있는 포획 팝업 위젯 인스턴스
	UPROPERTY()
	class UFDCapturePopupWidget* CaptureWidgetInstance;

	// 이모트 메뉴 위젯 블루프린트 클래스
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UFDEmoteMenuWidget> EmoteMenuWidgetClass;

	// 현재 뷰포트에 떠있는 이모트 메뉴 위젯 인스턴스
	UPROPERTY()
	UFDEmoteMenuWidget* EmoteMenuWidgetInstance;

	FTimerHandle ThrowCameraReturnTimerHandle;
	
	void Input_Attack(const FInputActionValue& Value);
	void Input_Move(const FInputActionValue& Value);
	void Input_Jump(const FInputActionValue& Value);
	void Input_Look(const FInputActionValue& Value);
	void Input_Spare(const FInputActionValue& Value);
	void Input_UseInvisibility(const FInputActionValue& Value);
	void Input_UseThrowItem(const FInputActionValue& Value);
	void Input_ToggleView(const FInputActionValue& Value);

	void Input_PaintStart(const FInputActionValue& Value);
	void Input_PaintOngoing(const FInputActionValue& Value);
	void Input_PaintEnd(const FInputActionValue& Value);

	void TryPaintAtCursor(bool bStrokeStart, bool bStrokeEnd);

	void Input_EmoteMenuHoldStart(const FInputActionValue& Value);
	void Input_EmoteMenuHoldEnd(const FInputActionValue& Value);

	void ToggleEmoteMenu(bool bOpen);

	EMatchPhase LastPhase = EMatchPhase::Warmup;
	void UpdatePhaseMusic(EMatchPhase NewPhase);
};