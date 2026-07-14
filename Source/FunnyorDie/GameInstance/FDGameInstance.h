// FDGameInstance.h

#pragma once
#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "FDGameInstance.generated.h"

// 세션 진행 상태
UENUM(BlueprintType)
enum class EFDSessionStatus : uint8
{
	Idle,           // 아무것도 안 함
	LoggingIn,      // EOS 로그인 중
	LoginFailed,    // 로그인 실패
	Ready,          // 로그인 완료 방 만들기/참여하기 가능
	Hosting,        // 방 만드는 중
	Searching,      // 방 찾는 중
	Joining,        // 방 참가 중
	NoSessionFound, // 검색했는데 방이 없음
	Failed          // 생성/참가 실패
};

// GameInstance -> UI 로 상태를 방송하는 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionStatusChanged, EFDSessionStatus, NewStatus);

UCLASS()
class FUNNYORDIE_API UFDGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	// 레벨보다도 먼저 뜨기 때문에 EOS 로그인을 시작하기에 가장 이른 지점
	virtual void Init() override;

	// StartWidget의 방 만들기 버튼이 부를 함수
	UFUNCTION(BlueprintCallable, Category = "FD|Session")
	void HostSession();

	// StartWidget의 참여하기 버튼이 부를 함수
	UFUNCTION(BlueprintCallable, Category = "FD|Session")
	void FindAndJoinSession();

	// 위젯이 구독할 상태 방송 채널
	UPROPERTY(BlueprintAssignable, Category = "FD|Session")
	FOnSessionStatusChanged OnSessionStatusChanged;

	UFUNCTION(BlueprintPure, Category = "FD|Session")
	EFDSessionStatus GetSessionStatus() const { return CurrentStatus; }

protected:
	// 호스트가 CreateSession 성공 후 ServerTravel할 맵
	UPROPERTY(EditDefaultsOnly, Category = "FD|Session")
	FString LobbyLevelPath = TEXT("/Game/FunnyorDie/LevelDesign/LobbyMaps");

	UPROPERTY(EditDefaultsOnly, Category = "FD|Session")
	int32 MaxPlayers = 8;

private:
	// IOnlineSessionPtr: 방 생성/검색/참가를 담당하는 인터페이스의 공유 포인터.
	IOnlineSessionPtr SessionInterface;

	// FOnlineSessionSearch: 검색 조건과 검색 결과가 같이 담기는 그릇.
	// FindSessions에 넘겼다가, 콜백에서 SearchResults를 꺼내 읽음
	// 콜백 시점까지 살아있어야 해서 멤버로 보관 (지역 변수로 두면 날아감)
	TSharedPtr<FOnlineSessionSearch> SearchSettings;

	EFDSessionStatus CurrentStatus = EFDSessionStatus::Idle;

	// 상태 바꾸고 방송까지 한 번에
	void SetStatus(EFDSessionStatus NewStatus);

	// EOS가 비동기 작업을 끝내면 불러주는 콜백들
	void OnLoginComplete(int32 LocalUserNum, bool bWasSuccessful,
	                     const FUniqueNetId& UserId, const FString& Error);
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);

	// DestroySession이 끝난 뒤 뭘 할지 기억해두는 플래그
	bool bWantsToHostAfterDestroy = false;

	// 실제 CreateSession 호출부
	void CreateSessionInternal();
};