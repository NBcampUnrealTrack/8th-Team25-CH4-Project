// PlayerController.h

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
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

	UFUNCTION(Client, Reliable) // 이동 잠금 해제 RPC (아직 쓰이는 곳은 없음)
	void Client_UnlockMovement();

	// 페인팅 모드 진입/해제 시 매핑 컨텍스트 스위칭 (Default <-> Customization)
	void SetCustomizationInputMode(bool bEnable);

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	// 인풋 매핑 컨텍스트 (에디터에서 할당)
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	// 페인팅 전용 매핑 컨텍스트 (에디터에서 할당) - 이동/카메라랑 겹치지 않게 분리
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputMappingContext* CustomizationMappingContext;

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

	// 채색(페인팅) 인풋 액션 - 마우스 좌클릭 드래그로 그림 (CustomizationMappingContext 전용)
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* IA_Paint;

	// 이모트 메뉴 홀드 인풋 액션 (에디터에서 할당) - 누르고 있는 동안만 열림, 이동 허용이라 기본 IMC에 그대로 포함
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* IA_EmoteMenu;
	
	// 투명화 아이템 사용 키 (Z) - 하이더 전용
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* IA_UseInvisibility;
	
	// 포획 팝업 위젯 블루프린트 클래스 (에디터에서 WBP 할당) -> 부모클래스 변경
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UFDCapturePopupWidget> CaptureWidgetClass;
	
	// 현재 뷰포트에 떠있는 포획 팝업 위젯 인스턴스 -> 마찬가지
	UPROPERTY()
	class UFDCapturePopupWidget* CaptureWidgetInstance;

	// 이모트 메뉴 위젯 블루프린트 클래스 (에디터에서 WBP_FDEmoteMenu 할당)
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UFDEmoteMenuWidget> EmoteMenuWidgetClass;

	// 현재 뷰포트에 떠있는 이모트 메뉴 위젯 인스턴스
	UPROPERTY()
	UFDEmoteMenuWidget* EmoteMenuWidgetInstance;
	
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
	
	// 토명화
	void Input_UseInvisibility(const FInputActionValue& Value);

	// 페인팅 입력 처리 - Started/Triggered/Completed 각각 스트로크 시작/중간/끝에 대응
	void Input_PaintStart(const FInputActionValue& Value);
	void Input_PaintOngoing(const FInputActionValue& Value);
	void Input_PaintEnd(const FInputActionValue& Value);

	// 커서 아래를 트레이스해서 자기 캐릭터 메시 UV를 뽑아내고, 커스터마이징 컴포넌트에 페인팅 요청
	void TryPaintAtCursor(bool bStrokeStart, bool bStrokeEnd);

	// 이모트 메뉴 홀드 입력 처리 - 누르는 동안만 열림, 떼면 바로 닫힘
	void Input_EmoteMenuHoldStart(const FInputActionValue& Value);
	void Input_EmoteMenuHoldEnd(const FInputActionValue& Value);

	// 실제로 메뉴를 열고 닫는 함수 - 이동/카메라는 계속 허용되게 FInputModeGameAndUI 사용
	void ToggleEmoteMenu(bool bOpen);
};
