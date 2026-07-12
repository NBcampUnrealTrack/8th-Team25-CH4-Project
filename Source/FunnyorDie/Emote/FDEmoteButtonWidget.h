// FDEmoteButtonWidget.h
// 이모트 메뉴 안에 목록 형태로 동적 생성되는 버튼 하나짜리 위젯
// 클릭되면 자기 RowName을 들고 델리게이트로 상위(FDEmoteMenuWidget)에 알림

#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FDEmoteButtonWidget.generated.h"

class UButton;
class UTextBlock;
class UImage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEmoteClicked, FName, EmoteRowName);

UCLASS()
class FUNNYORDIE_API UFDEmoteButtonWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 상위(FDEmoteMenuWidget)에서 이 델리게이트를 구독해서 클릭 이벤트를 받음
	UPROPERTY(BlueprintAssignable, Category = "Emote")
	FOnEmoteClicked OnEmoteClicked;

	// 메뉴 위젯이 버튼 생성 직후 호출해서 표시 정보를 채워줌
	void Setup(FName InRowName, const FText& InDisplayName, UTexture2D* InIcon);

	// 등록된 이모트가 없는 슬롯을 빈 슬롯으로 표시 (휠 모양은 유지하되 클릭 비활성화)
	void SetupEmpty();

protected:
	virtual void NativeConstruct() override;

	// 아래 세 개는 에디터에서 WBP_FDEmoteButton 만들 때 같은 이름으로 위젯 배치해야 자동 연결됨
	UPROPERTY(meta = (BindWidget))
	UButton* ClickButton;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* NameText;

	UPROPERTY(meta = (BindWidget))
	UImage* IconImage;

private:
	FName RowName;
	bool bIsEmptySlot = false;

	UFUNCTION()
	void OnButtonClicked();
};
