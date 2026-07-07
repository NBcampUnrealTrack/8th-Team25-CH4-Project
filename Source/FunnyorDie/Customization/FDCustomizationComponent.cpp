// FDCustomizationComponent.cpp
//
// ※ Build.cs 확인 필요: PrivateDependencyModuleNames에 "ImageWrapper" 추가해야 함
//   (PNG 압축/디코딩에 IImageWrapper 모듈을 씀)

#include "Customization/FDCustomizationComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/Character.h"
#include "PlayerState/FDPlayerState.h"
#include "ImageUtils.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Modules/ModuleManager.h"

UFDCustomizationComponent::UFDCustomizationComponent()
{
	// 매 프레임 Tick 필요 없음 - RPC/이벤트가 들어올 때만 동작
	PrimaryComponentTick.bCanEverTick = false;

	// 이 컴포넌트 자체는 UPROPERTY로 복제할 값이 없음 (색상/스냅샷은 PlayerState 쪽에서 관리)
	SetIsReplicatedByDefault(false);
}

void UFDCustomizationComponent::BeginPlay()
{
	Super::BeginPlay();

	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (!OwnerChar) return;

	TargetMesh = OwnerChar->GetMesh();
	InitializeRenderResources();

	// 조인 타이밍에 따라 PlayerState 복제가 아직 안 왔을 수도 있어서 있으면 적용, 없으면 나중에 OnRep에서 적용됨
	if (const AFDPlayerState* FDPS = OwnerChar->GetPlayerState<AFDPlayerState>())
	{
		ApplyColorPreset(FDPS->CustomizationColors);

		if (FDPS->PaintSnapshot.IsValid())
		{
			ApplyPaintSnapshot(FDPS->PaintSnapshot);
		}
	}
}

void UFDCustomizationComponent::InitializeRenderResources()
{
	if (!TargetMesh) return;

	// 캐릭터 메시에 이미 세팅된 머티리얼을 베이스로 Dynamic Material Instance 생성
	// 베이스 머티리얼에는 ColorSlotParamNames / PaintMaskParamName 파라미터가 미리 만들어져 있어야 함
	UMaterialInterface* BaseMaterial = TargetMesh->GetMaterial(0);
	if (!BaseMaterial) return;

	DynamicMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
	TargetMesh->SetMaterial(0, DynamicMaterial);

	// 자유 페인팅용 RenderTarget 생성 - 서버/클라 각자 로컬로 하나씩 만듦 (Replicate 대상 아님)
	// ※ CreateCanvasRenderTarget2D는 UKismetRenderingLibrary가 아니라 UCanvasRenderTarget2D 클래스 자체의 static 함수임
	PaintRenderTarget = UCanvasRenderTarget2D::CreateCanvasRenderTarget2D(
		this, UCanvasRenderTarget2D::StaticClass(), RenderTargetSize, RenderTargetSize);

	if (DynamicMaterial && PaintRenderTarget)
	{
		DynamicMaterial->SetTextureParameterValue(PaintMaskParamName, PaintRenderTarget);
	}
}

void UFDCustomizationComponent::ApplyColorPreset(const FFDColorPreset& Preset)
{
	if (!DynamicMaterial) return;

	for (int32 i = 0; i < ColorSlotParamNames.Num() && i < Preset.SlotColors.Num(); ++i)
	{
		DynamicMaterial->SetVectorParameterValue(ColorSlotParamNames[i], Preset.SlotColors[i]);
	}
}

void UFDCustomizationComponent::Server_RequestColorPreset_Implementation(const FFDColorPreset& NewPreset)
{
	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (!OwnerChar) return;

	if (AFDPlayerState* FDPS = OwnerChar->GetPlayerState<AFDPlayerState>())
	{
		// TODO: 색상 값 유효성 검사 필요하면 여기 추가 (예: 특정 색 금지, 알파 강제 1.0 등)
		FDPS->CustomizationColors = NewPreset;

		// 서버 자기 자신은 OnRep이 자동 호출되지 않음 -> 수동 호출
		// (AFDHiderCharacter::SetInvincible에서 쓴 패턴과 동일)
		FDPS->OnRep_CustomizationColors();
	}
}

void UFDCustomizationComponent::Server_BeginStroke_Implementation()
{
	// 필요하면 여기서 Phase 체크 가능
	// 예: GetWorld()->GetGameState<AFDGameState>()->CurrentPhase != EMatchPhase::InGame 일 때만 페인팅 허용하고 싶다면 여기서 막으면 됨
	// (FDHiderCharacter::IsNetRelevantFor에서 쓴 GameState 참조 패턴 참고)
}

void UFDCustomizationComponent::Server_StrokePoint_Implementation(const FFDStrokePoint& Point)
{
	StampPointOnRenderTarget(Point); // 서버도 자기 RT에 반영해둬야 늦은 조인자 스냅샷을 구울 수 있음
	Multicast_StrokePoint(Point);
}

void UFDCustomizationComponent::Server_EndStroke_Implementation()
{
	BakeAndStoreSnapshot();
}

void UFDCustomizationComponent::Multicast_StrokePoint_Implementation(const FFDStrokePoint& Point)
{
	// 그림을 그린 본인 클라이언트는 RequestLocalPaint에서 이미 이 좌표를 예측 스탬프로 찍어놓은 상태
	// 여기서 같은 자리에 같은 색으로 한 번 더 찍어도 결과는 동일함 (누적이 아니라 덮어쓰는 방식이라 중복 스탬프 문제 없음)
	StampPointOnRenderTarget(Point);
}

void UFDCustomizationComponent::StampPointOnRenderTarget(const FFDStrokePoint& Point)
{
	if (!PaintRenderTarget || !BrushMaterial) return;

	// TODO: 매번 새로 DMI를 만들면 비용이 커서, 브러시 스탬프용 DMI는 InitializeRenderResources에서 미리 하나 만들어두고
	// 파라미터만 갱신하는 식으로 최적화하는 걸 권장 (지금은 로직 이해 우선으로 단순하게 작성)
	UMaterialInstanceDynamic* BrushDMI = UMaterialInstanceDynamic::Create(BrushMaterial, this);
	if (!BrushDMI) return;

	BrushDMI->SetVectorParameterValue(TEXT("BrushColor"), Point.Color);
	BrushDMI->SetScalarParameterValue(TEXT("BrushRadius"), Point.BrushRadius);
	BrushDMI->SetVectorParameterValue(TEXT("BrushCenter"), FLinearColor(Point.UV.X, Point.UV.Y, 0.f, 0.f));

	// ※ 확인 필요: DrawMaterialToRenderTarget이 매 호출마다 RT를 클리어하지 않고 누적 그리기가 되는지
	// (엔진 버전에 따라 동작이 다를 수 있어서, 그림이 매번 지워지면 ClearRenderTarget 관련 옵션을 따로 찾아봐야 함)
	UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, PaintRenderTarget, BrushDMI);
}

void UFDCustomizationComponent::BakeAndStoreSnapshot()
{
	// PlayerState 데이터를 직접 갱신하는 함수라 서버에서만 실행되어야 함
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	if (!PaintRenderTarget) return;

	FTextureRenderTargetResource* RTResource = PaintRenderTarget->GameThread_GetRenderTargetResource();
	if (!RTResource) return;

	TArray<FColor> RawPixels;
	RTResource->ReadPixels(RawPixels);

	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
	if (!ImageWrapper.IsValid()) return;

	ImageWrapper->SetRaw(RawPixels.GetData(), RawPixels.Num() * sizeof(FColor),
		RenderTargetSize, RenderTargetSize, ERGBFormat::BGRA, 8);

	FFDPaintSnapshot Snapshot;
	Snapshot.Width = RenderTargetSize;
	Snapshot.Height = RenderTargetSize;

	// GetCompressed가 TArray64를 반환하는 엔진 버전이 있어서 TArray로 안전하게 옮겨 담음
	const TArray64<uint8>& Compressed64 = ImageWrapper->GetCompressed(100);
	Snapshot.CompressedPixels.Append(Compressed64.GetData(), Compressed64.Num());

	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (AFDPlayerState* FDPS = OwnerChar ? OwnerChar->GetPlayerState<AFDPlayerState>() : nullptr)
	{
		FDPS->PaintSnapshot = Snapshot; // Replicated - 다른 클라이언트/늦은 조인자에게 자동 전파됨
	}

	UE_LOG(LogTemp, Log, TEXT("[커스터마이징] 페인트 스냅샷 저장 - %d바이트"), Snapshot.CompressedPixels.Num());
}

void UFDCustomizationComponent::ApplyPaintSnapshot(const FFDPaintSnapshot& Snapshot)
{
	if (!Snapshot.IsValid() || !PaintRenderTarget || !SnapshotCopyMaterial) return;

	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
	if (!ImageWrapper.IsValid()) return;

	if (!ImageWrapper->SetCompressed(Snapshot.CompressedPixels.GetData(), Snapshot.CompressedPixels.Num()))
	{
		return;
	}

	TArray64<uint8> RawData;
	if (!ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, RawData))
	{
		return;
	}

	// 디코딩한 픽셀을 임시 UTexture2D로 만들고, 그 텍스처를 RT에 풀스크린으로 한 번 그려서 통째로 복원
	// (스냅샷은 이미 완성된 그림이라 브러시로 다시 그릴 필요 없이 한 번에 복사하면 됨)
	FCreateTexture2DParameters TexParams;
	UTexture2D* DecodedTexture = FImageUtils::CreateTexture2D(
		Snapshot.Width, Snapshot.Height,
		TArray<FColor>(reinterpret_cast<FColor*>(RawData.GetData()), Snapshot.Width * Snapshot.Height),
		GetTransientPackage(), TEXT("FDDecodedPaint"), RF_Transient, TexParams);

	if (!DecodedTexture) return;

	UMaterialInstanceDynamic* CopyDMI = UMaterialInstanceDynamic::Create(SnapshotCopyMaterial, this);
	if (!CopyDMI) return;

	CopyDMI->SetTextureParameterValue(TEXT("SourceTexture"), DecodedTexture);
	UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, PaintRenderTarget, CopyDMI);
}

void UFDCustomizationComponent::RequestLocalPaint(const FVector2D& UV, bool bStrokeStart, bool bStrokeEnd)
{
	// 주의: 이 함수는 로컬에서 조작 중인 자기 캐릭터에 대해서만 호출되어야 함
	// (다른 사람 캐릭터 UV를 계산해서 여기로 넘기면 안 되니까, 호출부인 PlayerController 쪽에서
	//  히트 액터가 자기 Pawn이 맞는지 검증 후에 이 함수를 호출하도록 해뒀음)

	if (bStrokeStart)
	{
		Server_BeginStroke();
		LastSentUV = FVector2D(-1.f, -1.f);
	}

	// 너무 잦은 전송 방지 - 마지막으로 보낸 좌표에서 일정 거리 이상 움직였을 때만 서버로 보냄
	const bool bMovedEnough = LastSentUV.X < 0.f || FVector2D::Distance(UV, LastSentUV) >= MinSendDistanceUV;
	if (!bMovedEnough && !bStrokeEnd)
	{
		return;
	}

	FFDStrokePoint Point;
	Point.UV = UV;
	Point.Color = FLinearColor::Red; // TODO: 커스터마이징 UI에서 선택한 현재 브러시 색으로 교체
	Point.BrushRadius = 0.02f;       // TODO: 커스터마이징 UI에서 선택한 브러시 크기로 교체

	// 클라이언트 예측 - 서버 응답 기다리지 않고 내 화면엔 바로 반영 (렉 없이 그려지는 느낌)
	StampPointOnRenderTarget(Point);
	Server_StrokePoint(Point);

	LastSentUV = UV;

	if (bStrokeEnd)
	{
		Server_EndStroke();
	}
}
