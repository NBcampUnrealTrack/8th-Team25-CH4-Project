// FDEmoteMenuWidget.cpp

#include "Emote/FDEmoteMenuWidget.h"
#include "Emote/FDEmoteComponent.h"
#include "Emote/FDEmoteButtonWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Engine/DataTable.h"
#include "GameDataTypes.h"

void UFDEmoteMenuWidget::InitializeMenu(UFDEmoteComponent* InEmoteComponent)
{
	EmoteComponent = InEmoteComponent;
	BuildEmoteButtons();
}

void UFDEmoteMenuWidget::BuildEmoteButtons()
{
	if (!ButtonContainer || !EmoteComponent || !EmoteButtonClass) return;

	ButtonContainer->ClearChildren();

	// DataTable이 아예 없거나 비어있어도 휠 모양 자체는 유지되게, 등록된 이모트 목록만 먼저 구해둠
	TArray<FName> RowNames;
	UDataTable* Table = EmoteComponent->GetEmoteDataTable();
	if (Table)
	{
		// GetRowNames는 테이블에 등록된 순서를 그대로 유지해서 반환 - 휠에서 시계방향 순서를 예측 가능하게 함
		RowNames = Table->GetRowNames();
	}

	// 슬롯 개수는 WheelSlotCount로 고정 - 등록된 이모트가 이보다 적으면 나머지는 빈 슬롯으로 채움
	const int32 SlotCount = WheelSlotCount;
	if (SlotCount <= 0) return;

	for (int32 Index = 0; Index < SlotCount; ++Index)
	{
		UFDEmoteButtonWidget* Button = CreateWidget<UFDEmoteButtonWidget>(this, EmoteButtonClass);
		if (!Button) continue;

		if (Index < RowNames.Num())
		{
			// 이 슬롯에 해당하는 이모트 데이터가 있으면 채워 넣음
			const FName RowName = RowNames[Index];
			const FFDEmoteData* Data = Table->FindRow<FFDEmoteData>(RowName, TEXT("이모트 휠 버튼 생성"));
			if (Data)
			{
				Button->Setup(RowName, Data->DisplayName, Data->Icon);
			}
			else
			{
				Button->SetupEmpty();
			}
		}
		else
		{
			// 등록된 이모트보다 슬롯이 더 많으면 남는 자리는 빈 슬롯
			Button->SetupEmpty();
		}

		Button->OnEmoteClicked.AddDynamic(this, &UFDEmoteMenuWidget::HandleEmoteClicked);

		UPanelSlot* AddedSlot = ButtonContainer->AddChild(Button);
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(AddedSlot))
		{
			// 12시 방향(위쪽)부터 시작해서 시계방향으로 등분 배치 - 빈 슬롯도 자리를 차지해서 휠 모양이 항상 유지됨
			const float AngleStep = 360.f / SlotCount;
			const float AngleDeg = -90.f + AngleStep * Index; // -90도 = 위쪽에서 시작
			const float AngleRad = FMath::DegreesToRadians(AngleDeg);

			const FVector2D Offset(
				WheelRadius * FMath::Cos(AngleRad),
				WheelRadius * FMath::Sin(AngleRad)
			);

			// 캔버스 정중앙을 기준점(0,0)으로 잡고, 버튼 자기 자신도 중심 정렬해서
			// Offset이 곧 "중심에서 이 방향으로 얼마나 떨어진 위치"가 되게 함
			CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			CanvasSlot->SetPosition(Offset);
			CanvasSlot->SetSize(ButtonSize);
		}
	}
}

void UFDEmoteMenuWidget::HandleEmoteClicked(FName EmoteRowName)
{
	if (EmoteComponent)
	{
		EmoteComponent->Server_RequestPlayEmote(EmoteRowName);
	}
}

