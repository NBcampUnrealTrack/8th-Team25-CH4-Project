// FDPlayerState.h
//
// ※ 주의: 원본 FDPlayerState.h 전체 파일이 없어서, 기존 코드(FDGameMode.cpp, FDLobbyGameMode.cpp,
// FDHiderCharacter.cpp 등)에서 참조된 RoleTag / bIsHost / bIsAlive 멤버만 기준으로 복원했고
// 거기에 커스터마이징 필드(CustomizationColors, PaintSnapshot)를 추가한 버전임.
// 실제 프로젝트 파일에 다른 멤버/함수가 더 있다면, 이 파일을 통째로 덮어쓰지 말고
// 아래 "커스터마이징 필드" 섹션만 실제 파일에 옮겨 넣어줘.

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Customization/FDCustomizationTypes.h"
#include "FDPlayerState.generated.h"

// 기존 코드에서 참조되던 역할 enum (FDGameMode.cpp 등에서 이미 사용 중)
UENUM(BlueprintType)
enum class EFDRole : uint8
{
	None,
	Tagger,
	Hider
};

UCLASS()
class FUNNYORDIE_API AFDPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ===== 기존 필드 (참조 기반 복원 - 실제 파일에 이미 있으면 중복 선언하지 말 것) =====

	UPROPERTY(Replicated)
	EFDRole RoleTag = EFDRole::None;

	UPROPERTY(Replicated)
	bool bIsHost = false;

	UPROPERTY(Replicated)
	bool bIsAlive = true;
	
	// 잡힌 서버 시각 
	// -1이면 아직 안 잡힘 (=생존 중)
	UPROPERTY(Replicated)
	float DeathServerTime = -1.f;

	// ===== 커스터마이징 필드 (신규 추가) =====

	// 파츠 색상 프리셋 - 서버에서 값 변경 시 OnRep으로 클라이언트에 자동 전파
	UPROPERTY(ReplicatedUsing = OnRep_CustomizationColors)
	FFDColorPreset CustomizationColors;

	// 자유 페인팅 결과 스냅샷 - 늦은 조인/재스폰/관전 시야 복귀 시 복원용
	UPROPERTY(ReplicatedUsing = OnRep_PaintSnapshot)
	FFDPaintSnapshot PaintSnapshot;

	// 색상 프리셋 복제 수신 시 호출 - 소유 Pawn의 커스터마이징 컴포넌트에 적용을 위임함
	// UFDCustomizationComponent::Server_RequestColorPreset에서 서버 자기 자신에 대해 수동 호출도 함
	UFUNCTION()
	void OnRep_CustomizationColors();

	// 페인트 스냅샷 복제 수신 시 호출
	UFUNCTION()
	void OnRep_PaintSnapshot();
};
