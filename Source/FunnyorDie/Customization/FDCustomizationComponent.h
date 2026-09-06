// FDCustomizationComponent.h
// 캐릭터 색상 커스터마이징 공용 컴포넌트
// AFDHiderCharacter / AFDTaggerCharacter 양쪽에 동일하게 부착해서 사용

/*
데이터 소유권 구조는 그대로 유지했다.
현재 고른 값(팔레트 인덱스)은 AFDPlayerState가 Replicated로 들고 있고,
이 컴포넌트는 그 값을 실제 메시에 적용하는 실행부다.
캐릭터는 역할 배정 때 재스폰되지만 PlayerState는 매치 내내 살아있기 때문이다.

이전 버전 대비 바뀐 점 세 가지.

1) SetIsReplicatedByDefault를 true로 켰다.
   컴포넌트가 복제 대상이 아니면 클라이언트가 부른 Server RPC가 서버에서
   대상 서브오브젝트를 찾지 못해 그냥 무시된다. 스탠드얼론에서는 RPC 라우팅 없이
   로컬 실행되어 정상 동작하는 것처럼 보이기 때문에 1인 테스트로는 절대 안 잡히는 문제였다.

2) 색상 값 대신 팔레트 인덱스를 주고받는다.
   복제량이 줄고, 클라이언트가 임의의 색을 서버에 밀어넣을 수 없게 된다.

3) 머티리얼 슬롯을 0번만이 아니라 전부 순회한다.
   몸통과 얼굴이 다른 슬롯으로 나뉜 캐릭터에서 0번만 색이 바뀌던 문제를 막는다.
*/

#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Customization/FDCustomizationTypes.h"
#include "FDCustomizationComponent.generated.h"

class USkeletalMeshComponent;
class UMaterialInstanceDynamic;

UCLASS(ClassGroup = (Customization), meta = (BlueprintSpawnableComponent))
class FUNNYORDIE_API UFDCustomizationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFDCustomizationComponent();

	virtual void BeginPlay() override;

	// 클라이언트 -> 서버: 팔레트 인덱스 변경 요청. 서버가 범위를 검증한다
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestPresetIndex(int32 NewIndex);

	// 로컬에서 실제로 머티리얼에 색을 입히는 함수
	// AFDPlayerState::OnRep_ColorPresetIndex가 이 함수를 호출한다
	void ApplyPresetIndex(int32 PresetIndex);

	// UI가 버튼을 만들 때 읽어가는 팔레트
	const TArray<FFDColorPreset>& GetPalette() const { return Palette; }

	// UI가 현재 선택 상태를 표시할 때 사용
	UFUNCTION(BlueprintPure, Category = "Customization")
	int32 GetCurrentPresetIndex() const;

protected:
	/*
	선택 가능한 색상 프리셋 목록.
	코드에 기본값이 들어 있어 에디터 세팅 없이도 바로 동작하고,
	BP에서 덮어쓰면 그 값이 우선한다.
	팔레트는 클래스 기본값이라 서버와 모든 클라이언트가 항상 동일하다.
	이게 인덱스만 복제해도 되는 근거다.
	*/
	UPROPERTY(EditDefaultsOnly, Category = "Customization")
	TArray<FFDColorPreset> Palette;

	// EFDColorSlot 순서에 맞춘 머티리얼 벡터 파라미터 이름
	UPROPERTY(EditDefaultsOnly, Category = "Customization")
	TArray<FName> ColorSlotParamNames = { TEXT("BodyColor"), TEXT("AccentColor"), TEXT("EyeColor") };

private:
	// 채색이 적용될 스켈레탈 메시 (Owner->GetMesh())
	UPROPERTY()
	USkeletalMeshComponent* TargetMesh;

	// 머티리얼 슬롯마다 하나씩 만들어두는 Dynamic Material Instance
	UPROPERTY()
	TArray<UMaterialInstanceDynamic*> SlotMaterials;

	// 모든 머티리얼 슬롯에 DMI를 만들어 끼운다
	void InitializeMaterials();

	/*
	베이스 머티리얼에 ColorSlotParamNames의 벡터 파라미터가 실제로 존재하는지 확인해 로그로 남긴다.
	파라미터가 없으면 SetVectorParameterValue는 아무 에러 없이 조용히 무시되기 때문에,
	이 로그가 없으면 색이 안 바뀌는 원인이 코드인지 머티리얼인지 구분할 방법이 없다.
	*/
	void LogMissingParameters() const;
};
