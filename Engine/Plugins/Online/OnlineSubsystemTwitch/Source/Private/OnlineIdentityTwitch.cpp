// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemTwitchPrivate.h"
#include "OnlineIdentityTwitch.h"
#include "OnlineAccountTwitch.h"
#include "OnlineExternalUIInterface.h"
#include "Http.h"
#include "Misc/ConfigCacheIni.h"

TSharedPtr<FUserOnlineAccountTwitch> FOnlineIdentityTwitch::GetUserAccountTwitch(const FUniqueNetId& UserId) const
{
	TSharedPtr<FUserOnlineAccountTwitch> Result;

	const TSharedRef<FUserOnlineAccountTwitch>* FoundUserAccount = UserAccounts.Find(UserId.ToString());
	if (FoundUserAccount != nullptr)
	{
		Result = *FoundUserAccount;
	}

	return Result;
}

TSharedPtr<FUserOnlineAccount> FOnlineIdentityTwitch::GetUserAccount(const FUniqueNetId& UserId) const
{
	return GetUserAccountTwitch(UserId);
}

TArray<TSharedPtr<FUserOnlineAccount> > FOnlineIdentityTwitch::GetAllUserAccounts() const
{
	TArray<TSharedPtr<FUserOnlineAccount> > Result;

	for (const TPair<FString, TSharedRef<FUserOnlineAccountTwitch> >& It : UserAccounts)
	{
		Result.Add(It.Value);
	}

	return Result;
}

TSharedPtr<const FUniqueNetId> FOnlineIdentityTwitch::GetUniquePlayerId(int32 LocalUserNum) const
{
	if (LocalUserNum >= 0 && LocalUserNum < MAX_LOCAL_PLAYERS)
	{
		const TSharedPtr<const FUniqueNetId>* FoundId = UserIds.Find(LocalUserNum);
		if (FoundId != nullptr)
		{
			return *FoundId;
		}
	}
	return nullptr;
}

TSharedPtr<const FUniqueNetId> FOnlineIdentityTwitch::CreateUniquePlayerId(uint8* Bytes, int32 Size)
{
	if (Bytes != nullptr && Size > 0)
	{
		FString StrId(Size, (TCHAR*)Bytes);
		return MakeShared<FUniqueNetIdString>(MoveTemp(StrId));
	}
	return nullptr;
}

TSharedPtr<const FUniqueNetId> FOnlineIdentityTwitch::CreateUniquePlayerId(const FString& Str)
{
	return MakeShared<FUniqueNetIdString>(Str);
}

bool FOnlineIdentityTwitch::Login(int32 LocalUserNum, const FOnlineAccountCredentials& AccountCredentials)
{
	FString ErrorStr;

	if (bHasLoginOutstanding)
	{
		ErrorStr = FString::Printf(TEXT("Login already pending for user"));
	}
	else if (!LoginURLDetails.IsValid())
	{
		ErrorStr = FString::Printf(TEXT("OnlineSubsystemTwitch is improperly configured in DefaultEngine.ini LoginURL=%s LoginRedirectUrl=%s ClientId=%s"),
			*LoginURLDetails.LoginUrl, *LoginURLDetails.LoginRedirectUrl, *LoginURLDetails.ClientId);
	}
	else if (LocalUserNum < 0 || LocalUserNum >= MAX_LOCAL_PLAYERS)
	{
		ErrorStr = FString::Printf(TEXT("Invalid LocalUserNum=%d"), LocalUserNum);
	}
	else
	{
		if (!AccountCredentials.Token.IsEmpty() && AccountCredentials.Type == GetAuthType())
		{
			bHasLoginOutstanding = true;

			// Validate the provided auth token and get our current scope permissions
			LoginWithAccessToken(LocalUserNum, AccountCredentials.Token, FOnLoginCompleteDelegate::CreateThreadSafeSP(this, &FOnlineIdentityTwitch::OnAccessTokenLoginComplete));
		}
		else
		{
			IOnlineExternalUIPtr OnlineExternalUI = Subsystem->GetExternalUIInterface();
			if (OnlineExternalUI.IsValid())
			{
				CurrentLoginNonce = LoginURLDetails.GenerateNonce();

				bHasLoginOutstanding = true;

				FOnLoginUIClosedDelegate CompletionDelegate = FOnLoginUIClosedDelegate::CreateThreadSafeSP(this, &FOnlineIdentityTwitch::OnExternalUILoginComplete);
				OnlineExternalUI->ShowLoginUI(LocalUserNum, true, true, CompletionDelegate);
			}
			else
			{
				ErrorStr = FString::Printf(TEXT("External interface missing"));
			}
		}
	}

	if (!ErrorStr.IsEmpty())
	{
		UE_LOG_ONLINE(Error, TEXT("Login for user %d failed: %s"), LocalUserNum, *ErrorStr);

		OnLoginAttemptComplete(LocalUserNum, ErrorStr);
		return false;
	}
	return true;
}

void FOnlineIdentityTwitch::LoginWithAccessToken(int32 LocalUserNum, const FString& AccessToken, const FOnLoginCompleteDelegate& InCompletionDelegate)
{
	// Validate the provided auth token and get our current scope permissions
	const FOnlineAccountCredentials AccountCredentials(GetAuthType(), FString(), AccessToken);
	ValidateAuthToken(LocalUserNum, AccountCredentials, FOnValidateAuthTokenComplete::CreateThreadSafeSP(this, &FOnlineIdentityTwitch::OnValidateAuthTokenComplete, InCompletionDelegate));
}

void FOnlineIdentityTwitch::OnValidateAuthTokenComplete(int32 LocalUserNum, const FOnlineAccountCredentials& AccountCredentials, TSharedPtr<FUserOnlineAccountTwitch> User, const FString& InErrorStr, const FOnLoginCompleteDelegate InCompletionDelegate)
{
	FString ErrorStr = InErrorStr;

	if (ErrorStr.IsEmpty())
	{
		// Confirm the specified user ID matches the auth token
		if (AccountCredentials.Id.IsEmpty() ||
			AccountCredentials.Id == User->GetUserId()->ToString())
		{
			// Confirm we have all of the scope permissions we require
			bool bHasAllPermissions = true;
			FString MissingScopeFields;
			for (const FString& ScopeField : LoginURLDetails.ScopeFields)
			{
				bool bHasPermission = User->GetScopePermissions().Contains(ScopeField);
				if (!bHasPermission)
				{
					bHasAllPermissions = false;
					if (!MissingScopeFields.IsEmpty())
					{
						MissingScopeFields += TEXT(" ");
					}
					MissingScopeFields += ScopeField;
				}
			}
			if (!MissingScopeFields.IsEmpty())
			{
				UE_LOG_ONLINE(Log, TEXT("FOnlineIdentityTwitch::OnValidateAuthTokenComplete: User %d missing scope field(s) [%s]"), LocalUserNum, *MissingScopeFields);
				ErrorStr = LOGIN_ERROR_MISSING_PERMISSIONS;
				ErrorStr += TEXT(" ");
				ErrorStr += MissingScopeFields;
			}
		}
		else
		{
			ErrorStr = LOGIN_ERROR_TOKEN_NOT_FOR_USER;
		}
	}
	
	if (ErrorStr.IsEmpty())
	{
		// update/add cached entry for user
		UserAccounts.Add(User->GetUserId()->ToString(), User.ToSharedRef());
		// keep track of user ids for local users
		UserIds.Add(LocalUserNum, User->GetUserId());
	}
	else
	{
		UE_LOG_ONLINE(Log, TEXT("ValidateAuthToken for user %d failed: %s"), LocalUserNum, *ErrorStr);
	}

	bool bWasSuccessful = ErrorStr.IsEmpty();
	InCompletionDelegate.ExecuteIfBound(LocalUserNum, bWasSuccessful, bWasSuccessful ? *User->GetUserId() : *ZeroId, ErrorStr);
}

void FOnlineIdentityTwitch::ValidateAuthToken(int32 LocalUserNum, const FOnlineAccountCredentials& AccountCredentials, const FOnValidateAuthTokenComplete& InCompletionDelegate)
{
	// kick off http request to validate access token
	TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();

	FString Url(LoginURLDetails.TokenValidateUrl);

	HttpRequest->OnProcessRequestComplete().BindThreadSafeSP(this, &FOnlineIdentityTwitch::ValidateAuthToken_HttpRequestComplete, LocalUserNum, AccountCredentials, InCompletionDelegate);
	HttpRequest->SetURL(MoveTemp(Url));
	HttpRequest->SetHeader(TEXT("Accept"), Subsystem->GetTwitchApiVersion());
	HttpRequest->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("OAuth %s"), *AccountCredentials.Token));
	HttpRequest->SetVerb(TEXT("GET"));
	HttpRequest->ProcessRequest();
}

void FOnlineIdentityTwitch::ValidateAuthToken_HttpRequestComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded, int32 LocalUserNum, const FOnlineAccountCredentials AccountCredentials, const FOnValidateAuthTokenComplete InCompletionDelegate)
{
	TSharedPtr<FUserOnlineAccountTwitch> User;
	FString ResponseStr, ErrorStr;

	if (bSucceeded &&
		HttpResponse.IsValid())
	{
		if (EHttpResponseCodes::IsOk(HttpResponse->GetResponseCode()) &&
			HttpResponse->GetContentType().StartsWith(TEXT("application/json")))
		{
			ResponseStr = HttpResponse->GetContentAsString();
			UE_LOG_ONLINE(Verbose, TEXT("Request Current Permissions request complete for user %d. url=%s code=%d response=%s"),
				LocalUserNum, *HttpRequest->GetURL(), HttpResponse->GetResponseCode(), *ResponseStr);

			User = MakeShared<FUserOnlineAccountTwitch>();
			if (!User->Parse(AccountCredentials.Token, ResponseStr))
			{
				ErrorStr = FString::Printf(TEXT("Error parsing login. payload=%s"),
					*ResponseStr);
			}
		}
		else
		{
			ErrorStr = FString::Printf(TEXT("Invalid response. code=%d contentType=%s error=%s"),
				HttpResponse->GetResponseCode(), *HttpResponse->GetContentType(), *ResponseStr);
		}
	}
	else
	{
		ErrorStr = TEXT("No response");
	}

	InCompletionDelegate.ExecuteIfBound(LocalUserNum, AccountCredentials, User, ErrorStr);
}

void FOnlineIdentityTwitch::OnAccessTokenLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UniqueId, const FString& Error)
{
	bHasLoginOutstanding = false;
	CurrentLoginNonce.Empty();
	OnLoginAttemptComplete(LocalUserNum, Error);
}

void FOnlineIdentityTwitch::OnLoginAttemptComplete(int32 LocalUserNum, const FString& ErrorStr)
{
	const FString ErrorStrCopy(ErrorStr);
	if (GetLoginStatus(LocalUserNum) == ELoginStatus::LoggedIn)
	{
		UE_LOG_ONLINE(Log, TEXT("Twitch login for user %d was successful."), LocalUserNum);
		TSharedPtr<const FUniqueNetId> UserId = GetUniquePlayerId(LocalUserNum);
		check(UserId.IsValid());

		Subsystem->ExecuteNextTick([this, UserId, LocalUserNum, ErrorStrCopy]()
		{
			TriggerOnLoginCompleteDelegates(LocalUserNum, true, *UserId, ErrorStrCopy);
			TriggerOnLoginStatusChangedDelegates(LocalUserNum, ELoginStatus::NotLoggedIn, ELoginStatus::LoggedIn, *UserId);
		});
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("Twitch login for user %d failed: %s"), LocalUserNum, *ErrorStr);
		Subsystem->ExecuteNextTick([this, LocalUserNum, ErrorStrCopy]()
		{
			TriggerOnLoginCompleteDelegates(LocalUserNum, false, *ZeroId, ErrorStrCopy);
		});
	}
}

void FOnlineIdentityTwitch::OnExternalUILoginComplete(TSharedPtr<const FUniqueNetId> UniqueId, const int ControllerIndex)
{
	FString ErrorStr;
	bool bWasSuccessful = UniqueId.IsValid() && UniqueId->IsValid();
	OnAccessTokenLoginComplete(ControllerIndex, bWasSuccessful, bWasSuccessful ? *UniqueId : *ZeroId, ErrorStr);
}

bool FOnlineIdentityTwitch::Logout(int32 LocalUserNum)
{
	bool bResult = false;
	TSharedPtr<const FUniqueNetId> UserId = GetUniquePlayerId(LocalUserNum);
	if (UserId.IsValid())
	{
		FString AuthToken = GetAuthToken(LocalUserNum);
		if (bRevokeTokenOnLogout && !AuthToken.IsEmpty())
		{
			RevokeAuthToken(LocalUserNum, AuthToken, FOnLogoutCompleteDelegate::CreateThreadSafeSP(this, &FOnlineIdentityTwitch::OnRevokeAuthTokenComplete));
		}
		else
		{
			Subsystem->ExecuteNextTick([this, LocalUserNum]()
			{
				OnTwitchLogoutComplete(LocalUserNum);
			});
		}
		bResult = true;
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("No logged in user found for LocalUserNum=%d."),
			LocalUserNum);
		Subsystem->ExecuteNextTick([this, LocalUserNum]() {
			TriggerOnLogoutCompleteDelegates(LocalUserNum, false);
		});
	}
	return bResult;
}

void FOnlineIdentityTwitch::OnTwitchLogoutComplete(int32 LocalUserNum)
{
	UE_LOG_ONLINE(Log, TEXT("Twitch logout for user %d complete"), LocalUserNum);
	TSharedPtr<const FUniqueNetId> UserId = GetUniquePlayerId(LocalUserNum);
	if (ensure(UserId.IsValid()))
	{
		// remove cached user account
		UserAccounts.Remove(UserId->ToString());
		// remove cached user id
		UserIds.Remove(LocalUserNum);

		TriggerOnLoginFlowLogoutDelegates(LoginDomains);

		Subsystem->ExecuteNextTick([this, UserId, LocalUserNum]()
		{
			TriggerOnLogoutCompleteDelegates(LocalUserNum, true);
			TriggerOnLoginStatusChangedDelegates(LocalUserNum, ELoginStatus::LoggedIn, ELoginStatus::NotLoggedIn, *UserId);
		});
	}
}

void FOnlineIdentityTwitch::RevokeAuthToken(int32 LocalUserNum, const FString& AuthToken, const FOnLogoutCompleteDelegate& InCompletionDelegate)
{
	// kick off http request to validate access token
	TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();

	FString Url(TokenRevokeUrl);
	FString PostData(FString::Printf(TEXT("client_id=%s&token=%s"), *FGenericPlatformHttp::UrlEncode(LoginURLDetails.ClientId), *FGenericPlatformHttp::UrlEncode(AuthToken)));

	HttpRequest->OnProcessRequestComplete().BindThreadSafeSP(this, &FOnlineIdentityTwitch::RevokeAuthToken_HttpRequestComplete, LocalUserNum, InCompletionDelegate);
	HttpRequest->SetURL(TokenRevokeUrl);
	HttpRequest->SetHeader(TEXT("Accept"), Subsystem->GetTwitchApiVersion());
	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/x-www-form-urlencoded"));
	HttpRequest->SetVerb(TEXT("POST"));
	HttpRequest->SetContentAsString(PostData);
	HttpRequest->ProcessRequest();
}

void FOnlineIdentityTwitch::RevokeAuthToken_HttpRequestComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded, int32 LocalUserNum, const FOnLogoutCompleteDelegate InCompletionDelegate)
{
	bool bWasSuccessful = false;

	FString ResponseStr, ErrorStr;

	if (bSucceeded &&
		HttpResponse.IsValid())
	{
		if (EHttpResponseCodes::IsOk(HttpResponse->GetResponseCode()) &&
			HttpResponse->GetContentType().StartsWith(TEXT("application/json")))
		{
			bWasSuccessful = true;
			ResponseStr = HttpResponse->GetContentAsString();
			UE_LOG_ONLINE(Verbose, TEXT("Revoke auth token request complete for user %d. url=%s code=%d response=%s"),
				LocalUserNum, *HttpRequest->GetURL(), HttpResponse->GetResponseCode(), *ResponseStr);
		}
		else
		{
			ErrorStr = FString::Printf(TEXT("Invalid response. code=%d contentType=%s error=%s"),
				HttpResponse->GetResponseCode(), *HttpResponse->GetContentType(), *ResponseStr);
		}
	}
	else
	{
		ErrorStr = TEXT("No response");
	}
	
	InCompletionDelegate.ExecuteIfBound(LocalUserNum, bWasSuccessful);
}

void FOnlineIdentityTwitch::OnRevokeAuthTokenComplete(int32 LocalUserNum, bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		UE_LOG_ONLINE(Log, TEXT("User %d successfully revoked their auth token"), LocalUserNum);
	}
	else
	{
		UE_LOG_ONLINE(Log, TEXT("User %d failed to revoke their auth token"), LocalUserNum);
	}
	OnTwitchLogoutComplete(LocalUserNum);
}

bool FOnlineIdentityTwitch::AutoLogin(int32 LocalUserNum)
{
	return false;
}

ELoginStatus::Type FOnlineIdentityTwitch::GetLoginStatus(int32 LocalUserNum) const
{
	TSharedPtr<const FUniqueNetId> UserId = GetUniquePlayerId(LocalUserNum);
	if (UserId.IsValid())
	{
		return GetLoginStatus(*UserId);
	}
	return ELoginStatus::NotLoggedIn;
}

ELoginStatus::Type FOnlineIdentityTwitch::GetLoginStatus(const FUniqueNetId& UserId) const
{
	TSharedPtr<FUserOnlineAccount> UserAccount = GetUserAccount(UserId);
	if (UserAccount.IsValid() &&
		UserAccount->GetUserId()->IsValid() &&
		!UserAccount->GetAccessToken().IsEmpty())
	{
		return ELoginStatus::LoggedIn;
	}
	return ELoginStatus::NotLoggedIn;
}

FString FOnlineIdentityTwitch::GetPlayerNickname(int32 LocalUserNum) const
{
	TSharedPtr<const FUniqueNetId> UserId = GetUniquePlayerId(LocalUserNum);
	if (UserId.IsValid())
	{
		return GetPlayerNickname(*UserId);
	}
	return TEXT("InvalidTwitchUser");
}

FString FOnlineIdentityTwitch::GetPlayerNickname(const FUniqueNetId& UserId) const
{
	// display name will be cached for users that registered or logged in manually
	const TSharedRef<FUserOnlineAccountTwitch>* FoundUserAccount = UserAccounts.Find(UserId.ToString());
	if (FoundUserAccount != nullptr)
	{
		FString DisplayName = (*FoundUserAccount)->GetDisplayName();
		if (!DisplayName.IsEmpty())
		{
			return DisplayName;
		}
	}
	return TEXT("InvalidTwitchUser");
}

FString FOnlineIdentityTwitch::GetAuthToken(int32 LocalUserNum) const
{
	FString AuthToken;

	TSharedPtr<const FUniqueNetId> UserId = GetUniquePlayerId(LocalUserNum);
	if (UserId.IsValid())
	{
		TSharedPtr<FUserOnlineAccountTwitch> FoundUserAccount = GetUserAccountTwitch(*UserId);
		if (FoundUserAccount.IsValid())
		{
			AuthToken = FoundUserAccount->GetAccessToken();
		}
	}

	return AuthToken;
}

FOnlineIdentityTwitch::FOnlineIdentityTwitch(class FOnlineSubsystemTwitch* InSubsystem)
	: Subsystem(InSubsystem)
	, bHasLoginOutstanding(false)
	, bRevokeTokenOnLogout(false)
	, ZeroId(MakeShared<FUniqueNetIdString>())
{
	check(InSubsystem);
	if (!GConfig->GetString(TEXT("OnlineSubsystemTwitch.OnlineIdentityTwitch"), TEXT("LoginUrl"), LoginURLDetails.LoginUrl, GEngineIni))
	{
		UE_LOG_ONLINE(Warning, TEXT("Missing LoginUrl= in [OnlineSubsystemTwitch.OnlineIdentityTwitch] of DefaultEngine.ini"));
	}
	if (!GConfig->GetBool(TEXT("OnlineSubsystemTwitch.OnlineIdentityTwitch"), TEXT("bForceVerify"), LoginURLDetails.bForceVerify, GEngineIni))
	{
		UE_LOG_ONLINE(Warning, TEXT("Missing bForceVerify= in [OnlineSubsystemTwitch.OnlineIdentityTwitch] of DefaultEngine.ini"));
	}
	if (!GConfig->GetString(TEXT("OnlineSubsystemTwitch.OnlineIdentityTwitch"), TEXT("LoginRedirectUrl"), LoginURLDetails.LoginRedirectUrl, GEngineIni))
	{
		UE_LOG_ONLINE(Warning, TEXT("Missing LoginRedirectUrl= in [OnlineSubsystemTwitch.OnlineIdentityTwitch] of DefaultEngine.ini"));
	}
	if (!GConfig->GetString(TEXT("OnlineSubsystemTwitch.OnlineIdentityTwitch"), TEXT("TokenValidateUrl"), LoginURLDetails.TokenValidateUrl, GEngineIni))
	{
		UE_LOG_ONLINE(Warning, TEXT("Missing TokenValidateUrl= in [OnlineSubsystemTwitch.OnlineIdentityTwitch] of DefaultEngine.ini"));
	}

	LoginURLDetails.ClientId = Subsystem->GetAppId();

	GConfig->GetArray(TEXT("OnlineSubsystemTwitch.OnlineIdentityTwitch"), TEXT("LoginDomains"), LoginDomains, GEngineIni);

	// Setup permission scope fields
	GConfig->GetArray(TEXT("OnlineSubsystemTwitch.OnlineIdentityTwitch"), TEXT("ScopeFields"), LoginURLDetails.ScopeFields, GEngineIni);

	if (!GConfig->GetString(TEXT("OnlineSubsystemTwitch.OnlineIdentityTwitch"), TEXT("TokenRevokeUrl"), TokenRevokeUrl, GEngineIni))
	{
		UE_LOG_ONLINE(Warning, TEXT("Missing TokenRevokeUrl= in [OnlineSubsystemTwitch.OnlineIdentityTwitch] of DefaultEngine.ini"));
	}
	if (!GConfig->GetBool(TEXT("OnlineSubsystemTwitch.OnlineIdentityTwitch"), TEXT("bRevokeTokenOnLogout"), bRevokeTokenOnLogout, GEngineIni))
	{
		UE_LOG_ONLINE(Warning, TEXT("Missing bRevokeTokenOnLogout= in [OnlineSubsystemTwitch.OnlineIdentityTwitch] of DefaultEngine.ini"));
	}
}

FPlatformUserId FOnlineIdentityTwitch::GetPlatformUserIdFromUniqueNetId(const FUniqueNetId& UniqueNetId)
{
	for (int PlayerIdx = 0; PlayerIdx < MAX_LOCAL_PLAYERS; ++PlayerIdx)
	{
		TSharedPtr<const FUniqueNetId> CurrentUniqueId = GetUniquePlayerId(PlayerIdx);
		if (CurrentUniqueId.IsValid() && (*CurrentUniqueId == UniqueNetId))
		{
			return PlayerIdx;
		}
	}

	return PLATFORMUSERID_NONE;
}

void FOnlineIdentityTwitch::GetUserPrivilege(const FUniqueNetId& UserId, EUserPrivileges::Type Privilege, const FOnGetUserPrivilegeCompleteDelegate& Delegate)
{
	Delegate.ExecuteIfBound(UserId, Privilege, (uint32)EPrivilegeResults::NoFailures);
}

FString FOnlineIdentityTwitch::GetAuthType() const
{
	return TEXT("twitch");
}
