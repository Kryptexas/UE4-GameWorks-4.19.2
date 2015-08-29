// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class FChatChromeViewModel
	: public TSharedFromThis<FChatChromeViewModel>
{
public:

	virtual void AddTab(const TSharedRef<class IChatTabViewModel>& Tab) = 0;
	virtual void ActivateTab(const TSharedRef<class IChatTabViewModel>& Tab) = 0;
	virtual TSharedPtr<class IChatTabViewModel> GetActiveTab() const = 0;
	virtual TArray<TSharedRef<IChatTabViewModel>>& GetVisibleTabs() = 0;
	virtual TArray<TSharedRef<IChatTabViewModel>>& GetAllTabs() = 0;
	virtual bool IsFading() const = 0;
	virtual bool IsActive() const = 0;
	virtual EVisibility GetHeaderVisibility() const = 0;
	virtual TSharedPtr<class FChatSettingsViewModel> GetChatSettingsViewModel() = 0;

	virtual ~FChatChromeViewModel() {}


	DECLARE_EVENT(FChatChromeViewModel, FActiveTabChangedEvent)
	virtual FActiveTabChangedEvent& OnActiveTabChanged() = 0;

	DECLARE_EVENT(FChatChromeViewModel, FVisibleTabsChangedEvent)
	virtual FVisibleTabsChangedEvent& OnVisibleTabsChanged() = 0;

	DECLARE_EVENT(FChatChromeViewModel, FOnOpenSettingsEvent)
	virtual FOnOpenSettingsEvent& OnOpenSettings() = 0;
};

/**
 * Creates the implementation for an ChatChromeViewModel.
 *
 * @return the newly created ChatChromeViewModel implementation.
 */
FACTORY(TSharedRef< FChatChromeViewModel >, FChatChromeViewModel,
	const TSharedRef<class FFriendsNavigationService>& NavigationService,
	const TSharedRef<class IChatDisplayService> & ChatDisplayService,
	const TSharedRef<class IChatSettingsService>& ChatSettingsService);