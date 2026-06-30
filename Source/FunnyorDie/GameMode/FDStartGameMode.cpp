// FDStartGameMode.cpp

#include "GameMode/FDStartGameMode.h"
#include "Controller/FDStartPlayerController.h" 

AFDStartGameMode::AFDStartGameMode()
{
	DefaultPawnClass = nullptr;
	// 메뉴 화면이라 조작할 캐릭터 자체가 필요 없음
	// 애초에 아무 Pawn도 안 띄우게 막아두는 거라 GetDefaultPawnClassForController_Implementation
	// 오버라이드도 필요 없음

	PlayerControllerClass = AFDStartPlayerController::StaticClass();
}