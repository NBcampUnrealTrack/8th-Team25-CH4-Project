// FDEmoteMenuWidget.h
// 이모트 목록을 보여주고 클릭하면 재생 요청까지 이어주는 컨테이너 위젯
// DataTable을 순회해서 버튼을 동적으로 생성 - 이모트 추가/삭제해도 이 위젯은 안 건드려도 됨

#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FDEmoteMenuWidget.generated.h"

class UPanelWidget;
class UFDEmoteButtonWidget;
class UFDEmoteComponent;

UCLASS()
class FUNNYORDIE_API UFDEmoteMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// PlayerController가 위젯 생성 직후 호출 - 이 컴포넌트를 통해 서버에 재생 요청을 보냄
	UFUNCTION(BlueprintCallable, Category = "Emote")
	void InitializeMenu(UFDEmoteComponent* InEmoteComponent);

protected:
	// 버튼들이 채워질 컨테이너 - 에디터 WBP에서 ScrollBox나 WrapBox를 이 이름으로 배치
	UPROPERTY(meta = (BindWidget))
	UPanelWidget* ButtonContainer;

	// 이모트 버튼 하나짜리 위젯 블루프린트 클래스 (에디터에서 WBP_FDEmoteButton 할당)
	UPROPERTY(EditDefaultsOnly, Category = "Emote")
	TSubclassOf<UFDEmoteButtonWidget> EmoteButtonClass;

private:
	UPROPERTY()
	UFDEmoteComponent* EmoteComponent;

	// DataTable을 순회해서 버튼들을 동적으로 생성
	void BuildEmoteButtons();

	// 버튼 클릭 시 호출되는 콜백 - 실제 재생 요청은 여기서 컴포넌트로 위임
	UFUNCTION()
	void HandleEmoteClicked(FName EmoteRowName);
};
