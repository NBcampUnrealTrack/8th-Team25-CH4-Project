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

	// GameMode/GameState 수정 없이, 정찰 단계 진입·종료를 스스로 감지해서 SetScoutingMode를 호출하기 위함
	virtual void Tick(float DeltaSeconds) override;

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

	// 정찰 모드
	// true: 정찰 단계 - 걷기 속도를 올려서 맵을 둘러볼 수 있게 함 / false: 본게임 - 기본 속도로 복귀
	// (기존 플라이 관전 방식은 삭제됨 - 이제 하이더와 동일하게 걸어서 정찰함)
	// GameMode를 건드리지 않기 위해 외부에서 호출받는 대신 Tick에서 GameState 페이즈를 직접 감지해서 스스로 호출함
	UFUNCTION(BlueprintCallable)
	void SetScoutingMode(bool bEnable);
	
	// 투사체에 맞았을 때 스턴 적용 (서버 전용 - AFDThrowItem에서 호출)
	// 이미 스턴 중이면 무시 (연장 없음 - 술래를 계속 묶어둘 수 없게)
	void ApplyStun(float Duration);

	// 현재 스턴 상태인지 조회 (bIsStunned가 복제되므로 클라/서버 모두 사용 가능)
	bool IsStunned() const { return bIsStunned; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
private:
	// 정찰 속도 버프가 현재 적용되어 있는지 (Tick에서 Phase 변화 감지용, 중복 호출 방지)
	bool bScoutModeApplied = false;

	// 포획 판정용 콜리전 컴포넌트 (에디터에서 크기 조절 가능)
	UPROPERTY(VisibleAnywhere, Category = "Capture")
	class USphereComponent* CaptureCollision;

	// 술래는 1인칭 고정 - 캡슐에 눈높이로 바로 부착
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	class UCameraComponent* FirstPersonCamera;

	// 채색(커스터마이징) 공용 컴포넌트 - Hider 쪽에도 동일하게 부착됨
	UPROPERTY(VisibleAnywhere, Category = "Customization")
	class UFDCustomizationComponent* CustomizationComp;

	// 이모트(감정표현) 공용 컴포넌트 - Hider 쪽에도 동일하게 부착됨
	UPROPERTY(VisibleAnywhere, Category = "Emote")
	class UFDEmoteComponent* EmoteComp;

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
	
	// 스턴 상태 - 서버에서 변경 시 클라이언트에 자동 전파
	UPROPERTY(ReplicatedUsing = OnRep_bIsStunned)
	bool bIsStunned = false;

	UFUNCTION()
	void OnRep_bIsStunned();

	// 스턴 해제 타이머
	FTimerHandle StunExpireTimerHandle;

	// 스턴 해제 처리 (서버 전용)
	void OnStunExpired();
};
