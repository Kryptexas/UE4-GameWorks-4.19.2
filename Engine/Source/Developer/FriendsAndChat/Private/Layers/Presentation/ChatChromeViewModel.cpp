// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "ChatChromeViewModel.h"
#include "FriendsNavigationService.h"
#include "ChatViewModel.h"
#include "IChatSettingsService.h"
#include "IChatTabViewModel.h"
#include "ChatSettingsViewModel.h"

class FChatChromeViewModelImpl
	: public FChatChromeViewModel
{
public:

	virtual void AddTab(const TSharedRef<IChatTabViewModel>& Tab) override
	{
		Tabs.Add(Tab);

		// Set a default tab if one is not active
		if (!ActiveTab.IsValid())
		{
			ActiveTab = Tab;
			ActiveTab->GetChatViewModel()->SetIsActive(true);
			ActiveTab->GetChatViewModel()->SetChatSettingsService(ChatSettingsService);
		}
	}

	virtual void ActivateTab(const TSharedRef<IChatTabViewModel>& Tab) override
	{
		if(ActiveTab.IsValid())
		{
			ActiveTab->GetChatViewModel()->SetIsActive(false);
		}
		ActiveTab = Tab;
		ActiveTab->GetChatViewModel()->SetIsActive(true);
		
		OnActiveTabChanged().Broadcast();
	}

	virtual TSharedPtr<IChatTabViewModel> GetActiveTab() const override
	{
		return ActiveTab;
	}

	virtual TArray<TSharedRef<IChatTabViewModel>>& GetAllTabs() override
	{
		return Tabs;
	}
	
	virtual TArray<TSharedRef<IChatTabViewModel>>& GetVisibleTabs() override
	{
		VisibleTabs.Empty();
		for (TSharedRef<IChatTabViewModel> Tab : Tabs)
		{
			if (Tab->IsTabVisible())
			{
				VisibleTabs.Add(Tab);
			}
		}
		return VisibleTabs;
	}

	virtual TSharedPtr <FChatSettingsViewModel> GetChatSettingsViewModel() override
	{
		if (!ChatSettingsViewModel.IsValid())
		{
			ChatSettingsViewModel = FChatSettingsViewModelFactory::Create(ChatSettingsService);
		}
		return ChatSettingsViewModel;
	}

	virtual bool IsFading() const override
	{
		return ChatDisplayService->IsFading();
	}

	virtual bool IsActive() const override
	{
		return ChatDisplayService->IsActive();
	}

	virtual EVisibility GetHeaderVisibility() const override
	{
		return ChatDisplayService->GetEntryBarVisibility();
	}

	DECLARE_DERIVED_EVENT(FChatChromeViewModelImpl, FChatChromeViewModel::FActiveTabChangedEvent, FActiveTabChangedEvent)
	virtual FActiveTabChangedEvent& OnActiveTabChanged() { return ActiveTabChangedEvent; }

	DECLARE_DERIVED_EVENT(FChatChromeViewModelImpl, FChatChromeViewModel::FVisibleTabsChangedEvent, FVisibleTabsChangedEvent)
	virtual FVisibleTabsChangedEvent& OnVisibleTabsChanged() { return VisibleTabsChangedEvent; }

	DECLARE_DERIVED_EVENT(FChatChromeViewModelImpl, FChatChromeViewModel::FOnOpenSettingsEvent, FOnOpenSettingsEvent)
	virtual FOnOpenSettingsEvent& OnOpenSettings() { return OpenSettingsEvent; }

private:

	void HandleViewChangedEvent(EChatMessageType::Type NewChannel)
	{
		for (const auto& Tab: Tabs)
		{
			if(Tab->GetTabID() == NewChannel)
			{
				if (Tab->IsTabVisible())
				{
					if (!VisibleTabs.Contains(Tab))
					{
						OnVisibleTabsChanged().Broadcast();
					}
					ActivateTab(Tab);
				}
				break;
			}
		}
	}

	void HandleChannelChangedEvent(EChatMessageType::Type ChannelSelected)
	{
		ActiveTab->GetChatViewModel()->SetOutgoingMessageChannel(ChannelSelected);
	}

	void HandleSettingsChanged(EChatSettingsType::Type OptionType, bool NewState)
	{
// 		if (NewState == ECheckBoxState::Unchecked)
// 		{
// 		}
	}

	void Initialize()
	{
		ChatSettingsService->OnChatSettingStateUpdated().AddSP(this, &FChatChromeViewModelImpl::HandleSettingsChanged);
		NavigationService->OnChatViewChanged().AddSP(this, &FChatChromeViewModelImpl::HandleViewChangedEvent);
		NavigationService->OnChatChannelChanged().AddSP(this, &FChatChromeViewModelImpl::HandleChannelChangedEvent);
	}

	FChatChromeViewModelImpl(const TSharedRef<FFriendsNavigationService>& InNavigationService, const TSharedRef<IChatDisplayService> & InChatDisplayService, const TSharedRef<IChatSettingsService>& InChatSettingsService)
		: NavigationService(InNavigationService)
		, ChatDisplayService(InChatDisplayService)
		, ChatSettingsService(InChatSettingsService)
	{};

private:

	FActiveTabChangedEvent ActiveTabChangedEvent;
	FVisibleTabsChangedEvent VisibleTabsChangedEvent;
	FOnOpenSettingsEvent OpenSettingsEvent;

	TSharedPtr<IChatTabViewModel> ActiveTab;
	TArray<TSharedRef<IChatTabViewModel>> Tabs;
	TArray<TSharedRef<IChatTabViewModel>> VisibleTabs;
	TSharedRef<FFriendsNavigationService> NavigationService;
	TSharedRef<IChatDisplayService> ChatDisplayService;
	TSharedRef<IChatSettingsService> ChatSettingsService;
	TSharedPtr<FChatSettingsViewModel> ChatSettingsViewModel;

	friend FChatChromeViewModelFactory;
};

TSharedRef< FChatChromeViewModel > FChatChromeViewModelFactory::Create(const TSharedRef<FFriendsNavigationService>& NavigationService, const TSharedRef<IChatDisplayService> & ChatDisplayService, const TSharedRef<IChatSettingsService>& InChatSettingsService)
{
	TSharedRef< FChatChromeViewModelImpl > ViewModel(new FChatChromeViewModelImpl(NavigationService, ChatDisplayService, InChatSettingsService));
	ViewModel->Initialize();
	return ViewModel;
}