// FDEmoteButtonWidget.cpp

#include "Emote/FDEmoteButtonWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

void UFDEmoteButtonWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ClickButton)
	{
		ClickButton->OnClicked.AddDynamic(this, &UFDEmoteButtonWidget::OnButtonClicked);
	}
}

void UFDEmoteButtonWidget::Setup(FName InRowName, const FText& InDisplayName, UTexture2D* InIcon)
{
	RowName = InRowName;

	if (NameText)
	{
		NameText->SetText(InDisplayName);
	}

	if (IconImage && InIcon)
	{
		IconImage->SetBrushFromTexture(InIcon);
	}
}

void UFDEmoteButtonWidget::OnButtonClicked()
{
	// 자기 RowName을 들고 상위 메뉴 위젯에 알림 - 실제 서버 요청은 메뉴 위젯이 처리
	OnEmoteClicked.Broadcast(RowName);
}
