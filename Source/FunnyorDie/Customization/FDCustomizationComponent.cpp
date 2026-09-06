// FDCustomizationComponent.cpp

#include "Customization/FDCustomizationComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "GameFramework/Character.h"
#include "PlayerState/FDPlayerState.h"

UFDCustomizationComponent::UFDCustomizationComponent()
{
	// 매 프레임 Tick 필요 없음 - RPC와 OnRep이 들어올 때만 동작
	PrimaryComponentTick.bCanEverTick = false;

	/*
	복제할 UPROPERTY는 없지만 이 플래그는 반드시 켜야 한다.
	UActorComponent의 Server RPC는 서버가 대상 서브오브젝트를 찾아야 실행되는데,
	컴포넌트가 복제 대상이 아니면 그 조회가 실패해서 RPC가 조용히 사라진다.
	*/
	SetIsReplicatedByDefault(true);

	// 에디터 세팅 없이도 바로 고를 게 있도록 기본 팔레트를 코드에 넣어둠
	Palette = {
		FFDColorPreset(FText::FromString(TEXT("레드")),
			FLinearColor(0.75f, 0.10f, 0.10f), FLinearColor(1.00f, 0.85f, 0.30f), FLinearColor::White),
		FFDColorPreset(FText::FromString(TEXT("블루")),
			FLinearColor(0.10f, 0.30f, 0.80f), FLinearColor(0.85f, 0.95f, 1.00f), FLinearColor::White),
		FFDColorPreset(FText::FromString(TEXT("그린")),
			FLinearColor(0.15f, 0.60f, 0.25f), FLinearColor(0.95f, 0.95f, 0.60f), FLinearColor::White),
		FFDColorPreset(FText::FromString(TEXT("옐로")),
			FLinearColor(0.95f, 0.80f, 0.15f), FLinearColor(0.35f, 0.25f, 0.10f), FLinearColor::White),
		FFDColorPreset(FText::FromString(TEXT("퍼플")),
			FLinearColor(0.45f, 0.20f, 0.70f), FLinearColor(0.95f, 0.75f, 1.00f), FLinearColor::White),
		FFDColorPreset(FText::FromString(TEXT("오렌지")),
			FLinearColor(0.95f, 0.45f, 0.10f), FLinearColor(0.20f, 0.20f, 0.20f), FLinearColor::White),
		FFDColorPreset(FText::FromString(TEXT("화이트")),
			FLinearColor(0.92f, 0.92f, 0.92f), FLinearColor(0.30f, 0.55f, 0.85f), FLinearColor::Black),
		FFDColorPreset(FText::FromString(TEXT("블랙")),
			FLinearColor(0.08f, 0.08f, 0.10f), FLinearColor(0.90f, 0.20f, 0.20f), FLinearColor::White)
	};
}

void UFDCustomizationComponent::BeginPlay()
{
	Super::BeginPlay();

	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (!OwnerChar) return;

	TargetMesh = OwnerChar->GetMesh();
	InitializeMaterials();
	LogMissingParameters();

	/*
	조인 타이밍에 따라 PlayerState 복제가 아직 안 왔을 수 있다.
	있으면 지금 적용하고, 없으면 나중에 OnRep_ColorPresetIndex가 적용한다.
	재스폰(역할 배정 직후)으로 컴포넌트가 새로 생기는 경우에도 이 경로로 복원된다.
	*/
	if (const AFDPlayerState* FDPS = OwnerChar->GetPlayerState<AFDPlayerState>())
	{
		ApplyPresetIndex(FDPS->ColorPresetIndex);
	}
}

void UFDCustomizationComponent::InitializeMaterials()
{
	SlotMaterials.Reset();
	if (!TargetMesh) return;

	/*
	0번 슬롯만 처리하면 몸통과 얼굴이 다른 머티리얼로 나뉜 캐릭터에서
	한쪽만 색이 바뀐다. 슬롯 개수만큼 전부 DMI를 만들어 끼운다.
	해당 슬롯에 파라미터가 없으면 그 슬롯은 그냥 아무 변화가 없을 뿐 문제되지 않는다.
	*/
	const int32 SlotCount = TargetMesh->GetNumMaterials();
	for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
	{
		UMaterialInterface* BaseMaterial = TargetMesh->GetMaterial(SlotIndex);
		if (!BaseMaterial) continue;

		UMaterialInstanceDynamic* DMI = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		if (!DMI) continue;

		TargetMesh->SetMaterial(SlotIndex, DMI);
		SlotMaterials.Add(DMI);
	}

	if (SlotMaterials.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[커스터마이징] 머티리얼 슬롯을 하나도 못 만듦 - 메시에 머티리얼이 없음"));
	}
}

void UFDCustomizationComponent::LogMissingParameters() const
{
	if (SlotMaterials.Num() == 0) return;

	for (const FName& ParamName : ColorSlotParamNames)
	{
		bool bFoundAnywhere = false;

		for (const UMaterialInstanceDynamic* DMI : SlotMaterials)
		{
			if (!DMI) continue;

			FLinearColor Dummy;
			// 파라미터가 존재하면 true를 반환한다
			if (DMI->GetVectorParameterValue(FMaterialParameterInfo(ParamName), Dummy))
			{
				bFoundAnywhere = true;
				break;
			}
		}

		if (!bFoundAnywhere)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[커스터마이징] 머티리얼에 벡터 파라미터 '%s' 가 없음 - 이 슬롯은 색이 안 바뀜"),
				*ParamName.ToString());
		}
	}
}

int32 UFDCustomizationComponent::GetCurrentPresetIndex() const
{
	const ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	const AFDPlayerState* FDPS = OwnerChar ? OwnerChar->GetPlayerState<AFDPlayerState>() : nullptr;

	return FDPS ? FDPS->ColorPresetIndex : -1;
}

void UFDCustomizationComponent::ApplyPresetIndex(int32 PresetIndex)
{
	// -1은 아직 아무것도 고르지 않은 상태라 기본 머티리얼 그대로 둔다
	if (!Palette.IsValidIndex(PresetIndex)) return;
	if (SlotMaterials.Num() == 0) return;

	const FFDColorPreset& Preset = Palette[PresetIndex];

	for (UMaterialInstanceDynamic* DMI : SlotMaterials)
	{
		if (!DMI) continue;

		for (int32 i = 0; i < ColorSlotParamNames.Num() && i < Preset.SlotColors.Num(); ++i)
		{
			DMI->SetVectorParameterValue(ColorSlotParamNames[i], Preset.SlotColors[i]);
		}
	}
}

bool UFDCustomizationComponent::Server_RequestPresetIndex_Validate(int32 NewIndex)
{
	// 팔레트 범위를 벗어난 인덱스는 애초에 서버가 받지 않는다
	return Palette.IsValidIndex(NewIndex);
}

void UFDCustomizationComponent::Server_RequestPresetIndex_Implementation(int32 NewIndex)
{
	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (!OwnerChar) return;

	AFDPlayerState* FDPS = OwnerChar->GetPlayerState<AFDPlayerState>();
	if (!FDPS) return;

	if (FDPS->ColorPresetIndex == NewIndex) return; // 같은 값이면 복제 트래픽 낭비

	FDPS->ColorPresetIndex = NewIndex;

	/*
	서버(리슨 호스트) 자기 자신은 OnRep이 자동 호출되지 않으므로 직접 부른다.
	이 프로젝트의 AFDGameState::SetPhase, FinalizeHiderRanking에서 쓰는 것과 같은 패턴이다.
	*/
	FDPS->OnRep_ColorPresetIndex();

	UE_LOG(LogTemp, Log, TEXT("[커스터마이징] %s 프리셋 %d 적용"),
		*FDPS->GetPlayerName(), NewIndex);
}
