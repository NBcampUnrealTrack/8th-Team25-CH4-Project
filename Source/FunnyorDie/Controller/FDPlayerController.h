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
	
	// 관전 시점을 지정한 액터로 이동
	UFUNCTION(Client, Reliable)
	void Client_SetSpectateTarget(AActor* NewViewTarget);

	// 색상 커스터마이징 메뉴 열기/닫기 (C키 토글)
	void ToggleColorMenu(bool bOpen);

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	// GameMode/GameState 수정 없이, GameState의 EMatchPhase 변화를 직접 감지해서
	// 정찰(60초)/본게임(300초)/엔딩 브금을 그때그때 전환하기 위함 (Tagger/Hider의 정찰 속도 감지와 동일 패턴)
	virtual void Tick(float DeltaSeconds) override;

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

	// 이모트 메뉴 홀드 인풋 액션 (에디터에서 할당) - 누르고 있는 동안만 열림, 이동 허용이라 기본 IMC에 그대로 포함
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* IA_EmoteMenu;
	
	// 투명화 아이템 사용 키 (Z) - 하이더 전용
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* IA_UseInvisibility;
	
	// 투사체 사용 키 (X) - 하이더 전용
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* IA_UseThrowItem;

	// 시점 전환 키 (1인칭 ↔ 3인칭, 기본값 3인칭) - 술래/하이더 공용
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* IA_ToggleView;

	// 색상 커스터마이징 메뉴 토글 인풋 액션 (에디터에서 IA_FDColorMenu 할당)
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* IA_ColorMenu;
	
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

	// 색상 커스터마이징 메뉴 위젯 블루프린트 클래스 (에디터에서 WBP_FDColorMenu 할당)
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UFDColorMenuWidget> ColorMenuWidgetClass;

	// 현재 뷰포트에 떠있는 색상 메뉴 위젯 인스턴스
	UPROPERTY()
	class UFDColorMenuWidget* ColorMenuWidgetInstance;


	// 투사체를 던진 뒤 3인칭 카메라로 복귀시키기 전 대기하는 타이머
	// (던지자마자 바로 3인칭으로 전환하면 시점이 확 바뀌어서 멀미를 유발해 텀을 둠)
	FTimerHandle ThrowCameraReturnTimerHandle;
	
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

	// 투명화
	void Input_UseInvisibility(const FInputActionValue& Value);
	
	// 투사체
	void Input_UseThrowItem(const FInputActionValue& Value);

	// 시점 전환 입력 처리 - 술래/하이더 공용 (조종 중인 폰 타입에 맞춰 알아서 분기됨)
	void Input_ToggleView(const FInputActionValue& Value);

	// 색상 메뉴 토글 입력 처리
	void Input_ColorMenu(const FInputActionValue& Value);

	// 이모트 메뉴 홀드 입력 처리 - 누르는 동안만 열림, 떼면 바로 닫힘
	void Input_EmoteMenuHoldStart(const FInputActionValue& Value);
	void Input_EmoteMenuHoldEnd(const FInputActionValue& Value);

	// 실제로 메뉴를 열고 닫는 함수 - 이동/카메라는 계속 허용되게 FInputModeGameAndUI 사용
	void ToggleEmoteMenu(bool bOpen);

	// 직전 프레임까지 확인한 매치 페이즈 - Tick에서 변화 감지용 (중복 호출 방지)
	// Warmup으로 시작 = 접속 직후엔 브금 없음
	EMatchPhase LastPhase = EMatchPhase::Warmup;

	// 페이즈가 바뀌었을 때 그에 맞는 브금으로 전환 (Tick에서 변화 감지 시 호출)
	void UpdatePhaseMusic(EMatchPhase NewPhase);
};
