// FDCustomizationComponent.h
// 캐릭터 채색(커스터마이징) 공용 컴포넌트
// AFDHiderCharacter / AFDTaggerCharacter 양쪽에 동일하게 부착해서 사용
//
// 역할 두 가지:
// 1) 파츠 색상 프리셋 스왑 - Dynamic Material Instance 파라미터로 즉시 반영 (가벼움)
// 2) 자유 페인팅 - 로컬 RenderTarget에 브러시 스탬프를 찍고,
//    스트로크 좌표를 서버 경유로 다른 클라이언트에 실시간 전파
//
// 데이터 소유권 구조:
// - "현재 값"(색상 프리셋, 페인트 스냅샷)은 AFDPlayerState에 Replicated로 저장됨
//   -> 캐릭터는 AssignRoles()에서 재스폰되지만 PlayerState는 매치 내내 유지되기 때문
// - 이 컴포넌트는 "그 값을 실제로 메시에 적용하는 실행부" + "실시간 스트로크 RPC 중계" 역할만 함

#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Customization/FDCustomizationTypes.h"
#include "FDCustomizationComponent.generated.h"

class USkeletalMeshComponent;
class UMaterialInstanceDynamic;
class UCanvasRenderTarget2D;
class UMaterialInterface;

UCLASS(ClassGroup = (Customization), meta = (BlueprintSpawnableComponent))
class FUNNYORDIE_API UFDCustomizationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFDCustomizationComponent();

	virtual void BeginPlay() override;

	// ===== 파츠 색상 스왑 (프리셋 방식) =====

	// 클라이언트 -> 서버: 색상 프리셋 변경 요청
	// 옵션 메뉴/로비뿐 아니라 매치 중에도 호출 가능 (가벼운 파라미터 변경이라 실시간 허용)
	UFUNCTION(Server, Reliable)
	void Server_RequestColorPreset(const FFDColorPreset& NewPreset);

	// 실제로 로컬에서 머티리얼 파라미터에 색을 입히는 함수
	// AFDPlayerState::OnRep_CustomizationColors 에서 이 함수를 호출해서 적용함
	void ApplyColorPreset(const FFDColorPreset& Preset);

	// ===== 자유 페인팅 =====

	// 브러시 스트로크 시작 (펜을 뗐다 다시 찍는 경계) - Reliable로 확실히 전달
	UFUNCTION(Server, Reliable)
	void Server_BeginStroke();

	// 스트로크 중간 지점들 - Unreliable, 몇 개 유실돼도 티 안 남 (그림이라 약간 끊겨도 무방)
	UFUNCTION(Server, Unreliable)
	void Server_StrokePoint(const FFDStrokePoint& Point);

	// 스트로크 종료 - 이 시점에 서버가 RT를 구워서(PNG 압축) PlayerState에 스냅샷으로 저장
	UFUNCTION(Server, Reliable)
	void Server_EndStroke();

	// 서버 -> 모든 클라이언트: 스트로크 지점 전파
	// 그림을 그린 본인 클라이언트도 다시 받지만, 같은 좌표를 한 번 더 찍는 거라 결과에는 영향 없음 (주석 참고)
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_StrokePoint(const FFDStrokePoint& Point);

	// PlayerState의 PaintSnapshot이 갱신됐을 때(늦은 조인, 재스폰, 관전 시야 복귀 등) 복원용으로 호출
	void ApplyPaintSnapshot(const FFDPaintSnapshot& Snapshot);

	// 로컬 플레이어 입력에서 호출 - PlayerController가 커서 아래 UV를 계산해서 넘겨줌
	UFUNCTION(BlueprintCallable, Category = "Customization")
	void RequestLocalPaint(const FVector2D& UV, bool bStrokeStart, bool bStrokeEnd);

protected:
	// 채색이 적용될 스켈레탈 메시 (Owner->GetMesh())
	UPROPERTY()
	USkeletalMeshComponent* TargetMesh;

	// 런타임에 만들어지는 Dynamic Material Instance (파츠 색상 + 페인팅 마스크 합성용)
	UPROPERTY()
	UMaterialInstanceDynamic* DynamicMaterial;

	// 자유 페인팅 결과가 그려지는 RenderTarget - 로컬 전용, Replicate 안 됨
	// (서버도 자기 몫으로 하나 만들어서 늦은 조인자 스냅샷 굽기용으로 사용)
	UPROPERTY()
	UCanvasRenderTarget2D* PaintRenderTarget;

	// RT 해상도 - 크면 그림 디테일은 좋아지지만 스냅샷 압축/전송 비용도 커짐
	UPROPERTY(EditDefaultsOnly, Category = "Customization")
	int32 RenderTargetSize = 512;

	// 브러시 스탬프용 머티리얼
	// 에디터에서 만들어야 함: 파라미터 "BrushColor"(Vector), "BrushCenter"(Vector), "BrushRadius"(Scalar)를 받아서
	// UV와 BrushCenter 간 거리가 BrushRadius 이내인 영역만 BrushColor로 칠하는 원형 브러시 머티리얼
	UPROPERTY(EditDefaultsOnly, Category = "Customization")
	UMaterialInterface* BrushMaterial;

	// 스냅샷 복원(디코딩한 텍스처를 RT에 통째로 복사)용 머티리얼
	// BrushMaterial과 목적이 달라서 별도로 분리함 - 파라미터 "SourceTexture"(Texture) 하나만 받아서 그대로 그리는 단순 머티리얼
	UPROPERTY(EditDefaultsOnly, Category = "Customization")
	UMaterialInterface* SnapshotCopyMaterial;

	// DynamicMaterial에서 페인팅 RT를 연결할 텍스처 파라미터 이름
	UPROPERTY(EditDefaultsOnly, Category = "Customization")
	FName PaintMaskParamName = TEXT("PaintMask");

	// EFDColorSlot 순서에 맞춰 머티리얼 벡터 파라미터 이름 지정
	UPROPERTY(EditDefaultsOnly, Category = "Customization")
	TArray<FName> ColorSlotParamNames = { TEXT("BodyColor"), TEXT("AccentColor"), TEXT("EyeColor") };

private:
	// RenderTarget 최초 생성 및 DynamicMaterial 세팅
	void InitializeRenderResources();

	// 한 점을 실제 RT에 스탬프로 찍는 내부 함수 (본인 예측 그리기 / Multicast 수신 양쪽에서 공용으로 사용)
	void StampPointOnRenderTarget(const FFDStrokePoint& Point);

	// 서버 전용: 스트로크 종료 시 RT를 읽어서 PNG로 압축 후 PlayerState에 저장
	void BakeAndStoreSnapshot();

	// 로컬에서 마지막으로 보낸 스트로크 좌표 - 너무 잦은 전송 방지용 샘플링 기준점
	FVector2D LastSentUV = FVector2D(-1.f, -1.f);

	// 최소 이동 거리(UV 공간 기준) 이상 움직였을 때만 서버로 전송
	static constexpr float MinSendDistanceUV = 0.01f;
};
