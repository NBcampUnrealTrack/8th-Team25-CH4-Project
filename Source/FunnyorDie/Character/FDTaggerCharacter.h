// FDTaggerCharacter.h
// 술래용 캐릭터

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameDataTypes.h" 
#include "FDTaggerCharacter.generated.h"

// 밸런스 데이터 테이블 행 구조체 (FMatchBalanceSettings)
// 포획 대기 시간, 무적 시간, 속도 배율 등 수치 조정용
// 상황보고 따로 빼서 관리할 수 도 있음 --> GameDataTypes.h로 이동


UCLASS()
class FUNNYORDIE_API AFDTaggerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AFDTaggerCharacter();

	// 외부(GameMode 등)에서 포획 대상을 직접 지정해 포획 시퀀스 시작
	UFUNCTION(BlueprintCallable)
	void StartCaptureSequence(ACharacter* TargetHider);
	
	virtual void BeginPlay() override;

	void ForceOut();       // 아웃 선택시 호출되는 함수
	void RequestSpare();   // 아웃 선택시 호출되는 함수
	
	// 공격 콜리전이 숨는 자와 Overlap됐을 때 서버에서 포획 판정 실행
	UFUNCTION()
	void OnCaptureCollisionOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	// 공격 콜리전 활성화 요청 (클라이언트 → 서버)
	UFUNCTION(Server, Reliable)
	void Server_TryCapture();

	// 관전 모드 (GameMode가 Phase 변화에 맞춰 호출)
	UFUNCTION(BlueprintCallable)
	void SetScoutingMode(bool bEnable);
	
private:
	// 관전모드때 술래 메시 안보이게 하기
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SetMeshVisibility(bool bVisible);
	
	// 포획 판정용 콜리전 컴포넌트 (에디터에서 크기 조절 가능)
	UPROPERTY(VisibleAnywhere, Category = "Capture")
	class USphereComponent* CaptureCollision;

	// 채색(커스터마이징) 공용 컴포넌트 - Hider 쪽에도 동일하게 부착됨
	UPROPERTY(VisibleAnywhere, Category = "Customization")
	class UFDCustomizationComponent* CustomizationComp;

	// 밸런스 수치 데이터 테이블 (에디터에서 FMatchBalanceSettings 에셋 할당)
	UPROPERTY(EditDefaultsOnly, Category = "Balance")
	class UDataTable* BalanceDataTable;

	// 현재 포획 판정 중인 숨는 자
	UPROPERTY()
	ACharacter* CapturedHider;

	// 포획 판정 대기 타이머 (3초 후 자동 아웃) -> 15초 변경
	FTimerHandle CaptureJudgeTimerHandle;

	// 봐주기 무적 해제 타이머 (7초 후 속도·무적 복구)
	FTimerHandle SpareExpireTimerHandle;

	// 포획 시퀀스 내부 실행 함수 (서버 전용)
	void Internal_StartCaptureSequence(ACharacter* TargetHider);

	// 타이머 만료 시 자동 아웃 처리 (서버 전용)
	void OnCaptureTimerExpired();

	// 봐주기 무적·속도 버프 만료 처리 (서버 전용)
	void OnSpareExpired(ACharacter* TargetHider);
	
	void Internal_ResolveCaptureLocally(bool bWasCaptured); // 공통 마무리 로직
};
