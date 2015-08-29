// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "XmppPrivatePCH.h"
#include "XmppJingle.h"
#include "XmppConnectionJingle.h"
#include "XmppPresenceJingle.h"
#include "XmppMessagesJingle.h"
#include "XmppChatJingle.h"
#include "XmppMultiUserChatJingle.h"

#if WITH_XMPP_JINGLE

/**
 * Thread to create the xmpp pump/connection
 * Spawned during login and destroyed on logout
 */
class FXmppConnectionPumpThread
	: public FRunnable
	, public sigslot::has_slots<>
{
public:

	// FXmppConnectionPumpThread

	FXmppConnectionPumpThread(FXmppConnectionJingle& InConnection)
		: Connection(InConnection)
		, Thread(NULL)
		, XmppPump(NULL)
		, XmppThread(NULL)
		, LoginState(ELoginProgress::NotStarted)
		, ServerPingTask(NULL)
		, ServerPingRetries(0)
	{
		Thread = FRunnableThread::Create(this, TEXT("XmppConnectionThread"));
	}

	~FXmppConnectionPumpThread()
	{
		if (Thread != NULL)
		{
			Thread->Kill(true);
			delete Thread;
		}
	}

	/**
	 * Signal login request
	 */
	void Login()
	{
		check(IsInGameThread());

		LoginRequest.Set(true);
	}

	/**
	 * Signal logout request
	 */
	void Logout()
	{
		check(IsInGameThread());

		LogoutRequest.Set(true);
	}

	buzz::XmppClient* GetClient() const
	{
		return XmppPump != NULL ? XmppPump->client() : NULL;
	}

	buzz::XmppPump* GetPump() const
	{
		return XmppPump;
	}

	// FRunnable

	virtual bool Init() override
	{
		XmppThread = rtc::ThreadManager::Instance()->WrapCurrentThread();
		XmppPump = new buzz::XmppPump();

		if (UE_LOG_ACTIVE(LogXmpp, Verbose))
		{
			GetClient()->SignalLogInput.connect(this, &FXmppConnectionPumpThread::DebugLogInput);
			GetClient()->SignalLogOutput.connect(this, &FXmppConnectionPumpThread::DebugLogOutput);
		}
		// Register for login state change
		GetClient()->SignalStateChange.connect(this, &FXmppConnectionPumpThread::OnSignalStateChange);

		return true;
	}

	virtual uint32 Run() override
	{
		while (!ExitRequest.GetValue())
		{
			// initial startup and login requests
			if (LoginState == ELoginProgress::NotStarted ||
				LoginRequest.GetValue())
			{
				Connection.HandleLoginChange(LoginState, ELoginProgress::ProcessingLogin);
				LoginState = ELoginProgress::ProcessingLogin;
				if (XmppThread->IsQuitting())
				{
					XmppThread->Restart();
				}
				// kick off login task
				XmppPump->DoLogin(
					Connection.ClientSettings, 
					new buzz::XmppSocket(Connection.ClientSettings.use_tls()), 
					NULL);

				Connection.HandlePumpStarting(XmppPump);
			}
			// logout requests
			else if (LoginState == ELoginProgress::LoggedIn &&
					 LogoutRequest.GetValue())
			{
				Connection.HandleLoginChange(LoginState, ELoginProgress::ProcessingLogout);
				LoginState = ELoginProgress::ProcessingLogout;

				Connection.HandlePumpQuitting(XmppPump);
				
				XmppThread->Quit();
				XmppPump->DoDisconnect();
			}
			
			LoginRequest.Reset();
			LogoutRequest.Reset();

			if (!XmppThread->IsQuitting())
			{
				// tick connection on this thread
				Connection.HandlePumpTick(XmppPump);
				// allow xmpp pump to process
				XmppThread->ProcessMessages(100);
			}
		}		
		return 0;
	}

	virtual void Stop() override
	{
		ExitRequest.Set(true);
	}

	virtual void Exit() override
	{
		Connection.HandlePumpQuitting(XmppPump);

		delete XmppPump;
		XmppPump = NULL;
	}

private:

	void StartServerPing()
	{
		if (ServerPingTask == NULL)
		{
			ServerPingTask = new buzz::PingTask(
				GetClient(),
				rtc::Thread::Current(),
				Connection.GetServer().PingInterval * 1000,
				Connection.GetServer().PingTimeout * 1000
				);
		}
		ServerPingTask->SignalTimeout.connect(this, &FXmppConnectionPumpThread::OnServerPingTimeout);
		ServerPingTask->Start();
	}

	void StopServerPing()
	{
		if (ServerPingTask != NULL)
		{
			ServerPingTask->Abort(true);
			ServerPingTask = NULL;
		}
	}

	void OnServerPingTimeout()
	{
		// task is auto deleted on timeout
		ServerPingTask = NULL;
		// keep track of retries
		ServerPingRetries++;
		// restart task for a retry
		if (ServerPingRetries <= Connection.GetServer().MaxPingRetries)
		{
			StartServerPing();
		}
		// done with ping retries, logout of xmpp
		else
		{
			LogoutRequest.Set(true);
		}
	}

	// callbacks
	void OnSignalStateChange(buzz::XmppEngine::State State)
	{
		switch (State)
		{
		case buzz::XmppEngine::STATE_START:
			UE_LOG(LogXmpp, Verbose, TEXT("STATE_START"));
			break;
		case buzz::XmppEngine::STATE_OPENING:
			UE_LOG(LogXmpp, Verbose, TEXT("STATE_OPENING"));
			break;
		case buzz::XmppEngine::STATE_OPEN:
		{
			UE_LOG(LogXmpp, Verbose, TEXT("STATE_OPEN"));

			Connection.HandleLoginChange(LoginState, ELoginProgress::LoggedIn);
			LoginState = ELoginProgress::LoggedIn;

			StartServerPing();
		}
			break;
		case buzz::XmppEngine::STATE_CLOSED:
		{
			UE_LOG(LogXmpp, Verbose, TEXT("STATE_CLOSED"));

			if (LoginState != ELoginProgress::LoggedIn)
			{
				LogError(TEXT("log-in"));
			}

			StopServerPing();
			Connection.HandlePumpQuitting(XmppPump);
			XmppThread->Quit();

			Connection.HandleLoginChange(LoginState, ELoginProgress::LoggedOut);
			LoginState = ELoginProgress::LoggedOut;
		}
			break;
		}
	}

	void DebugLogInput(const char* Data, int Len)
	{
		FString LogEntry((int32)Len, StringCast<TCHAR>(Data, Len).Get());
		UE_LOG(LogXmpp, VeryVerbose, TEXT("recv: \n%s"), *LogEntry);
	}

	void DebugLogOutput(const char* Data, int Len)
	{
		FString LogEntry((int32)Len, StringCast<TCHAR>(Data, Len).Get());
		UE_LOG(LogXmpp, VeryVerbose, TEXT("send: \n%s"), *LogEntry);
	}

	/**
	 * Get the last error from the XMPP client and log as warning if not ERROR_NONE
	 * @param Context Prefix to add to warning to identify operation attempted
	 */
	void LogError(const FString& Context)
	{
		// see WebRTC/sdk/include/talk/xmpp/xmppengine.h for error codes
		int subCode;
		auto errorCode = GetClient()->GetError(&subCode);

		switch (errorCode)
		{
		default:
			UE_LOG(LogXmpp, Warning, TEXT("XMPP %s error: %d (%d)"), *Context, (int32)errorCode, subCode);
			return;

		case buzz::XmppEngine::ERROR_NONE:
			return;

		case buzz::XmppEngine::ERROR_AUTH:
		case buzz::XmppEngine::ERROR_UNAUTHORIZED:
			UE_LOG(LogXmpp, Warning, TEXT("XMPP %s credentials not valid (%d)"), *Context, (int32)errorCode);
			return;

		case buzz::XmppEngine::ERROR_SOCKET:
			UE_LOG(LogXmpp, Warning, TEXT("XMPP %s socket error"), *Context);
			return;

		case buzz::XmppEngine::ERROR_NETWORK_TIMEOUT:
			UE_LOG(LogXmpp, Warning, TEXT("XMPP %s timed out"), *Context);
			return;
		}
	}

	/** Reference to the connection that spawned this thread */
	FXmppConnectionJingle& Connection;
	/** Thread to run when constructed */
	FRunnableThread* Thread;
	
	/** signal request for login */
	FThreadSafeCounter LoginRequest;
	/** signal request for logout */
	FThreadSafeCounter LogoutRequest;
	/** signal request to stop and exit thread */
	FThreadSafeCounter ExitRequest;

	/** creates the xmpp client connection and processes messages on it */
	buzz::XmppPump* XmppPump;
	/** thread used by xmpp. set to this current thread */
	rtc::Thread* XmppThread;

	/** steps during login/logout */
	ELoginProgress::Type LoginState;

	/** Used for pinging the server periodically to maintain connection */
	class buzz::PingTask* ServerPingTask;
	/** Number of times ping task has been restarted before logging out */
	int32 ServerPingRetries;
};

FXmppConnectionJingle::FXmppConnectionJingle()
	: LastLoginState(ELoginProgress::NotStarted)
	, LoginState(ELoginProgress::NotStarted)
	, PresenceJingle(NULL)
	, PumpThread(NULL)
{
	PresenceJingle = MakeShareable(new FXmppPresenceJingle(*this));
	MessagesJingle = MakeShareable(new FXmppMessagesJingle(*this));
	ChatJingle = MakeShareable(new FXmppChatJingle(*this));
	MultiUserChatJingle = MakeShareable(new FXmppMultiUserChatJingle(*this));
}

FXmppConnectionJingle::~FXmppConnectionJingle()
{
	Shutdown();
}

const FString& FXmppConnectionJingle::GetMucDomain() const
{
	if (!MucDomain.IsEmpty())
	{
		return MucDomain;
	}
	else
	{
		return UserJid.Domain;
	}
}

const FString& FXmppConnectionJingle::GetPubSubDomain() const
{
	if (!PubSubDomain.IsEmpty())
	{
		return PubSubDomain;
	}
	else
	{
		return UserJid.Domain;
	}
}

TSharedPtr<class IXmppPresence> FXmppConnectionJingle::Presence()
{
	return PresenceJingle;
}

TSharedPtr<class IXmppPubSub> FXmppConnectionJingle::PubSub()
{
	//@todo samz - not implemented
	return NULL;
}

TSharedPtr<class IXmppMessages> FXmppConnectionJingle::Messages()
{
	return MessagesJingle;
}

TSharedPtr<class IXmppMultiUserChat> FXmppConnectionJingle::MultiUserChat()
{
	return MultiUserChatJingle;
}

TSharedPtr<class IXmppChat> FXmppConnectionJingle::PrivateChat()
{
	return ChatJingle;
}

bool FXmppConnectionJingle::Tick(float DeltaTime)
{
	FScopeLock Lock(&LoginStateLock);

	if (LastLoginState != LoginState)
	{
		if (LoginState == ELoginProgress::LoggedIn)
		{
			UE_LOG(LogXmpp, Log, TEXT("Logged IN JID=%s"), *GetUserJid().GetFullPath());
			if (LastLoginState == ELoginProgress::ProcessingLogin)
			{
				OnLoginComplete().Broadcast(GetUserJid(), true, FString());

 				// send/obtain initial presence
				if (Presence().IsValid())
				{
					FXmppUserPresence InitialPresence = Presence()->GetPresence();
					InitialPresence.bIsAvailable = true;
					InitialPresence.Status = EXmppPresenceStatus::Offline;
					Presence()->UpdatePresence(InitialPresence);
				}
			}
			OnLoginChanged().Broadcast(GetUserJid(), EXmppLoginStatus::LoggedIn);
		}
		else if (LoginState == ELoginProgress::LoggedOut)
		{
			UE_LOG(LogXmpp, Log, TEXT("Logged OUT JID=%s"), *GetUserJid().GetFullPath());
			if (LastLoginState == ELoginProgress::ProcessingLogin)
			{
				OnLoginComplete().Broadcast(GetUserJid(), false, FString());
			}
			else if (LastLoginState == ELoginProgress::ProcessingLogout)
			{
				OnLogoutComplete().Broadcast(GetUserJid(), true, FString());
			}
			if (LastLoginState == ELoginProgress::LoggedIn ||
				LastLoginState == ELoginProgress::ProcessingLogout)
			{
				OnLoginChanged().Broadcast(GetUserJid(), EXmppLoginStatus::LoggedOut);
			}
		}
		LastLoginState = LoginState;
	}
	return true;
}

void FXmppConnectionJingle::Startup()
{
	UE_LOG(LogXmpp, Log, TEXT("Startup connection"));

	LastLoginState = ELoginProgress::NotStarted;
	LoginState = ELoginProgress::NotStarted;

	check(PumpThread == NULL);
	PumpThread = new FXmppConnectionPumpThread(*this);
}

void FXmppConnectionJingle::Shutdown()
{
	UE_LOG(LogXmpp, Log, TEXT("Shutdown connection"));

	LoginState = ELoginProgress::LoggedOut;

	delete PumpThread;
	PumpThread = NULL;
}

void FXmppConnectionJingle::HandleLoginChange(ELoginProgress::Type InLastLoginState, ELoginProgress::Type InLoginState)
{
	FScopeLock Lock(&LoginStateLock);

	LoginState = InLoginState;
	LastLoginState = InLastLoginState;

	UE_LOG(LogXmpp, Log, TEXT("Login Changed from %s to %s"), 
		ELoginProgress::ToString(LastLoginState), ELoginProgress::ToString(LoginState));
}

void FXmppConnectionJingle::HandlePumpStarting(buzz::XmppPump* XmppPump)
{
	if (PresenceJingle.IsValid())
	{
		PresenceJingle->HandlePumpStarting(XmppPump);
	}
	if (MessagesJingle.IsValid())
	{
		MessagesJingle->HandlePumpStarting(XmppPump);
	}
	if (ChatJingle.IsValid())
	{
		ChatJingle->HandlePumpStarting(XmppPump);
	}
	if (MultiUserChatJingle.IsValid())
	{
		MultiUserChatJingle->HandlePumpStarting(XmppPump);
	}
}

void FXmppConnectionJingle::HandlePumpQuitting(buzz::XmppPump* XmppPump)
{
	if (PresenceJingle.IsValid())
	{
		PresenceJingle->HandlePumpQuitting(XmppPump);
	}
	if (MessagesJingle.IsValid())
	{
		MessagesJingle->HandlePumpQuitting(XmppPump);
	}
	if (ChatJingle.IsValid())
	{
		ChatJingle->HandlePumpQuitting(XmppPump);
	}
	if (MultiUserChatJingle.IsValid())
	{
		MultiUserChatJingle->HandlePumpQuitting(XmppPump);
	}
}

void FXmppConnectionJingle::HandlePumpTick(buzz::XmppPump* XmppPump)
{
	if (PresenceJingle.IsValid())
	{
		PresenceJingle->HandlePumpTick(XmppPump);
	}
	if (MessagesJingle.IsValid())
	{
		MessagesJingle->HandlePumpTick(XmppPump);
	}
	if (ChatJingle.IsValid())
	{
		ChatJingle->HandlePumpTick(XmppPump);
	}
	if (MultiUserChatJingle.IsValid())
	{
		MultiUserChatJingle->HandlePumpTick(XmppPump);
	}
}

void FXmppConnectionJingle::SetServer(const FXmppServer& InServer)
{
	// in order to ensure unique connections from user/client combination
	// add random number to the client resource identifier
	ServerConfig = InServer;
	ServerConfig.ClientResource += FString(TEXT("-")) + FGuid::NewGuid().ToString(EGuidFormats::Digits);
}

const FXmppServer& FXmppConnectionJingle::GetServer() const
{
	return ServerConfig;
}

void FXmppConnectionJingle::Login(const FString& UserId, const FString& Password)
{
	FString ErrorStr;

	// Configure the server connection
	buzz::XmppClientSettings Settings;
	Settings.set_host(TCHAR_TO_UTF8(*ServerConfig.Domain));
	Settings.set_use_tls(ServerConfig.bUseSSL ? buzz::TLS_ENABLED : buzz::TLS_DISABLED);
	Settings.set_resource(TCHAR_TO_UTF8(*ServerConfig.ClientResource));
	Settings.set_server(rtc::SocketAddress(TCHAR_TO_UTF8(*ServerConfig.ServerAddr), ServerConfig.ServerPort));

	// Cache user Jid
	UserJid.Id = UserId;
	UserJid.Domain = ServerConfig.Domain;
	UserJid.Resource = ServerConfig.ClientResource;
	//@todo samz - user service discovery to find these domains
	MucDomain = TEXT("muc.") + ServerConfig.Domain;
	PubSubDomain = TEXT("pubsub.") + ServerConfig.Domain;

	if (!UserJid.IsValid())
	{
		ErrorStr = FString::Printf(TEXT("Invalid Jid %s"), *UserJid.GetFullPath());
	}
	else
	{
		// Set user id/pass
		rtc::InsecureCryptStringImpl Auth;
		Auth.password() = TCHAR_TO_UTF8(*Password);
		Settings.set_user(TCHAR_TO_UTF8(*UserJid.Id));
		Settings.set_pass(rtc::CryptString(Auth));

		// Cache client connection settings
		ClientSettings = Settings;

		UE_LOG(LogXmpp, Log, TEXT("Starting Login on connection"));
		UE_LOG(LogXmpp, Log, TEXT("  server = %s:%d"), *ServerConfig.ServerAddr, ServerConfig.ServerPort);
		UE_LOG(LogXmpp, Log, TEXT("  user = %s"), *UserJid.GetFullPath());

		// socket/thread for pumping connection	
		if (LoginState == ELoginProgress::ProcessingLogin)
		{
			ErrorStr = TEXT("Still processing last login");
		}
		else if (LoginState == ELoginProgress::ProcessingLogout)
		{
			ErrorStr = TEXT("Still processing last logout");
		}
		else if (LoginState == ELoginProgress::LoggedIn)
		{
			ErrorStr = TEXT("Already logged in");
		}
		else
		{
			if (PumpThread != NULL)
			{
#if 0
				PumpThread->Login();
#else
				//@todo sz1 - should be able to reuse existing connection pump for login/logout
				Shutdown();
				Startup();
#endif
			}
			else
			{
				Startup();				
			}
		}
	}

	if (!ErrorStr.IsEmpty())
	{
		UE_LOG(LogXmpp, Warning, TEXT("Login failed. %s"), *ErrorStr);
		OnLoginComplete().Broadcast(GetUserJid(), false, ErrorStr);
	}
}

void FXmppConnectionJingle::Logout()
{	
	FString ErrorStr;

	if (GetLoginStatus() != EXmppLoginStatus::LoggedIn)
	{
		ErrorStr = TEXT("not logged in");
	}
	if (PumpThread != NULL)
	{
		//@todo sz1 - should be able to reuse existing connection pump for login/logout
#if 0
		PumpThread->Logout();
#else
		Shutdown();
		OnLogoutComplete().Broadcast(GetUserJid(), true, ErrorStr);
#endif
	}
	else
	{
		ErrorStr = TEXT("not xmpp thread");		
	}

	if (!ErrorStr.IsEmpty())
	{
		UE_LOG(LogXmpp, Warning, TEXT("Logout failed. %s"), *ErrorStr);
		OnLogoutComplete().Broadcast(GetUserJid(), false, ErrorStr);
	}
}

EXmppLoginStatus::Type FXmppConnectionJingle::GetLoginStatus() const
{
	FScopeLock Lock(&LoginStateLock);

	if (LoginState == ELoginProgress::LoggedIn)
	{
		return EXmppLoginStatus::LoggedIn;
	}
	else
	{
		return EXmppLoginStatus::LoggedOut;
	}
}

const FXmppUserJid& FXmppConnectionJingle::GetUserJid() const
{
	return UserJid;
}

#endif //WITH_XMPP_JINGLE
