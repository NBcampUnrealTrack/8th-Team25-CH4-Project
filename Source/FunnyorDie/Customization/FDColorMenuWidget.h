// FDColorMenuWidget.h
// 색상 프리셋 선택 메뉴

/*
WBP에는 CanvasPanel 하나만 SwatchContainer 라는 이름으로 두면 되고,
버튼은 C++에서 팔레트 개수만큼 만들어 격자로 배치한다.
이모트 휠(UFDEmoteMenuWidget)이 쓰는 방식과 같은 구조다.

UButton::OnClicked는 인자가 없는 다이나믹 델리게이트라 어느 버튼이 눌렸는지 알 수 없다.
그래서 버튼마다 인덱스를 들고 있는 작은 핸들러 오브젝트를 하나씩 붙여
각자 자기 인덱스로 서버 요청을 보내게 했다.
*/

#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FDColorMenuWidget.generated.h"

class UCanvasPanel;
class UFDCustomizationComponent;

// 버튼 하나에 대응하는 클릭 핸들러 - 자기 팔레트 인덱스를 기억한다
UCLASS()
class FUNNYORDIE_API UFDColorSwatchHandler : public UObject
{
	GENERATED_BODY()

public:
	void Setup(UFDCustomizationComponent* InComponent, int32 InPresetIndex);

	UFUNCTION()
	void HandleClicked();

private:
	UPROPERTY()
	UFDCustomizationComponent* CustomizationComponent;

	int32 PresetIndex = -1;
};

UCLASS()
class FUNNYORDIE_API UFDColorMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// PlayerController가 위젯을 띄우기 직전에 호출
	UFUNCTION(BlueprintCallable, Category = "Customization")
	void InitializeMenu(UFDCustomizationComponent* InComponent);

protected:
	// 스와치 버튼이 배치될 캔버스 - WBP에서 Canvas Panel을 이 이름으로 배치
	UPROPERTY(meta = (BindWidget))
	UCanvasPanel* SwatchContainer;

	// 버튼 하나의 크기
	UPROPERTY(EditDefaultsOnly, Category = "Customization")
	FVector2D SwatchSize = FVector2D(72.f, 72.f);

	// 버튼 사이 간격
	UPROPERTY(EditDefaultsOnly, Category = "Customization")
	float SwatchPadding = 14.f;

	// 버튼 아래 이름표가 차지할 높이
	UPROPERTY(EditDefaultsOnly, Category = "Customization")
	float LabelHeight = 20.f;

	// 한 줄에 배치할 버튼 수
	UPROPERTY(EditDefaultsOnly, Category = "Customization")
	int32 ColumnCount = 4;

private:
	UPROPERTY()
	UFDCustomizationComponent* CustomizationComponent;

	// 핸들러가 GC로 날아가지 않게 붙잡아둠
	UPROPERTY()
	TArray<UFDColorSwatchHandler*> Handlers;

	// 팔레트를 순회하며 버튼을 만들고 격자로 배치
	void BuildSwatches();
};
