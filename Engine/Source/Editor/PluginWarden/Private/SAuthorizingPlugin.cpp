// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "PluginWardenPrivatePCH.h"

#include "SAuthorizingPlugin.h"

#define LOCTEXT_NAMESPACE "PluginWarden"

void SAuthorizingPlugin::Construct(const FArguments& InArgs, const TSharedRef<SWindow>& InParentWindow, const FText& InPluginFriendlyName, const FString& InPluginGuid, TFunction<void()> InAuthorizedCallback)
{
	CurrentState = EPluginAuthorizationState::Initializing;
	WaitingTime = 0;
	ParentWindow = InParentWindow;
	PluginFriendlyName = InPluginFriendlyName;
	PluginGuid = InPluginGuid;
	AuthorizedCallback = InAuthorizedCallback;

	InParentWindow->SetOnWindowClosed(FOnWindowClosed::CreateSP(this, &SAuthorizingPlugin::OnWindowClosed));
	bUserInterrupted = true;

	RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateSP(this, &SAuthorizingPlugin::RefreshStatus));

	ChildSlot
	[
		SNew(SBox)
		.MinDesiredWidth(500)
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SThrobber)
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(10, 0)
				[
					SNew(STextBlock)
					.Text(this, &SAuthorizingPlugin::GetWaitingText)
					.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 12))
				]
			]
		]
	];

	TSharedRef<IPortalServiceLocator> ServiceLocator = GEditor->GetServiceLocator();
	PortalWindowService = ServiceLocator->GetServiceRef<IPortalApplicationWindow>();
	PortalUserService = ServiceLocator->GetServiceRef<IPortalUser>();
	PortalUserLoginService = ServiceLocator->GetServiceRef<IPortalUserLogin>();
}

FText SAuthorizingPlugin::GetWaitingText() const
{
	switch ( CurrentState )
	{
	case EPluginAuthorizationState::Initializing:
	case EPluginAuthorizationState::StartLauncher:
		return LOCTEXT("StartingLauncher", "Starting Epic Games Launcher...");
	case EPluginAuthorizationState::StartLauncher_Waiting:
		return LOCTEXT("ConnectingToLauncher", "Connecting...");
	case EPluginAuthorizationState::AuthorizePlugin:
	case EPluginAuthorizationState::AuthorizePlugin_Waiting:
		return FText::Format(LOCTEXT("CheckingIfYouCanUseFormat", "Checking license for {0}..."), PluginFriendlyName);
	case EPluginAuthorizationState::IsUserSignedIn:
	case EPluginAuthorizationState::IsUserSignedIn_Waiting:
		return LOCTEXT("CheckingIfUserSignedIn", "Authorization failed, checking user information...");
	case EPluginAuthorizationState::SigninRequired:
	case EPluginAuthorizationState::SigninRequired_Waiting:
		return LOCTEXT("NeedUserToLoginToCheck", "Authorization failed, Sign-in required...");
	}

	return LOCTEXT("Processing", "Processing...");
}

EActiveTimerReturnType SAuthorizingPlugin::RefreshStatus(double InCurrentTime, float InDeltaTime)
{
	// Engine tick isn't running when the modal window is open, so we need to tick any core tickers
	// to as that's what the RPC system uses to update the current state of RPC calls.
	FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::GameThread);
	FTicker::GetCoreTicker().Tick(InDeltaTime);

	switch ( CurrentState )
	{
		case EPluginAuthorizationState::Initializing:
		{
			if ( PortalWindowService->IsAvailable() )
			{
				CurrentState = EPluginAuthorizationState::AuthorizePlugin;
			}
			else
			{
				CurrentState = EPluginAuthorizationState::StartLauncher;
			}
			break;
		}
		case EPluginAuthorizationState::StartLauncher:
		{
			WaitingTime = 0;
			IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();

			if ( DesktopPlatform != nullptr )
			{
				FOpenLauncherOptions SilentOpen;
				if ( DesktopPlatform->OpenLauncher(SilentOpen) )
				{
					CurrentState = EPluginAuthorizationState::StartLauncher_Waiting;
				}
				else
				{
					CurrentState = EPluginAuthorizationState::LauncherStartFailed;
				}
			}
			else
			{
				CurrentState = EPluginAuthorizationState::LauncherStartFailed;
			}
			break;
		}
		case EPluginAuthorizationState::StartLauncher_Waiting:
		{
			if ( PortalWindowService->IsAvailable() && PortalUserService->IsAvailable() )
			{
				CurrentState = EPluginAuthorizationState::AuthorizePlugin;
			}
			else
			{
				WaitingTime += InDeltaTime;
			}
			break;
		}
		case EPluginAuthorizationState::AuthorizePlugin:
		{
			WaitingTime = 0;
			EntitlementResult = PortalUserService->IsEntitledToItem(PluginGuid, EEntitlementCacheLevelRequest::Memory);
			CurrentState = EPluginAuthorizationState::AuthorizePlugin_Waiting;
			break;
		}
		case EPluginAuthorizationState::AuthorizePlugin_Waiting:
		{
			WaitingTime += InDeltaTime;
			
			check(EntitlementResult.GetFuture().IsValid());
			if ( EntitlementResult.GetFuture().IsReady() )
			{
				FPortalUserIsEntitledToItemResult Entitlement = EntitlementResult.GetFuture().Get();
				if ( Entitlement.IsEntitled )
				{
					CurrentState = EPluginAuthorizationState::Authorized;
				}
				else
				{
					CurrentState = EPluginAuthorizationState::IsUserSignedIn;
				}
			}

			break;
		}
		case EPluginAuthorizationState::IsUserSignedIn:
		{
			WaitingTime = 0;
			UserDetailsResult = PortalUserService->GetUserDetails();
			CurrentState = EPluginAuthorizationState::IsUserSignedIn_Waiting;
			break;
		}
		case EPluginAuthorizationState::IsUserSignedIn_Waiting:
		{
			WaitingTime += InDeltaTime;

			check(UserDetailsResult.GetFuture().IsValid());
			if ( UserDetailsResult.GetFuture().IsReady() )
			{
				FPortalUserDetails UserDetails = UserDetailsResult.GetFuture().Get();

				if ( UserDetails.IsSignedIn )
				{
					// if the user is signed in, and we're at this stage, we know they are unauthorized.
					CurrentState = EPluginAuthorizationState::Unauthorized;
				}
				else
				{
					// If they're not signed in, but they were unauthorized, they may have purchased it
					// they may just need to sign-in.
					if ( PortalUserLoginService->IsAvailable() )
					{
						CurrentState = EPluginAuthorizationState::SigninRequired;
					}
				}
			}

			break;
		}
		case EPluginAuthorizationState::SigninRequired:
		{
			WaitingTime = 0;
			UserSigninResult = PortalUserLoginService->PromptUserForSignIn();
			CurrentState = EPluginAuthorizationState::SigninRequired_Waiting;
			break;
		}
		case EPluginAuthorizationState::SigninRequired_Waiting:
		{
			// We don't advance the wait time in the sign-required state, as this may take a long time.

			check(UserSigninResult.GetFuture().IsValid());
			if ( UserSigninResult.GetFuture().IsReady() )
			{
				bool IsUserSignedIn = UserSigninResult.GetFuture().Get();
				if ( IsUserSignedIn )
				{
					// if the user is signed in, and we're at this stage, we know they are unauthorized.
					CurrentState = EPluginAuthorizationState::AuthorizePlugin;
				}
				else
				{
					CurrentState = EPluginAuthorizationState::Unauthorized;
				}
			}

			break;
		}
		case EPluginAuthorizationState::Authorized:
		case EPluginAuthorizationState::Unauthorized:
		case EPluginAuthorizationState::Timeout:
		case EPluginAuthorizationState::LauncherStartFailed:
		{
			bUserInterrupted = false;
			ParentWindow.Pin()->RequestDestroyWindow();
			break;
		}
		case EPluginAuthorizationState::Canceled:
		{
			bUserInterrupted = true;
			ParentWindow.Pin()->RequestDestroyWindow();
			break;
		}
		default:
		{
			// Should never be in a non-explicit state.
			check(false);
			break;
		}
		}

		// If we're in a waiting state, check to see if we're over the timeout period.
		switch ( CurrentState )
		{
		case EPluginAuthorizationState::StartLauncher_Waiting:
		case EPluginAuthorizationState::AuthorizePlugin_Waiting:
		case EPluginAuthorizationState::IsUserSignedIn_Waiting:
		// We Ignore EPluginAuthorizationState::SigninRequired_Waiting, that state could take forever, the user needs to sign-in or close the dialog.
		{
			const float TimeoutSeconds = 15;
			if ( WaitingTime > TimeoutSeconds )
			{
				bUserInterrupted = false;
				CurrentState = EPluginAuthorizationState::Timeout;
			}
			break;
		}
	}

	return EActiveTimerReturnType::Continue;
}

void SAuthorizingPlugin::OnWindowClosed(const TSharedRef<SWindow>& InWindow)
{
	if ( bUserInterrupted || CurrentState == EPluginAuthorizationState::Canceled )
	{
		// User interrupted or canceled, just close down.
	}
	else
	{
		if ( CurrentState == EPluginAuthorizationState::Authorized )
		{
			AuthorizedPlugins.Add(PluginGuid);
			AuthorizedCallback();
		}
		else
		{
			switch ( CurrentState )
			{
			case EPluginAuthorizationState::Timeout:
			{
				FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("TimeoutFailure", "Something went wrong.  We were unable to verify your access to the plugin before timing out."));
				break;
			}
			case EPluginAuthorizationState::LauncherStartFailed:
			{
				FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("LauncherStartFailure", "Something went wrong starting the launcher.  We were unable to verify your access to the plugin."));
				break;
			}
			case EPluginAuthorizationState::Unauthorized:
			{
				FText FailureMessage = FText::Format(LOCTEXT("UnathorizedFailure", "It doesn't look like you've purchased {0}.\n\nWould you like to see the store page?"), PluginFriendlyName);
				EAppReturnType::Type Response = FMessageDialog::Open(EAppMsgType::YesNo, FailureMessage);
				if ( Response == EAppReturnType::Yes )
				{
					ShowStorePageForPlugin();
				}
				break;
			}
			default:
			{
				// We expect all failure situations to have an explicit case.
				check(false);
				break;
			}
			}
		}
	}
}

void SAuthorizingPlugin::ShowStorePageForPlugin()
{
	TAsyncResult<bool> Result = PortalWindowService->NavigateTo(FString(TEXT("/ue/marketplace/offers/")) + PluginGuid);
}

#undef LOCTEXT_NAMESPACE
