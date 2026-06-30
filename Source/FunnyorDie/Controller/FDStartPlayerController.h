// FDStartPlayerController.h

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FDStartPlayerController.generated.h"

UCLASS()
class FUNNYORDIE_API AFDStartPlayerController : public APlayerController
{
	GENERATED_BODY()

	// 지금은 빈 채로 둠
	// 메뉴 위젯 띄우기에서 BeginPlay 오버라이드해서
	// CreateWidget + AddToViewport + SetInputMode(UIOnly) 추가할 예정
};