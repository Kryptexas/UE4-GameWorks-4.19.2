// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

enum class EPluginAuthorizationState
{
	Initializing,
	StartLauncher,
	WaitingForAvailableLauncher,
	AuthorizePlugin,
	WaitingForPluginAuthorization,
	Authorized,
	Unauthorized,
	SigninRequired,
	LauncherStartFailed,
	Timeout,
	Canceled,
};

class IPortalApplicationWindow;

class SAuthorizingPlugin : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAuthorizingPlugin){}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<SWindow>& InParentWindow, const FText& InPluginFriendlyName, const FString& InPluginGuid, TFunction<void()> InAuthorizedCallback);

	FText GetWaitingText() const;

	EActiveTimerReturnType RefreshStatus(double InCurrentTime, float InDeltaTime);

	void OnWindowClosed(const TSharedRef<SWindow>& InWindow);

	/** Show the store page for the plug-in, happens in response to the user asking to see the store page when license detection fails. */
	void ShowStorePageForPlugin();

private:
	/** The parent window holding this dialog, for when we need to trigger a close. */
	TWeakPtr<SWindow> ParentWindow;

	/** The display name of the plug-in used in the auto generated dialog text. */
	FText PluginFriendlyName;

	/** The unique id of the plug-in on the marketplace. */
	FString PluginGuid;

	/** Flag for tracking user interruption of the process, either with the cancel button or the close button. */
	bool bUserInterrupted;

	/** The amount of time we've been waiting for confirmation for a given step.  It's possible a problem may arise and we need to timeout. */
	float WaitingTime;

	/** A pointer to the portal application communication service. */
	TSharedPtr<IPortalApplicationWindow> PortalWindowService;

	/** The current state of the plug-in auth pipeline. */
	EPluginAuthorizationState CurrentState;

	/** If the user is authorized to us the plug-in, we'll call this function to alert the plug-in that everything is good to go. */
	TFunction<void()> AuthorizedCallback;
};
