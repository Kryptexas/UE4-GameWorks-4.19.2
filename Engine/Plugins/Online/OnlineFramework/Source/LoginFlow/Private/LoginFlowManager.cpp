// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "LoginFlowPrivatePCH.h"
#include "LoginFlowManager.h"
#include "OnlineSubsystem.h"
#include "OnlineIdentityInterface.h"
#include "OnlineExternalUIInterface.h"
#include "OnlineError.h"

DEFINE_LOG_CATEGORY(LogLoginFlow);

#define LOGIN_TOKEN_FOUND FString()
#define LOGIN_ERROR_UNKNOWN TEXT("com.epicgames.login.unknown")
#define LOGIN_CANCELLED TEXT("com.epicgames.login.cancelled")
#define LOGIN_PAGELOADFAILED TEXT("com.epicgames.login.pageloadfailed")
#define LOGIN_CEFLOADFAILED TEXT("com.epicgames.login.cefloadfailed")

struct FLoginFlowProperties
{
	/** Instance of the login flow */
	FString InstanceId;
	/** Delegate fired on every RedirectURL detected by the web interface */
	FOnLoginRedirectURL OnRedirectURL;
	/** Wrapper slate widget around the actual login flow web page */
	TSharedPtr<SBox> PopupHolder;
	/** Delegate fired externally when the login flow is dismissed */
	FLoginFlowManager::FOnPopupDismissed OnPopupDismissed;
	/** Delegate fired when the login flow is complete for any reason */
	FOnLoginFlowComplete OnComplete;
	/** Structure containing results of login flow attempt */
	FLoginFlowResult Result;
	/** Is the login flow actively being shown */
	bool bIsDisplayed;
};

FLoginFlowManager::FLoginFlowManager()
	: bLoginFlowInProgress(false)
{
}

FLoginFlowManager::~FLoginFlowManager()
{
	// if we're in an active flow, just fire dismissal then shut down
	if (PendingLogin.IsValid())
	{
		PendingLogin->OnPopupDismissed.ExecuteIfBound();
		PendingLogin = nullptr;
	}

	for (auto& OnlineParamPair : OnlineSubsystemsMap)
	{
		FOnlineParams& OnlineParams = OnlineParamPair.Value;
		IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get(OnlineParams.OnlineIdentifier);
		if (OnlineSub != nullptr)
		{
			IOnlineExternalUIPtr OnlineExternalUI = OnlineSub->GetExternalUIInterface();
			if (OnlineExternalUI.IsValid())
			{
				OnlineExternalUI->ClearOnLoginFlowUIRequiredDelegate_Handle(OnlineParams.LoginFlowUIRequiredDelegateHandle);
			}
		}
	}
	OnlineSubsystemsMap.Empty();
}

bool FLoginFlowManager::AddLoginFlow(FName OnlineIdentifier, const FOnDisplayPopup& InPopupDelegate)
{
	bool bSuccess = false;

	FOnlineParams* Params = OnlineSubsystemsMap.Find(OnlineIdentifier);
	if (Params == nullptr)
	{
		IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get(OnlineIdentifier);
		if (OnlineSub != nullptr)
		{
			// bind to the OSS delegate
			IOnlineExternalUIPtr OnlineExternalUI = OnlineSub->GetExternalUIInterface();
			if (OnlineExternalUI.IsValid())
			{
				FOnlineParams& NewParams = OnlineSubsystemsMap.Add(OnlineIdentifier);
				NewParams.OnlineIdentifier = OnlineIdentifier;
				NewParams.OnDisplayPopup = InPopupDelegate;
				NewParams.LoginFlowUIRequiredDelegateHandle = OnlineExternalUI->AddOnLoginFlowUIRequiredDelegate_Handle(FOnLoginFlowUIRequiredDelegate::CreateSP(this, &FLoginFlowManager::OnLoginFlowStarted, OnlineIdentifier));
				bSuccess = true;
			}
		}
		else
		{
			UE_LOG(LogLoginFlow, Warning, TEXT("No OSS specified. Login flow will be disabled."));
		}
	}
	else
	{
		UE_LOG(LogLoginFlow, Warning, TEXT("OSS already registered, skipping [%s]"), *OnlineIdentifier.ToString());
	}

	return bSuccess;
}

void FLoginFlowManager::OnLoginFlowStarted(const FString& RequestedURL, const FOnLoginRedirectURL& OnRedirectURL, const FOnLoginFlowComplete& OnLoginFlowComplete, bool& bOutShouldContinueLogin, FName InOnlineIdentifier)
{
	bOutShouldContinueLogin = false;

	FOnlineParams* Params = OnlineSubsystemsMap.Find(InOnlineIdentifier);
	if (Params)
	{
		// check for login in flight (one at a time)
		if (PendingLogin.IsValid())
		{
			UE_LOG(LogLoginFlow, Error, TEXT("Simultaneous login flows not supported"));
			return;
		}

		// make sure we have a display callback currently bound
		if (!Params->OnDisplayPopup.IsBound())
		{
			UE_LOG(LogLoginFlow, Error, TEXT("Login did not have display code bound to OnLoginFlowStarted."));
			return;
		}

		bOutShouldContinueLogin = true;

		// save the pending order for reference later
		PendingLogin = MakeShareable(new FLoginFlowProperties());
		PendingLogin->InstanceId = FGuid::NewGuid().ToString();
		PendingLogin->OnRedirectURL = OnRedirectURL;
		PendingLogin->OnComplete = OnLoginFlowComplete;
		PendingLogin->bIsDisplayed = false;

		// create a placeholder widget to display while this process is going on
		SAssignNew(PendingLogin->PopupHolder, SBox);

		// give the widget to the App to display and get back a callback we should use to dismiss it
		FString AnalyticsSessionId;
		PendingLogin->OnPopupDismissed = Params->OnDisplayPopup.Execute(PendingLogin->PopupHolder.ToSharedRef());

		// generate a login flow chromium widget
		ILoginFlowModule::FCreateSettings CreateSettings;
		CreateSettings.Url = RequestedURL;

		// Setup will allow login flow module events to trigger passed in delegates
		CreateSettings.CloseCallback = FOnLoginFlowRequestClose::CreateSP(this, &FLoginFlowManager::OnLoginFlow_Close, PendingLogin->InstanceId);
		CreateSettings.ErrorCallback = FOnLoginFlowError::CreateSP(this, &FLoginFlowManager::OnLoginFlow_Error, PendingLogin->InstanceId);
		CreateSettings.RedirectCallback = FOnLoginFlowRedirectURL::CreateSP(this, &FLoginFlowManager::OnLoginFlow_RedirectURL, PendingLogin->InstanceId);
		TSharedRef<SWidget> LoginFlowWidget = ILoginFlowModule::Get().CreateLoginFlowWidget(CreateSettings);

		// make sure none of the delegates were called during creation (would invalidate this ptr)
		if (PendingLogin.IsValid())
		{
			// put this widget into the holder
			PendingLogin->PopupHolder->SetContent(LoginFlowWidget);
			PendingLogin->bIsDisplayed = true;

			// focus the login flow widget
			FSlateApplication::Get().SetKeyboardFocus(LoginFlowWidget, EFocusCause::SetDirectly);
		}
	}
	else
	{
		UE_LOG(LogLoginFlow, Error, TEXT("Online platform requesting login flow not registered [%s]"), *InOnlineIdentifier.ToString());
	}
}

void FLoginFlowManager::OnLoginFlow_Error(ELoginFlowErrorResult ErrorType, const FString& ErrorInfo, FString InstanceId)
{
	if (!PendingLogin.IsValid() || PendingLogin->InstanceId != InstanceId)
	{
		// assume we got canceled
		return;
	}

	FString ErrorString = ErrorInfo;
	if (ErrorString.IsEmpty())
	{
		// try to get more info from the type
		switch (ErrorType)
		{
		case ELoginFlowErrorResult::LoadFail:
			ErrorString = PendingLogin->bIsDisplayed ? LOGIN_PAGELOADFAILED : LOGIN_CEFLOADFAILED;
			break;
		default:
			ErrorString = LOGIN_ERROR_UNKNOWN;
			break;
		}
	}

	PendingLogin->Result.Error.SetFromErrorCode(ErrorString);
	FinishLogin();
}

void FLoginFlowManager::OnLoginFlow_Close(const FString& CloseInfo, FString InstanceId)
{
	if (!PendingLogin.IsValid() || PendingLogin->InstanceId != InstanceId)
	{
		// assume we got canceled
		return;
	}
 
	PendingLogin->Result.Error.SetFromErrorCode(CloseInfo);
	FinishLogin();
}

bool FLoginFlowManager::OnLoginFlow_RedirectURL(const FString& RedirectURL, FString InstanceId)
{
	if (!PendingLogin.IsValid() || PendingLogin->InstanceId != InstanceId)
	{
		return false;
	}

	FLoginFlowResult Result = PendingLogin->OnRedirectURL.Execute(RedirectURL);
	if (Result.IsComplete())
	{
		PendingLogin->Result = Result;
		FinishLogin();
		return true;
	}

	return false;
}

void FLoginFlowManager::CancelLoginFlow()
{
	if (!PendingLogin.IsValid())
	{
		return;
	}

	PendingLogin->Result.Error.SetFromErrorCode(LOGIN_CANCELLED);
	FinishLogin();
}

void FLoginFlowManager::FinishLogin()
{
	check(PendingLogin.IsValid());

	if (!PendingLogin->Result.Error.bSucceeded)
	{
		UE_LOG(LogLoginFlow, Warning, TEXT("Login Flow failed with error: %s"), PendingLogin->Result.Error.ToLogString());
	}

	// dismiss the popup
	PendingLogin->OnPopupDismissed.ExecuteIfBound();

	// fire the login complete callback
	PendingLogin->OnComplete.Execute(PendingLogin->Result);

	// clear this object so we can handle another login
	PendingLogin.Reset();
}
