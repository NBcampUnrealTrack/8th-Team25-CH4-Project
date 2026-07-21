// FDStartWidget.cpp

#include "UI/FDStartWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Kismet/KismetSystemLibrary.h"

void UFDStartWidget::NativeConstruct()
{
	Super::NativeConstruct();

	FDGameInstance = GetGameInstance<UFDGameInstance>();

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
	
	if (HostButton)
	{
		HostButton->OnClicked.AddDynamic(this, &UFDStartWidget::OnHostClicked);
	}
	
	if (JoinButton)
	{
		JoinButton->OnClicked.AddDynamic(this, &UFDStartWidget::OnJoinClicked);
	}
	
	if (BackButton)
	{
		BackButton->OnClicked.AddDynamic(this, &UFDStartWidget::OnBackClicked);
	}

	// 패널들은 처음엔 닫아둠
	if (HowToPlayPanel)
	{
		HowToPlayPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	SetSessionPanelOpen(false);

	if (FDGameInstance)
	{
		FDGameInstance->OnSessionStatusChanged.AddDynamic(this, &UFDStartWidget::HandleSessionStatusChanged);
		
		ApplyStatusToUI(FDGameInstance->GetSessionStatus());
	}
}

void UFDStartWidget::NativeDestruct()
{
	if (FDGameInstance)
	{
		FDGameInstance->OnSessionStatusChanged.RemoveDynamic(this, &UFDStartWidget::HandleSessionStatusChanged);
	}

	Super::NativeDestruct();
}

// 세션 패널 열기/닫기
void UFDStartWidget::OnStartClicked()
{
	SetSessionPanelOpen(true);
}

void UFDStartWidget::OnBackClicked()
{
	SetSessionPanelOpen(false);
}

void UFDStartWidget::SetSessionPanelOpen(bool bOpen)
{
	if (SessionPanel)
	{
		SessionPanel->SetVisibility(bOpen
			? ESlateVisibility::Visible
			: ESlateVisibility::Collapsed);
	}

	// 세션 패널이 열려 있는 동안엔 뒤쪽 메인 메뉴 버튼들을 잠가서
	// 실수로 Play를 또 누르거나 Leave를 누르는 걸 막음
	if (StartButton)      StartButton->SetIsEnabled(!bOpen);
	if (HowToPlayButton)  HowToPlayButton->SetIsEnabled(!bOpen);
	if (QuitButton)       QuitButton->SetIsEnabled(!bOpen);

	// 패널을 열 때 현재 세션 상태를 반영 (로그인 중이면 버튼이 잠겨 있어야 함)
	if (bOpen && FDGameInstance)
	{
		ApplyStatusToUI(FDGameInstance->GetSessionStatus());
	}
}

// 세션 버튼 -> GameInstance 로 전달만 
void UFDStartWidget::OnHostClicked()
{
	if (!FDGameInstance) return;
	FDGameInstance->HostSession();
}

void UFDStartWidget::OnJoinClicked()
{
	if (!FDGameInstance) return;
	FDGameInstance->FindAndJoinSession();
}

// GameInstance의 방송을 받아 UI 갱신
void UFDStartWidget::HandleSessionStatusChanged(EFDSessionStatus NewStatus)
{
	ApplyStatusToUI(NewStatus);
}

void UFDStartWidget::ApplyStatusToUI(EFDSessionStatus Status)
{
	FText Message = FText::GetEmpty();

	bool bSessionButtonsEnabled = false;

	switch (Status)
	{
	case EFDSessionStatus::Idle:
		Message = FText::FromString(TEXT("준비 중..."));
		break;

	case EFDSessionStatus::LoggingIn:
		Message = FText::FromString(TEXT("EOS 로그인 중..."));
		break;

	case EFDSessionStatus::LoginFailed:
		Message = FText::FromString(TEXT("로그인 실패. 네트워크를 확인해주세요."));
		break;

	case EFDSessionStatus::Ready:
		Message = FText::GetEmpty();
		bSessionButtonsEnabled = true;
		break;

	case EFDSessionStatus::Hosting:
		Message = FText::FromString(TEXT("방을 만드는 중..."));
		break;

	case EFDSessionStatus::Searching:
		Message = FText::FromString(TEXT("방을 찾는 중..."));
		break;

	case EFDSessionStatus::Joining:
		Message = FText::FromString(TEXT("방에 입장하는 중..."));
		break;

	case EFDSessionStatus::NoSessionFound:
		Message = FText::FromString(TEXT("열린 방이 없습니다. 방을 만들어보세요."));
		bSessionButtonsEnabled = true;   // 재시도 가능하게 풀어줌
		break;

	case EFDSessionStatus::Failed:
		Message = FText::FromString(TEXT("실패했습니다. 다시 시도해주세요."));
		bSessionButtonsEnabled = true;
		break;

	default:
		break;
	}

	if (HostButton) HostButton->SetIsEnabled(bSessionButtonsEnabled);
	if (JoinButton) JoinButton->SetIsEnabled(bSessionButtonsEnabled);

	if (StatusText)
	{
		StatusText->SetText(Message);
	}
}

void UFDStartWidget::OnHowToPlayClicked()
{
	if (!HowToPlayPanel) return;

	const bool bCurrentlyVisible = HowToPlayPanel->GetVisibility() == ESlateVisibility::Visible;
	
	HowToPlayPanel->SetVisibility(bCurrentlyVisible
		? ESlateVisibility::Collapsed
		: ESlateVisibility::Visible);
}

void UFDStartWidget::OnCloseHowToPlayClicked()
{
	if (HowToPlayPanel)
	{
		HowToPlayPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UFDStartWidget::OnQuitClicked()
{
	UKismetSystemLibrary::QuitGame(
		this,
		GetOwningPlayer(),
		EQuitPreference::Quit,
		false);
}