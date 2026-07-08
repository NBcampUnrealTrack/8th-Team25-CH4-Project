// FDEmoteComponent.cpp

#include "Emote/FDEmoteComponent.h"
#include "GameFramework/Character.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/DataTable.h"

UFDEmoteComponent::UFDEmoteComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// 이모트는 "지금 상태"를 유지할 필요 없이 이벤트만 흘려보내면 되니까 컴포넌트 자체는 복제 안 함
	SetIsReplicatedByDefault(false);
}

void UFDEmoteComponent::Server_RequestPlayEmote_Implementation(FName EmoteRowName)
{
	// 연타 방지 - 쿨다운 안 지났으면 무시
	const float Now = GetWorld()->GetTimeSeconds();
	if (LastEmoteTime >= 0.f && (Now - LastEmoteTime) < EmoteCooldown)
	{
		UE_LOG(LogTemp, Log, TEXT("[이모트] 쿨다운 중이라 재생 무시: %s"), *EmoteRowName.ToString());
		return;
	}

	// TODO: 여기에 상황별 재생 제한 추가 (기획 확정되면 채울 것)
	// - 포획 판정 중인 캐릭터인지 (Tagger의 CapturedHider 관련 상태)
	// - Hider가 위장 중인지 (bIsDisguised)
	// - 매치가 GameOver 이후인지
	// 지금은 이동은 허용하는 조건이라 별도로 막지 않고 통과시킴

	if (!EmoteDataTable || !EmoteDataTable->FindRow<FFDEmoteData>(EmoteRowName, TEXT("이모트 유효성 검사")))
	{
		UE_LOG(LogTemp, Warning, TEXT("[이모트] 존재하지 않는 이모트 요청: %s"), *EmoteRowName.ToString());
		return;
	}

	LastEmoteTime = Now;
	Multicast_PlayEmote(EmoteRowName);
}

void UFDEmoteComponent::Multicast_PlayEmote_Implementation(FName EmoteRowName)
{
	if (!EmoteDataTable) return;

	const FFDEmoteData* Data = EmoteDataTable->FindRow<FFDEmoteData>(EmoteRowName, TEXT("이모트 조회"));
	if (!Data || !Data->Montage) return;

	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (!OwnerChar || !OwnerChar->GetMesh()) return;

	if (UAnimInstance* AnimInstance = OwnerChar->GetMesh()->GetAnimInstance())
	{
		// ※ 확인 필요: AnimBP에서 이 몽타주가 재생되는 슬롯이 UpperBody(또는 그에 준하는 슬롯)로
		// 설정되어 있어야 함. Slot이 FullBody거나 기본값이면 이동 중 하체 로코모션이 멈춰버림.
		// 애니메이터 쪽에 "이동 중에도 재생돼야 하니 UpperBody 슬롯으로 부탁"이라고 요청 필요
		AnimInstance->Montage_Play(Data->Montage);

		UE_LOG(LogTemp, Log, TEXT("[이모트] 재생: %s"), *EmoteRowName.ToString());
	}
}
