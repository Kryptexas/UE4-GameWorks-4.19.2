// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "PluginWardenPrivatePCH.h"
#include "SAuthorizingPlugin.h"

#define LOCTEXT_NAMESPACE "PluginWarden"

extern TSet<FString> AuthorizedPlugins;

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

	PortalWindowService = GEditor->GetServiceLocator()->GetServiceRef<IPortalApplicationWindow>();
}

FText SAuthorizingPlugin::GetWaitingText() const
{
	switch ( CurrentState )
	{
	case EPluginAuthorizationState::Initializing:
	case EPluginAuthorizationState::StartLauncher:
		return LOCTEXT("StartingLauncher", "Starting Epic Games Launcher...");
	case EPluginAuthorizationState::WaitingForAvailableLauncher:
		return LOCTEXT("ConnectingToLauncher", "Connecting...");
	case EPluginAuthorizationState::AuthorizePlugin:
	case EPluginAuthorizationState::WaitingForPluginAuthorization:
		return FText::Format(LOCTEXT("CheckingIfYouCanUseFormat", "Checking license for {0}..."), PluginFriendlyName);
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
				if ( DesktopPlatform->OpenLauncher(false, FString(), FString(" -silent")) )
				{
					CurrentState = EPluginAuthorizationState::WaitingForAvailableLauncher;
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
		case EPluginAuthorizationState::WaitingForAvailableLauncher:
		{
			if ( PortalWindowService->IsAvailable() )
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
			// TODO GET API TO CHECK PLUGIN
			CurrentState = EPluginAuthorizationState::WaitingForPluginAuthorization;
			break;
		}
		case EPluginAuthorizationState::WaitingForPluginAuthorization:
		{
			WaitingTime += InDeltaTime;
			// TODO GET API TO CHECK PLUGIN
			break;
		}
		case EPluginAuthorizationState::SigninRequired:
		{
			//TODO Move Launcher sign-in to the foreground

			FText SigninMessage = FText::Format(LOCTEXT("UnauthorizedNeedSignin", "We can't tell if you've purchased {0}.\n\nCan you please sign-in to Epic Launcher and click [OK] when complete?"), PluginFriendlyName);
			EAppReturnType::Type Response = FMessageDialog::Open(EAppMsgType::OkCancel, SigninMessage);
			if ( Response == EAppReturnType::Cancel )
			{
				CurrentState = EPluginAuthorizationState::Canceled;
			}
			else
			{
				// Return to the authorize plug-in state now that the user has signed in.
				CurrentState = EPluginAuthorizationState::AuthorizePlugin;
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
		case EPluginAuthorizationState::WaitingForAvailableLauncher:
		case EPluginAuthorizationState::WaitingForPluginAuthorization:
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
