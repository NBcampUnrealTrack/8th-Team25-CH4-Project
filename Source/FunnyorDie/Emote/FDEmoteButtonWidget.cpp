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
	bIsEmptySlot = false;

	if (NameText)
	{
		NameText->SetText(InDisplayName);
	}

	if (IconImage)
	{
		if (InIcon)
		{
			IconImage->SetBrushFromTexture(InIcon);
			IconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}

	if (ClickButton)
	{
		ClickButton->SetIsEnabled(true);
	}
}

void UFDEmoteButtonWidget::SetupEmpty()
{
	RowName = NAME_None;
	bIsEmptySlot = true;

	if (NameText)
	{
		NameText->SetText(FText::GetEmpty());
	}

	if (IconImage)
	{
		// 빈 슬롯은 아이콘을 비워서 휠 자체는 유지하되 내용은 없는 상태로 표시
		// (에디터 WBP에서 기본 "빈 슬롯" 텍스처를 IconImage 기본 브러시로 깔아두면 그게 그대로 보임)
		IconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (ClickButton)
	{
		// 클릭 자체를 막아서 빈 슬롯 눌러도 아무 일 안 일어나게 함
		ClickButton->SetIsEnabled(false);
	}
}

void UFDEmoteButtonWidget::OnButtonClicked()
{
	if (bIsEmptySlot) return; // 빈 슬롯은 클릭돼도 무시 (버튼 자체도 비활성화돼있지만 이중 방어)

	// 자기 RowName을 들고 상위 메뉴 위젯에 알림 - 실제 서버 요청은 메뉴 위젯이 처리
	OnEmoteClicked.Broadcast(RowName);
}
