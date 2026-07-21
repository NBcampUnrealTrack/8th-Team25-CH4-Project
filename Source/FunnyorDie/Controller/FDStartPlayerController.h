// FDStartPlayerController.h

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FDStartPlayerController.generated.h"

class UFDStartWidget;

UCLASS()
class FUNNYORDIE_API AFDStartPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 메뉴 위젯 블루프린트 클래스
	UPROPERTY(EditDefaultsOnly, Category = "Menu")
	TSubclassOf<UFDStartWidget> StartWidgetClass;

	// 현재 떠있는 메뉴 위젯 인스턴스
	UPROPERTY()
	UFDStartWidget* StartWidget;
};