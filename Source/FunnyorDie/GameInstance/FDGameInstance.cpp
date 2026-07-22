// FDGameInstance.cpp

#include "GameInstance/FDGameInstance.h"
#include "OnlineSubsystem.h"                        
#include "OnlineSessionSettings.h"       
#include "Online/OnlineSessionNames.h"
#include "Interfaces/OnlineIdentityInterface.h"     
#include "OnlineSubsystemUtils.h"                   
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"

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

	// 로그인 완료 콜백 등록
	Identity->AddOnLoginCompleteDelegate_Handle(
		0, FOnLoginCompleteDelegate::CreateUObject(this, &UFDGameInstance::OnLoginComplete));

	SetStatus(EFDSessionStatus::LoggingIn);

	// AutoLogin()은 에디터에서만 Account Portal을 자동으로 처리해주고,
	// 패키지 빌드(Standalone)에서는 커맨드라인 인자나 ini 계정 정보가 없으면 그냥 실패함.
	// 그래서 패키지에서도 동작하도록 Login()으로 Account Portal 로그인을 명시적으로 요청.
	// (Epic Games Launcher로 배포할 계획이 생기면, 그때는 커맨드라인의 -AUTH_TYPE=exchangecode
	//  값을 파싱해서 Credentials.Type에 넣는 방식으로 바꿔야 함)
	FOnlineAccountCredentials Credentials;
	Credentials.Type  = TEXT("accountportal");
	Credentials.Id    = TEXT("");
	Credentials.Token = TEXT("");

	Identity->Login(0, Credentials);

	UE_LOG(LogTemp, Log, TEXT("[EOS] Login(accountportal) 요청 (서브시스템: %s)"), *OSS->GetSubsystemName().ToString());
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
	if (!SessionInterface.IsValid())
	{ return; }

	UE_LOG(LogTemp, Warning, TEXT("[EOS] 방 검색 시작"));

	SetStatus(EFDSessionStatus::Searching);

	SearchSettings = MakeShared<FOnlineSessionSearch>();
	SearchSettings->MaxSearchResults = 20;
	SearchSettings->bIsLanQuery = false;
	
	SearchSettings->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
	
	SearchSettings->QuerySettings.Set(SEARCH_KEYWORDS, FString("FunnyorDie"),
									  EOnlineComparisonOp::Equals);

	SessionInterface->OnFindSessionsCompleteDelegates.AddUObject(
		this, &UFDGameInstance::OnFindSessionsComplete);

	const bool bStarted = SessionInterface->FindSessions(0, SearchSettings.ToSharedRef());
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

void UFDGameInstance::PlayMusic(USoundBase* NewMusic)
{
	if (!NewMusic) return;

	// 이미 같은 곡이 재생 중이면 아무것도 안 함
	// (Start -> Lobby 넘어갈 때 둘 다 MenuMusic이라 여기서 걸러져서 끊기지 않고 이어짐)
	if (NewMusic == CurrentMusic && MusicAudioComponent && MusicAudioComponent->IsPlaying())
	{
		return;
	}

	// 재생 중이던 이전 곡은 페이드아웃 후 정지
	if (MusicAudioComponent)
	{
		MusicAudioComponent->FadeOut(1.0f, 0.f);
	}

	CurrentMusic = NewMusic;

	// SpawnSound2D: 월드 위치와 무관하게 항상 같은 볼륨으로 들리는 브금용 사운드 생성
	// bPersistAcrossLevelTransition = true가 핵심 - 레벨 이동(ServerTravel) 중에도 안 끊기게 해줌
	MusicAudioComponent = UGameplayStatics::SpawnSound2D(
		this, NewMusic, /*VolumeMultiplier=*/1.f, /*PitchMultiplier=*/1.f,
		/*StartTime=*/0.f, /*ConcurrencySettings=*/nullptr,
		/*bPersistAcrossLevelTransition=*/true, /*bAutoDestroy=*/true);

	if (MusicAudioComponent)
	{
		MusicAudioComponent->FadeIn(1.0f);
	}

	UE_LOG(LogTemp, Log, TEXT("[GameInstance] 브금 전환: %s"), *NewMusic->GetName());
}

void UFDGameInstance::StopMusic()
{
	// 이미 정지 상태(재생 중인 게 없음)면 아무것도 안 함
	if (!CurrentMusic && !MusicAudioComponent) return;

	if (MusicAudioComponent)
	{
		MusicAudioComponent->FadeOut(1.0f, 0.f);
		MusicAudioComponent = nullptr;
	}

	CurrentMusic = nullptr;

	UE_LOG(LogTemp, Log, TEXT("[GameInstance] 브금 정지"));
}
