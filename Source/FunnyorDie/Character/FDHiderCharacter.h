// FDHiderCharacter.h
// 숨는사람용 캐릭터

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Engine/DataTable.h"
#include "GameDataTypes.h"  
#include "FDHiderCharacter.generated.h"

// 위장 사물의 크기 정보를 데이터 테이블로 관리하기 위한 행 구조체
// 에디터에서 DataTable 에셋에 행을 추가해 사물별 캡슐 크기를 설정
USTRUCT(BlueprintType)
struct FDisguiseShapeData : public FTableRowBase
{
	GENERATED_BODY()

	// 캡슐 반지름 (가로 반경)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float CapsuleRadius = 34.f;

	// 캡슐 반높이 (세로 반경)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float CapsuleHalfHeight = 88.f;
};

UCLASS()
class FUNNYORDIE_API AFDHiderCharacter : public ACharacter
{
	GENERATED_BODY()
	
public:
	AFDHiderCharacter();

	// 정찰 단계 시야 차단용
	virtual bool IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const override;
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 위장 상태 진입 요청 (클라이언트 → 서버)
	UFUNCTION(Server, Reliable)
	void Server_EnterDisguise(FName DisguiseRowName);

	// 위장 상태 해제 요청 (클라이언트 → 서버)
	UFUNCTION(Server, Reliable)
	void Server_ExitDisguise();
	
	// 서버 전용: 무적 상태 설정 (봐주기 등 TaggerCharacter/GameMode에서 호출)
	void SetInvincible(bool bNewInvincible);

	// 현재 무적 여부 조회 ( bIsInvincible이 복제되므로 클라이언트/서버 모두 사용 가능)
	bool IsInvincible() const { return bIsInvincible; }

	// 아이템 투명화 on/off
	void SetItemInvisible(bool bNewInvisible);

	// 동상 오브젝트와 상호작용해서 머리 장비 장착 요청 (클라이언트 → 서버)
	// 상호작용 시스템(E키/트레이스 등, 팀원 확인 예정)이 무엇이든 이 함수 하나만 호출하면 연결됨
	UFUNCTION(Server, Reliable)
	void Server_RequestEquipHead(FName HeadRowName);
	
	// 소리 아이템 발동 - 모든 클라이언트에서 하이더 위치에 사운드 재생
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayNoise(float Duration);
	
protected:
	virtual void BeginPlay() override;

	// 위장 상태 여부, 서버에서 변경 시 클라이언트에 자동 전파
	UPROPERTY(ReplicatedUsing = OnRep_bIsDisguised)
	bool bIsDisguised = false;

	// 봐주기 등으로 인한 무적 상태, 서버에서 변경 시 클라이언트에 자동 전파
	UPROPERTY(ReplicatedUsing = OnRep_bIsInvincible)
	bool bIsInvincible = false;
	
	// bIsDisguised 복제 시 클라이언트에서 호출되는 콜백
	UFUNCTION()
	void OnRep_bIsDisguised();
	
	// bIsInvincible 복제 시 클라이언트에서 호출되는 콜백
	UFUNCTION()
	void OnRep_bIsInvincible();

	// 위장 진입 시 애님 재생 속도 0으로 고정 (모든 클라이언트에 동기화)
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_FreezeAnimation(bool bFreeze);

	// 캡슐 크기를 데이터 테이블 값에 맞춰 변경 (모든 클라이언트에 동기화)
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_ResizeCapsule(float NewRadius, float NewHalfHeight);

	// Overlap 됐을 때 호출될 함수
	void OnCaptureOverlap();

	// 채색(커스터마이징) 공용 컴포넌트 - Tagger 쪽에도 동일하게 부착됨
	// PlayerState의 색상/페인트 스냅샷을 실제 메시에 적용하는 실행부 역할
	UPROPERTY(VisibleAnywhere, Category = "Customization")
	class UFDCustomizationComponent* CustomizationComp;

	// 이모트(감정표현) 공용 컴포넌트 - Tagger 쪽에도 동일하게 부착됨
	UPROPERTY(VisibleAnywhere, Category = "Emote")
	class UFDEmoteComponent* EmoteComp;
	
	// 각자 가질 아이템 인벤토리
	UPROPERTY(VisibleAnywhere, Category = "Item")
	class UFDItemInventoryComponent* ItemInventoryComp;

	// 아이템 투명화 상태 
	UPROPERTY(ReplicatedUsing = OnRep_bIsItemInvisible)
	bool bIsItemInvisible = false;

	// bIsItemInvisible 복제 시 호출되는 콜백
	UFUNCTION()
	void OnRep_bIsItemInvisible();

	// 현재 장착 중인 동상 머리 행 이름 - Hider 전용 기능이라 컴포넌트로 안 빼고 직접 관리
	// 아무것도 장착 안 했으면 NAME_None
	UPROPERTY(ReplicatedUsing = OnRep_EquippedHeadRow)
	FName EquippedHeadRow = NAME_None;

	// EquippedHeadRow 복제 시 클라이언트에서 호출 - 실제로 메시를 갈아끼우는 지점
	UFUNCTION()
	void OnRep_EquippedHeadRow();

	// 동상 머리가 실제로 붙는 메시 컴포넌트 - 생성자에서 만들어두고 머리 소켓에 부착, 평소엔 숨겨둠
	UPROPERTY(VisibleAnywhere, Category = "Equip")
	class UStaticMeshComponent* HeadEquipMesh;

	// 하이더는 3인칭 고정 - SpringArm으로 카메라를 캐릭터 뒤에 띄움
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	class UCameraComponent* FollowCamera;

	// 동상 머리 데이터 테이블 (에디터에서 DT_HeadEquip 할당)
	UPROPERTY(EditDefaultsOnly, Category = "Equip")
	UDataTable* HeadEquipDataTable;

	// HeadEquipMesh를 붙일 스켈레톤 소켓 이름 (에디터에서 실제 소켓 이름에 맞춰 수정 필요)
	UPROPERTY(EditDefaultsOnly, Category = "Equip")
	FName HeadSocketName = TEXT("head");
	
private:
	// 위장 사물 크기 데이터 테이블 (에디터에서 할당)
	UPROPERTY(EditDefaultsOnly, Category = "Disguise")
	UDataTable* DisguiseDataTable;

	// 밸런스 수치 데이터 테이블 (에디터에서 DT_MatchBalanceSettings 할당)
	UPROPERTY(EditDefaultsOnly, Category = "Balance")
	UDataTable* BalanceDataTable;

	// 위장 해제 전 원래 캡슐 크기 저장용
	float OriginalCapsuleRadius = 34.f;
	float OriginalCapsuleHalfHeight = 88.f;

	// 무적/속도버프 전 원래 걷기 속도 저장용 (BeginPlay에서 자동 저장됨)
	float DefaultWalkSpeed = 600.f;

	// 데이터 테이블에서 밸런스 설정값을 가져오는 헬퍼 함수
	const FMatchBalanceSettings* GetBalanceSettings() const;
	
	// 소리 아이템용 사운드 (에디터에서 할당)
	UPROPERTY(EditDefaultsOnly, Category = "Item")
	class USoundBase* NoiseSound;
	
	// 재생 중인 소리 - 타이머로 정지시키려면 붙잡고 있어야 함
	UPROPERTY()
	class UAudioComponent* ActiveNoiseAudio;

	// 소리 정지 타이머
	FTimerHandle NoiseStopTimerHandle;

	// 소리 정지
	void StopNoise();
};
