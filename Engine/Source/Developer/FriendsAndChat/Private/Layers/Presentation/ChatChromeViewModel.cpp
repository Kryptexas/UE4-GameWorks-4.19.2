// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "ChatChromeViewModel.h"
#include "FriendsNavigationService.h"
#include "GameAndPartyService.h"
#include "ChatViewModel.h"
#include "IChatSettingsService.h"
#include "IChatTabViewModel.h"
#include "ChatSettingsViewModel.h"
#include "ChatDisplayService.h"
#include "ChatSettingsService.h"

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
		}
		Tab->GetChatViewModel()->SetChatSettingsService(ChatSettingsService);
	}

	virtual void ActivateTab(const TSharedRef<IChatTabViewModel>& InTab) override
	{
		if(ActiveTab.IsValid())
		{
			ActiveTab->GetChatViewModel()->SetIsActive(false);
		}
		ActiveTab = InTab;
		ActiveTab->GetChatViewModel()->SetIsActive(true);

		// Tell Other tabs what messages we read
		ActiveTab->GetChatViewModel()->SetReadChannelFlags(0);
		for (TSharedRef<IChatTabViewModel> Tab : Tabs)
		{
			if (Tab != ActiveTab)
			{
				Tab->GetChatViewModel()->SetReadChannelFlags(ActiveTab->GetChatViewModel()->GetChannelFlags());
			}
		}
		
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
		return ChatDisplayService->GetChatHeaderVisibiliy();
	}

	virtual bool DisplayChatSettings() const override
	{
		return !NavigationService->IsInGame();
	}

	virtual void ToggleChatMinimized() override
	{
		ChatDisplayService->ToggleChatMinimized();

		if (IsChatMinimized())
		{
			for (TSharedRef<IChatTabViewModel> Tab : Tabs)
			{
				Tab->GetChatViewModel()->SetMessageShown(false, 0);
			}
		}
	}

	virtual bool IsMinimizeEnabled() const override
	{
		return ChatDisplayService->IsChatMinimizeEnabled();
	}

	virtual bool IsChatMinimized() const override
	{
		return ChatDisplayService->IsChatMinimized();
	}

	virtual TSharedRef<FChatChromeViewModel> Clone(TSharedRef<IChatDisplayService> InChatDisplayService, TSharedRef<IChatSettingsService> InChatSettingsService, TArray<TSharedRef<ICustomSlashCommand> >* CustomSlashCommands) override
	{
		TSharedRef< FChatChromeViewModelImpl > ViewModel(new FChatChromeViewModelImpl(NavigationService, GamePartyService, InChatDisplayService, InChatSettingsService));
		ViewModel->Initialize();
		for (const auto& Tab: Tabs)
		{
			ViewModel->AddTab(Tab->Clone(InChatDisplayService, CustomSlashCommands));
		}

		if(ActiveTab.IsValid())
		{
			for (const auto& Tab: ViewModel->Tabs)
			{
				if(ActiveTab->GetTabID() == Tab->GetTabID())
				{
					ViewModel->ActivateTab(Tab);
					break;
				}
			}
		}

		return ViewModel;
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
		EChatMessageType::Type SelectedChannel = NewChannel;
		// Hack to combine Party and Game tabs - NDavies
		if(NewChannel == EChatMessageType::Game)
		{
			SelectedChannel = EChatMessageType::Party;
		}

		for (const auto& Tab: Tabs)
		{
			if(Tab->GetTabID() == SelectedChannel)
			{
				if (!VisibleTabs.Contains(Tab))
				{
					OnVisibleTabsChanged().Broadcast();
				}
				ActivateTab(Tab);
				break;
			}
		}

		if(NewChannel == EChatMessageType::Game)
		{
			ActiveTab->GetChatViewModel()->SetOutgoingMessageChannel(EChatMessageType::Game);
		}
	}

	void HandleSettingsChanged(EChatSettingsType::Type OptionType, bool NewState)
	{
// 		if (NewState == ECheckBoxState::Unchecked)
// 		{
// 		}
	}

	void HandlePartyChanged()
	{
		if(GamePartyService->IsInPartyChat() || GamePartyService->IsInGameSession())
		{
			for (const auto& Tab: Tabs)
			{
				if(Tab->GetTabID() == EChatMessageType::Party)
				{
					if (!VisibleTabs.Contains(Tab))
					{
						ActivateTab(Tab);
						NavigationService->ChangeViewChannel(GamePartyService->IsInGameSession() ? EChatMessageType::Game : EChatMessageType::Party);
					}
					break;
				}
			}
		}
		else if(ActiveTab.IsValid() && ActiveTab->GetTabID() == EChatMessageType::Party)
		{
			NavigationService->ChangeViewChannel(EChatMessageType::Global);
		}
	}

	void Initialize()
	{
		ChatSettingsService->OnChatSettingStateUpdated().AddSP(this, &FChatChromeViewModelImpl::HandleSettingsChanged);
		NavigationService->OnChatViewChanged().AddSP(this, &FChatChromeViewModelImpl::HandleViewChangedEvent);
		GamePartyService->OnPartyMembersChanged().AddSP(this, &FChatChromeViewModelImpl::HandlePartyChanged);
		GamePartyService->OnGameSessionChanged().AddSP(this, &FChatChromeViewModelImpl::HandlePartyChanged);
	}

	FChatChromeViewModelImpl(const TSharedRef<FFriendsNavigationService>& InNavigationService, const TSharedRef<FGameAndPartyService>& InGamePartyService, const TSharedRef<IChatDisplayService> & InChatDisplayService, const TSharedRef<IChatSettingsService>& InChatSettingsService)
		: NavigationService(InNavigationService)
		, GamePartyService(InGamePartyService)
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
	TSharedRef<FGameAndPartyService> GamePartyService;
	TSharedRef<IChatDisplayService> ChatDisplayService;
	TSharedRef<IChatSettingsService> ChatSettingsService;
	TSharedPtr<FChatSettingsViewModel> ChatSettingsViewModel;

	friend FChatChromeViewModelFactory;
};

TSharedRef< FChatChromeViewModel > FChatChromeViewModelFactory::Create(const TSharedRef<FFriendsNavigationService>& NavigationService, const TSharedRef<FGameAndPartyService>& GamePartyService, const TSharedRef<IChatDisplayService> & ChatDisplayService, const TSharedRef<IChatSettingsService>& InChatSettingsService)
{
	TSharedRef< FChatChromeViewModelImpl > ViewModel(new FChatChromeViewModelImpl(NavigationService, GamePartyService, ChatDisplayService, InChatSettingsService));
	ViewModel->Initialize();
	return ViewModel;
}