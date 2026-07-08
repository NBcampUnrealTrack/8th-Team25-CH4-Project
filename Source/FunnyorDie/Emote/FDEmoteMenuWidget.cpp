// FDEmoteMenuWidget.cpp

#include "Emote/FDEmoteMenuWidget.h"
#include "Emote/FDEmoteComponent.h"
#include "Emote/FDEmoteButtonWidget.h"
#include "Components/PanelWidget.h"
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

	UDataTable* Table = EmoteComponent->GetEmoteDataTable();
	if (!Table) return;

	// GetRowMap은 런타임(패키징된 빌드)에서도 사용 가능 - 에디터 전용 아님
	for (const TPair<FName, uint8*>& RowPair : Table->GetRowMap())
	{
		const FName RowName = RowPair.Key;
		const FFDEmoteData* Data = reinterpret_cast<FFDEmoteData*>(RowPair.Value);
		if (!Data) continue;

		UFDEmoteButtonWidget* Button = CreateWidget<UFDEmoteButtonWidget>(this, EmoteButtonClass);
		if (!Button) continue;

		Button->Setup(RowName, Data->DisplayName, Data->Icon);
		Button->OnEmoteClicked.AddDynamic(this, &UFDEmoteMenuWidget::HandleEmoteClicked);

		ButtonContainer->AddChild(Button);
	}
}

void UFDEmoteMenuWidget::HandleEmoteClicked(FName EmoteRowName)
{
	if (EmoteComponent)
	{
		EmoteComponent->Server_RequestPlayEmote(EmoteRowName);
	}
}
