// FDStartWidget.h
// 게임 첫 실행 시 뜨는 메인 메뉴 위젯

#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FDStartWidget.generated.h"

class UButton;
class UWidget;

UCLASS()
class FUNNYORDIE_API UFDStartWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	// 게임 입장 버튼 (지금은 로그만 찍힘 접속 로직은 추후 구현)
	UPROPERTY(meta = (BindWidget))
	UButton* StartButton;

	// 게임 설명 버튼 -> 설명
	UPROPERTY(meta = (BindWidget))
	UButton* HowToPlayButton;

	// 게임 종료 버튼
	UPROPERTY(meta = (BindWidget))
	UButton* QuitButton;
	
	// BindWidgetOptional: WBP에 없어도 에러 안 남
	// 게임 설명 버튼을 눌렀을 때 열리는 설명 화면
	// 어떤걸 패널로 쓸지 몰라서 uwidget으로 타입 설정함
	UPROPERTY(meta = (BindWidgetOptional))
	UWidget* HowToPlayPanel;

	// 설명 패널 닫기 버튼 (패널 안에 배치)
	UPROPERTY(meta = (BindWidgetOptional))
	UButton* CloseHowToPlayButton;
	
	UFUNCTION()
	void OnStartClicked();

	UFUNCTION()
	void OnQuitClicked();

	UFUNCTION()
	void OnHowToPlayClicked();
	
	UFUNCTION()
	void OnCloseHowToPlayClicked();
};