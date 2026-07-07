// FDStartWidget.cpp

#include "UI/FDStartWidget.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

void UFDStartWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	if (StartButton)
	{
		StartButton->OnClicked.AddDynamic(this, &UFDStartWidget::OnStartClicked);
	}
	
	if (HowToPlayButton)
	{
		HowToPlayButton->OnClicked.AddDynamic(this, &UFDStartWidget::OnHowToPlayClicked);
	}
	
	if (CloseHowToPlayButton)
	{
		CloseHowToPlayButton->OnClicked.AddDynamic(this, &UFDStartWidget::OnCloseHowToPlayClicked);
	}
	
	if (QuitButton)
	{
		QuitButton->OnClicked.AddDynamic(this, &UFDStartWidget::OnQuitClicked);
	}

	// 설명 패널은 처음엔 숨겨둠
	if (HowToPlayPanel)
	{
		HowToPlayPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UFDStartWidget::OnStartClicked()
{
	// 구현 전까지 확인용 로그
	UE_LOG(LogTemp, Warning, TEXT("게임 입장 클릭"));
}

void UFDStartWidget::OnHowToPlayClicked()
{
	if (!HowToPlayPanel) return;

	// 현재 상태를 파악해서 bCurrentlyVisible 변수에 넣고
	const bool bCurrentlyVisible = HowToPlayPanel->GetVisibility() == ESlateVisibility::Visible;
	
	// 닫혀있으면 패널 열고 열려있으면 패널 닫음
	HowToPlayPanel->SetVisibility(bCurrentlyVisible
		? ESlateVisibility::Collapsed
		: ESlateVisibility::Visible);
}

// 이건 열려 있을 때 닫힘 버튼 없으면 플레이어가 당황할수도 있으니까 닫힘 버튼도 만든 것
void UFDStartWidget::OnCloseHowToPlayClicked()
{
	if (HowToPlayPanel)
	{
		HowToPlayPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UFDStartWidget::OnQuitClicked()
{
	// 게임 프로세스 종료
	UKismetSystemLibrary::QuitGame(
		this,
		GetOwningPlayer(),
		EQuitPreference::Quit,
		false);
}
