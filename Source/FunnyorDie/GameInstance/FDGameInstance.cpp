// FDGameInstance.cpp

#include "GameInstance/FDGameInstance.h"
#include "OnlineSubsystem.h"                        
#include "OnlineSessionSettings.h"       
#include "Online/OnlineSessionNames.h"
#include "Interfaces/OnlineIdentityInterface.h"     
#include "OnlineSubsystemUtils.h"                   
#include "GameFramework/PlayerController.h"

void UFDGameInstance::Init()
{
	Super::Init();

	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	if (!OSS)
	{
		UE_LOG(LogTemp, Error, TEXT("[EOS] OnlineSubsystem을 찾을 수 없음. DefaultEngine.ini 확인 필요"));
		SetStatus(EFDSessionStatus::LoginFailed);
		return;
	}

	// 세션 인터페이스를 미리 잡아둠
	SessionInterface = OSS->GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[EOS] SessionInterface 획득 실패"));
		SetStatus(EFDSessionStatus::LoginFailed);
		return;
	}

	// IOnlineIdentity 로그인/신원 담당 인터페이스
	IOnlineIdentityPtr Identity = OSS->GetIdentityInterface();
	if (!Identity.IsValid())
	{
		SetStatus(EFDSessionStatus::LoginFailed);
		return;
	}

	// 로그인 완료 콜백 등록 후 AutoLogin 호출
	Identity->AddOnLoginCompleteDelegate_Handle(
		0, FOnLoginCompleteDelegate::CreateUObject(this, &UFDGameInstance::OnLoginComplete));

	SetStatus(EFDSessionStatus::LoggingIn);
	Identity->AutoLogin(0);

	UE_LOG(LogTemp, Log, TEXT("[EOS] AutoLogin 요청 (서브시스템: %s)"), *OSS->GetSubsystemName().ToString());
}

void UFDGameInstance::OnLoginComplete(int32 LocalUserNum, bool bWasSuccessful,
                                      const FUniqueNetId& UserId, const FString& Error)
{
	if (bWasSuccessful)
	{
		UE_LOG(LogTemp, Log, TEXT("[EOS] 로그인 성공: %s"), *UserId.ToString());
		SetStatus(EFDSessionStatus::Ready);  
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[EOS] 로그인 실패: %s"), *Error);
		SetStatus(EFDSessionStatus::LoginFailed);
	}
}

// 방 만들기
void UFDGameInstance::HostSession()
{
	if (!SessionInterface.IsValid()) return;

	if (SessionInterface->GetNamedSession(NAME_GameSession) != nullptr)
	{
		bWantsToHostAfterDestroy = true;
		SessionInterface->OnDestroySessionCompleteDelegates.AddUObject(
			this, &UFDGameInstance::OnDestroySessionComplete);
		SessionInterface->DestroySession(NAME_GameSession);
		return;
	}

	CreateSessionInternal();
}

void UFDGameInstance::CreateSessionInternal()
{
	SetStatus(EFDSessionStatus::Hosting);

	FOnlineSessionSettings Settings;
	Settings.bIsLANMatch            = false;   // false = EOS 릴레이 경유
	Settings.NumPublicConnections   = MaxPlayers;
	Settings.bShouldAdvertise       = true;    // 검색 목록에 노출
	Settings.bAllowJoinInProgress   = true;    // 게임 도중 난입 허용 (로비 단계 재접속에도 필요)
	Settings.bUsesPresence          = true;
	Settings.bUseLobbiesIfAvailable = true;    // EOS 로비 기능으로 세션 생성
	Settings.bAllowJoinViaPresence  = true;

	// bUsesPresence / bUseLobbiesIfAvailable 은 호스트와 클라이언트가 반드시 일치해야 함

	// 검색 시 필터로 쓸 키워드
	Settings.Set(SEARCH_KEYWORDS, FString("FunnyorDie"),
	             EOnlineDataAdvertisementType::ViaOnlineService);

	SessionInterface->OnCreateSessionCompleteDelegates.AddUObject(
		this, &UFDGameInstance::OnCreateSessionComplete);

	// NAME_GameSession — 엔진이 정해둔 기본 세션 이름 상수
	// 호스트/클라가 같은 이름을 써야 GetResolvedConnectString이 세션을 찾을 수 있음
	SessionInterface->CreateSession(0, NAME_GameSession, Settings);
}

void UFDGameInstance::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	// 콜백은 한 번 쓰고 정리 안 하면 다음 호출 때 중복 실행됨
	SessionInterface->ClearOnCreateSessionCompleteDelegates(this);

	if (!bWasSuccessful)
	{
		UE_LOG(LogTemp, Error, TEXT("[EOS] 세션 생성 실패"));
		SetStatus(EFDSessionStatus::Failed);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[EOS] 세션 생성 성공 → 리슨 서버로 LobbyMaps 오픈"));
	
	GetWorld()->ServerTravel(LobbyLevelPath + TEXT("?listen"));
}

// 클라이언트: 방 검색 → 참가
void UFDGameInstance::FindAndJoinSession()
{
	if (!SessionInterface.IsValid()) return;

	SetStatus(EFDSessionStatus::Searching);

	// 검색 조건 그릇 생성 콜백까지 살아있어야 해서 멤버 변수에 보관
	SearchSettings = MakeShared<FOnlineSessionSearch>();
	SearchSettings->MaxSearchResults = 20;
	SearchSettings->bIsLanQuery = false;

	// SEARCH_LOBBIES — 로비 방식으로 만들어진 세션 찾기
	// 호스트의 bUseLobbiesIfAvailable=true 와 짝을 이룸
	SearchSettings->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);

	SessionInterface->OnFindSessionsCompleteDelegates.AddUObject(
		this, &UFDGameInstance::OnFindSessionsComplete);

	SessionInterface->FindSessions(0, SearchSettings.ToSharedRef());
}

void UFDGameInstance::OnFindSessionsComplete(bool bWasSuccessful)
{
	SessionInterface->ClearOnFindSessionsCompleteDelegates(this);

	if (!bWasSuccessful || !SearchSettings.IsValid() || SearchSettings->SearchResults.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EOS] 찾은 방 없음"));
		SetStatus(EFDSessionStatus::NoSessionFound);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[EOS] 방 %d개 발견 → 첫 번째 방 참가 시도"),
	       SearchSettings->SearchResults.Num());

	SetStatus(EFDSessionStatus::Joining);

	SessionInterface->OnJoinSessionCompleteDelegates.AddUObject(
		this, &UFDGameInstance::OnJoinSessionComplete);

	// 지금은 첫 번째 결과에 무조건 참가
	SessionInterface->JoinSession(0, NAME_GameSession, SearchSettings->SearchResults[0]);
}

void UFDGameInstance::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	SessionInterface->ClearOnJoinSessionCompleteDelegates(this);

	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		UE_LOG(LogTemp, Error, TEXT("[EOS] 세션 참가 실패 (코드 %d)"), (int32)Result);
		SetStatus(EFDSessionStatus::Failed);
		return;
	}

	FString ConnectString;
	if (!SessionInterface->GetResolvedConnectString(NAME_GameSession, ConnectString))
	{
		UE_LOG(LogTemp, Error, TEXT("[EOS] 접속 문자열 획득 실패"));
		SetStatus(EFDSessionStatus::Failed);
		return;
	}

	APlayerController* PC = GetFirstLocalPlayerController();
	if (!PC) return;

	UE_LOG(LogTemp, Log, TEXT("[EOS] 접속 시도: %s"), *ConnectString);

	// ClientTravel — 클라이언트가 특정 서버 주소로 건너감
	PC->ClientTravel(ConnectString, TRAVEL_Absolute);
}

void UFDGameInstance::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	SessionInterface->ClearOnDestroySessionCompleteDelegates(this);

	if (bWantsToHostAfterDestroy && bWasSuccessful)
	{
		bWantsToHostAfterDestroy = false;
		CreateSessionInternal();   // 정리 끝났으니 이제 진짜로 방 만들기
	}
}

void UFDGameInstance::SetStatus(EFDSessionStatus NewStatus)
{
	CurrentStatus = NewStatus;
	OnSessionStatusChanged.Broadcast(NewStatus);   // 구독 중인 위젯들에게 방송
}