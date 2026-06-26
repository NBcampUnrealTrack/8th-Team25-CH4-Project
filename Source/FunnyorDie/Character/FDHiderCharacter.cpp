// FDHiderCharacter.cpp
// 숨는사람용 캐릭터

#include "Character/FDHiderCharacter.h"

AFDHiderCharacter::AFDHiderCharacter()
{
}

bool AFDHiderCharacter::IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget,
	const FVector& SrcLocation) const
{
	return Super::IsNetRelevantFor(RealViewer, ViewTarget, SrcLocation);
}

void AFDHiderCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AFDHiderCharacter::OnCaptureOverlap()
{
}
