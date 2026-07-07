// PlayerController.h

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "FDPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UFDCapturePopupWidget;

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
	
	// 커스터마이징 모드 진입/해제 시 매핑 컨텍스트 스위칭
	void SetCustomizationInputMode(bool bEnable);

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	// 인풋 매핑 컨텍스트 (에디터에서 할당)
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext;
	
	// 커스터마이징 모드 전용 매핑 컨텍스트 (에디터에서 IMC_Customization 할당)
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

	// 채색(페인팅) 인풋 액션 - 마우스 좌클릭 드래그로 그림
	// 커스터마이징 화면/모드에서만 활성화되는 별도 매핑 컨텍스트에 두는 걸 권장
	// (일반 플레이 중 좌클릭은 IA_Attack이랑 겹치니까 매핑 컨텍스트를 분리해서 상황별로 스위칭해야 함)
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* IA_Paint;
	
	// 포획 팝업 위젯 블루프린트 클래스 (에디터에서 WBP 할당) -> 부모클래스 변경
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UFDCapturePopupWidget> CaptureWidgetClass;
	
	// 현재 뷰포트에 떠있는 포획 팝업 위젯 인스턴스 -> 마찬가지
	UPROPERTY()
	class UFDCapturePopupWidget* CaptureWidgetInstance;
	
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

	// 페인팅 입력 처리 - Started/Triggered/Completed 각각 스트로크 시작/중간/끝에 대응
	void Input_PaintStart(const FInputActionValue& Value);
	void Input_PaintOngoing(const FInputActionValue& Value);
	void Input_PaintEnd(const FInputActionValue& Value);

	// 커서 아래를 트레이스해서 자기 캐릭터 메시 UV를 뽑아내고, 커스터마이징 컴포넌트에 페인팅 요청
	void TryPaintAtCursor(bool bStrokeStart, bool bStrokeEnd);
};
