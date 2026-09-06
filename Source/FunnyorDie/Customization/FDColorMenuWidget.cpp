// FDColorMenuWidget.cpp

#include "Customization/FDColorMenuWidget.h"
#include "Customization/FDCustomizationComponent.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Blueprint/WidgetTree.h"

void UFDColorSwatchHandler::Setup(UFDCustomizationComponent* InComponent, int32 InPresetIndex)
{
	CustomizationComponent = InComponent;
	PresetIndex = InPresetIndex;
}

void UFDColorSwatchHandler::HandleClicked()
{
	if (!CustomizationComponent) return;

	// 로컬에서 미리 칠하지 않는다. 서버가 PlayerState를 갱신하고 그 복제로 되돌아오는 경로 하나만 쓴다
	CustomizationComponent->Server_RequestPresetIndex(PresetIndex);
}

void UFDColorMenuWidget::InitializeMenu(UFDCustomizationComponent* InComponent)
{
	CustomizationComponent = InComponent;
	BuildSwatches();
}

void UFDColorMenuWidget::BuildSwatches()
{
	if (!SwatchContainer)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[커스터마이징] SwatchContainer가 없음 - WBP에 Canvas Panel을 SwatchContainer 이름으로 배치해야 함"));
		return;
	}
	if (!CustomizationComponent) return;

	SwatchContainer->ClearChildren();
	Handlers.Reset();

	const TArray<FFDColorPreset>& Palette = CustomizationComponent->GetPalette();
	const int32 Columns = FMath::Max(1, ColumnCount);

	// 격자 전체를 캔버스 중앙에 놓기 위한 좌상단 기준점
	/*
	한 칸의 실제 높이는 버튼 높이에 이름표 높이를 더한 값이다.
	이름표를 버튼 안에 넣으면 세 글자만 넘어가도 버튼 폭을 뚫고 나가기 때문에
	버튼 아래에 별도 줄로 빼서 폭 제약을 받지 않게 했다.
	*/
	const float CellHeight = SwatchSize.Y + LabelHeight;

	const int32 RowCount = FMath::DivideAndRoundUp(Palette.Num(), Columns);
	const float TotalWidth = Columns * SwatchSize.X + (Columns - 1) * SwatchPadding;
	const float TotalHeight = RowCount * CellHeight + (RowCount - 1) * SwatchPadding;
	const FVector2D Origin(-TotalWidth * 0.5f, -TotalHeight * 0.5f);

	for (int32 Index = 0; Index < Palette.Num(); ++Index)
	{
		const FFDColorPreset& Preset = Palette[Index];

		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		if (!Button) continue;

		/*
		버튼 배경을 프리셋 대표 색으로 칠해서 색 자체가 라벨 역할을 하게 한다.
		FButtonStyle을 직접 조립하면 SlateCore 모듈 심볼을 참조하게 되어
		Build.cs에 SlateCore를 추가해야 링크가 된다.
		UMG가 제공하는 배경 틴트 API만 쓰면 의존성이 UMG 안에서 끝나므로
		이쪽을 택했다. 호버와 눌림 상태 색은 엔진 기본 동작을 그대로 쓴다.
		*/
		Button->SetBackgroundColor(Preset.GetSwatchColor());

		// 버튼마다 자기 인덱스를 아는 핸들러를 하나씩 붙임
		UFDColorSwatchHandler* Handler = NewObject<UFDColorSwatchHandler>(this);
		Handler->Setup(CustomizationComponent, Index);
		Button->OnClicked.AddDynamic(Handler, &UFDColorSwatchHandler::HandleClicked);
		Handlers.Add(Handler);

		/*
		버튼과 이름표를 세로 박스 하나로 묶어 캔버스에 올린다.
		이름표는 버튼 밖이라 글자가 길어져도 버튼을 뚫지 않고 아래로 흐른다.
		*/
		UVerticalBox* Cell = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		if (!Cell) continue;

		if (UVerticalBoxSlot* ButtonSlot = Cast<UVerticalBoxSlot>(Cell->AddChild(Button)))
		{
			// 버튼이 남는 세로 공간을 전부 차지하게 해서 스와치를 정사각형에 가깝게 유지
			ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		if (UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()))
		{
			Label->SetText(Preset.DisplayName);
			Label->SetJustification(ETextJustify::Center);

			if (UVerticalBoxSlot* LabelSlot = Cast<UVerticalBoxSlot>(Cell->AddChild(Label)))
			{
				// 이름표는 글자 높이만큼만 차지
				LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
				LabelSlot->SetHorizontalAlignment(HAlign_Center);
			}
		}

		UPanelSlot* AddedSlot = SwatchContainer->AddChild(Cell);
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(AddedSlot))
		{
			const int32 Col = Index % Columns;
			const int32 Row = Index / Columns;

			const FVector2D Position(
				Origin.X + Col * (SwatchSize.X + SwatchPadding),
				Origin.Y + Row * (CellHeight + SwatchPadding)
			);

			// 캔버스 정중앙을 기준점으로 잡고 격자를 그 주변에 펼침
			CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			CanvasSlot->SetAlignment(FVector2D(0.f, 0.f));
			CanvasSlot->SetPosition(Position);
			CanvasSlot->SetSize(FVector2D(SwatchSize.X, CellHeight));
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[커스터마이징] 색상 버튼 %d개 생성"), Palette.Num());
}
