// PlayerController.h

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "FDPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UUserWidget;

UCLASS()
class FUNNYORDIE_API AFDPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	// 정찰 단계 시작 시 이동 잠금 (서버에서 호출)
	void LockMovementForScouting();

	// 본 게임 시작 시 이동 잠금 해제 (서버에서 호출)
	void UnlockMovement();

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

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	// 인풋 매핑 컨텍스트 (에디터에서 할당)
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	// 마우스 좌클릭 공격 인풋 액션 (에디터에서 할당)
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* IA_Attack;

	// 이동 인풋 액션 (에디터에서 할당)
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* IA_Move;
	
	// 점프 인풋 액션 (에디터에서 할당)
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* IA_Jump;

	// 카메라 회전 인풋 액션 (에디터에서 할당)
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* IA_Look;
	
	// 봐주기 키 인풋 액션 (에디터에서 할당) — 술래 전용
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* IA_Spare;
	
	// 포획 팝업 위젯 블루프린트 클래스 (에디터에서 WBP 할당)
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> CaptureWidgetClass;
	
	// 현재 뷰포트에 떠있는 포획 팝업 위젯 인스턴스
	UPROPERTY()
	UUserWidget* CaptureWidgetInstance;
	
	// 마우스 좌클릭 → 서버에 공격 요청
	void Input_Attack(const FInputActionValue& Value);

	// 이동 입력 처리
	void Input_Move(const FInputActionValue& Value);
	
	// 점프 입력 처리
	void Input_Jump(const FInputActionValue& Value);

	// 카메라 회전 입력 처리
	void Input_Look(const FInputActionValue& Value);
	
	// 봐주기 키 입력 처리 → 서버에 봐주기 요청
	void Input_Spare(const FInputActionValue& Value);
};