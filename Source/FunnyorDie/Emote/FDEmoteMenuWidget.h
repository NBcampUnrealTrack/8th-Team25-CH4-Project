// FDEmoteMenuWidget.h
// 이모트 목록을 원형 휠(PUBG/몬헌 스타일)로 배치해서 보여주는 위젯
// DataTable을 순회해서 버튼을 동적으로 생성하고, 각도 계산으로 원 둘레에 배치함
// 배경이 투명해서 GameAndUI 입력 모드로 열면 뒤에 실제 게임 화면(캐릭터)이 그대로 비쳐 보임

#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FDEmoteMenuWidget.generated.h"

class UCanvasPanel;
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
	// 버튼들이 원형으로 배치될 캔버스 - 에디터 WBP에서 Canvas Panel을 이 이름으로 배치
	// (원형 배치는 좌표 지정이 자유로운 CanvasPanel에서만 가능해서 WrapBox/ScrollBox 대신 이걸 씀)
	UPROPERTY(meta = (BindWidget))
	UCanvasPanel* ButtonContainer;

	// 이모트 버튼 하나짜리 위젯 블루프린트 클래스 (에디터에서 WBP_FDEmoteButton 할당)
	UPROPERTY(EditDefaultsOnly, Category = "Emote")
	TSubclassOf<UFDEmoteButtonWidget> EmoteButtonClass;

	// 휠 반지름 (캔버스 중심으로부터 버튼 중심까지의 거리, 픽셀 단위)
	UPROPERTY(EditDefaultsOnly, Category = "Emote")
	float WheelRadius = 220.f;

	// 버튼 하나의 크기
	UPROPERTY(EditDefaultsOnly, Category = "Emote")
	FVector2D ButtonSize = FVector2D(80.f, 80.f);

	// 휠에 항상 표시할 슬롯 개수 (고정값) - DataTable에 이 개수보다 적게 등록돼 있어도
	// 나머지는 빈 슬롯으로 채워서 휠 모양 자체는 항상 유지되게 함 (롤/오버워치 스킬 슬롯 같은 방식)
	UPROPERTY(EditDefaultsOnly, Category = "Emote")
	int32 WheelSlotCount = 8;

private:
	UPROPERTY()
	UFDEmoteComponent* EmoteComponent;

	// DataTable을 순회해서 버튼들을 원형으로 배치하며 생성
	void BuildEmoteButtons();

	// 버튼 클릭 시 호출되는 콜백 - 실제 재생 요청은 여기서 컴포넌트로 위임
	UFUNCTION()
	void HandleEmoteClicked(FName EmoteRowName);
};

