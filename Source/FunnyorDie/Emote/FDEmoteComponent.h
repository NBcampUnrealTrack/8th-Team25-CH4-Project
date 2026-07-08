// FDEmoteComponent.h
// 이모트(감정표현 애니메이션) 재생 공용 컴포넌트
// Hider/Tagger 양쪽 캐릭터에 동일하게 부착
//
// 채색 컴포넌트랑 다르게 이모트는 "지금 상태"를 들고 있을 필요가 없음 (재생하고 끝나면 그만)
// 그래서 PlayerState에 데이터를 안 두고, 이 컴포넌트 안에서 RPC로 바로 처리함

#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameDataTypes.h"
#include "FDEmoteComponent.generated.h"

class UDataTable;

UCLASS(ClassGroup = (Emote), meta = (BlueprintSpawnableComponent))
class FUNNYORDIE_API UFDEmoteComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFDEmoteComponent();

	// 클라이언트 -> 서버: 이모트 재생 요청 (UI 버튼 클릭에서 호출)
	UFUNCTION(Server, Reliable)
	void Server_RequestPlayEmote(FName EmoteRowName);

	// UI(FDEmoteMenuWidget)에서 목록을 그릴 때 데이터 테이블에 접근하기 위한 getter
	UDataTable* GetEmoteDataTable() const { return EmoteDataTable; }

protected:
	// 이모트 목록 데이터 테이블 (에디터에서 DT_Emotes 할당, 행 구조체는 FFDEmoteData)
	UPROPERTY(EditDefaultsOnly, Category = "Emote")
	UDataTable* EmoteDataTable;

	// 같은 이모트 연타 방지용 최소 재생 간격 (초)
	UPROPERTY(EditDefaultsOnly, Category = "Emote")
	float EmoteCooldown = 1.0f;

private:
	// 서버 -> 모든 클라이언트: 실제로 몽타주 재생
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayEmote(FName EmoteRowName);

	// 마지막 이모트 재생 시각 (서버 기준, 연타 방지 체크용)
	float LastEmoteTime = -1.f;
};
