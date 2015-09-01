// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "ChatDisplayService.h"
#include "ChatViewModel.h"

class FChatDisplayServiceImpl
	: public FChatDisplayService
{
public:

	virtual void SetFocus() override
	{
		SetChatListVisibility(true);
		SetChatEntryVisibility(true);
		ChatMinimized = false;
		OnChatListSetFocus().Broadcast();
	}

	virtual void ChatEntered() override
	{
		SetChatEntryVisibility(true);
		SetChatListVisibility(true);
	}

	virtual void MessageCommitted() override
	{
		OnChatMessageCommitted().Broadcast();

		if (FadeChatEntry)
		{
			SetChatEntryVisibility(false);
		}
		else if(AutoReleaseFocus)
		{
			OnFocuseReleasedEvent().Broadcast();
		}

		SetChatListVisibility(true);

		if (IsChatMinimized())
		{
			ChatListVisibility = EVisibility::HitTestInvisible;
		}
	}

	virtual EVisibility GetEntryBarVisibility() const override
	{
		if(EntryVisibilityOverride.IsSet())
		{
			return EntryVisibilityOverride.GetValue();
		}
		return ChatEntryVisibility;
	}

	virtual EVisibility GetChatHeaderVisibiliy() const override
	{
		// Don't show the header when the chat list is hidden
		if(TabVisibilityOverride.IsSet())
		{
			return TabVisibilityOverride.GetValue();
		}

		return IsChatMinimized() ? EVisibility::Collapsed : ChatEntryVisibility;
	}

	virtual EVisibility GetChatWindowVisibiliy() const override
	{
		return IsChatMinimized() ? EVisibility::Collapsed : EVisibility::Visible;
	}

	virtual EVisibility GetChatMinimizedVisibility() const override
	{
		return IsChatMinimized() ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed;
	}

	virtual EVisibility GetChatListVisibility() const override
	{
		if(ChatListVisibilityOverride.IsSet())
		{
			return ChatListVisibilityOverride.GetValue();
		}
		return ChatListVisibility;
	}

	virtual void SetFadeBackgroundEnabled(bool bEnabled) override
	{
		FadeBackgroundEnabled = bEnabled;
	}

	virtual bool IsFadeBackgroundEnabled() const override
	{
		return FadeBackgroundEnabled;
	}

	virtual EVisibility GetBackgroundVisibility() const override
	{
		if(BackgroundVisibilityOverride.IsSet())
		{
			return BackgroundVisibilityOverride.GetValue();
		}
		return IsChatMinimized() ? EVisibility::Hidden : EVisibility::Visible;
	}

	virtual bool IsFading() const override
	{
		return FadeChatList || FadeChatEntry;
	}

	virtual void SetActive(bool bIsActive) override
	{
		IsChatActive = bIsActive;
		if(IsChatActive)
		{
			SetFocus();
		}
	}

	virtual bool IsActive() const override
	{
		return IsChatActive;
	}

	virtual bool IsChatMinimizeEnabled() override
	{
		return ChatMinimizeEnabled;
	}

	virtual bool IsChatAutoMinimizeEnabled() override
	{
		return ChatAutoMinimizeEnabled;
	}

	virtual void ToggleChatMinimized() override
	{
		if (!IsChatMinimizeEnabled())
		{
			return;
		}

		ChatMinimized = !ChatMinimized;
		if(ChatMinimized)
		{
			ChatFadeDelay = ChatFadeInterval;
			if (!TickDelegate.IsBound())
			{
				TickDelegate = FTickerDelegate::CreateSP(this, &FChatDisplayServiceImpl::HandleTick);
				TickerHandle = FTicker::GetCoreTicker().AddTicker(TickDelegate);
			}
			ChatListVisibility = EVisibility::HitTestInvisible;
		}
		else
		{
			SetChatListVisibility(true);
		}
	}

	virtual bool IsChatMinimized() const override
	{
		return ChatMinimized;
	}

	virtual void SetTabVisibilityOverride(EVisibility InTabVisibilityOverride) override
	{
		TabVisibilityOverride = InTabVisibilityOverride;
	}

	virtual void ClearTabVisibilityOverride() override
	{
		TabVisibilityOverride.Reset();
	}

	virtual void SetEntryVisibilityOverride(EVisibility InEntryVisibilityOverride) override
	{
		EntryVisibilityOverride = InEntryVisibilityOverride;
	}

	virtual void ClearEntryVisibilityOverride() override
	{
		EntryVisibilityOverride.Reset();
	}

	virtual void SetBackgroundVisibilityOverride(EVisibility InBackgroundVisibilityOverride) override
	{
		BackgroundVisibilityOverride = InBackgroundVisibilityOverride;
	}

	virtual void ClearBackgroundVisibilityOverride() override
	{
		BackgroundVisibilityOverride.Reset();
	}

	virtual void SetChatListVisibilityOverride(EVisibility InChatVisibilityOverride) override
	{
		ChatListVisibilityOverride = InChatVisibilityOverride; 
	}

	virtual void ClearChatListVisibilityOverride() override
	{
		ChatListVisibilityOverride.Reset();
	}

	virtual void SetActiveTab(TWeakPtr<FChatViewModel> InActiveTab) override
	{
		ActiveTab = InActiveTab;
	}

	virtual void SendGameMessage(const FString& GameMessage) override
	{
		OnNetworkMessageSentEvent().Broadcast(GameMessage);
	}

	virtual bool ShouldAutoRelease() override
	{
		return AutoReleaseFocus;
	}

	virtual void SetAutoReleaseFocus(bool InAutoRelease) override
	{
		AutoReleaseFocus = InAutoRelease;
	}

	DECLARE_DERIVED_EVENT(FChatDisplayServiceImpl, IChatDisplayService::FChatListUpdated, FChatListUpdated);
	virtual FChatListUpdated& OnChatListUpdated() override
	{
		return ChatListUpdatedEvent;
	}

	DECLARE_DERIVED_EVENT(FChatDisplayServiceImpl, IChatDisplayService::FOnFriendsChatMessageCommitted, FOnFriendsChatMessageCommitted);
	virtual FOnFriendsChatMessageCommitted& OnChatMessageCommitted() override
	{
		return ChatMessageCommittedEvent;
	}

	DECLARE_DERIVED_EVENT(FChatDisplayServiceImpl, IChatDisplayService::FOnFriendsSendNetworkMessageEvent, FOnFriendsSendNetworkMessageEvent);
	virtual FOnFriendsSendNetworkMessageEvent& OnNetworkMessageSentEvent() override
	{
		return FriendsSendNetworkMessageEvent;
	}

	DECLARE_DERIVED_EVENT(FChatDisplayServiceImpl, IChatDisplayService::FOnFocusReleasedEvent, FOnFocusReleasedEvent);
	virtual FOnFocusReleasedEvent& OnFocuseReleasedEvent() override
	{
		return OnFocusReleasedEvent;
	}

	DECLARE_DERIVED_EVENT(FChatDisplayServiceImpl, IChatDisplayService::FChatListSetFocus, FChatListSetFocus);
	virtual FChatListSetFocus& OnChatListSetFocus() override
	{
		return ChatSetFocusEvent;
	}

private:

	void SetChatEntryVisibility(bool Visible)
	{
		if(Visible)
		{
			ChatEntryVisibility = EVisibility::Visible;
			EntryFadeDelay = EntryFadeInterval;
			if (IsChatMinimized())
			{
				ChatFadeDelay = ChatFadeInterval;
			}
		}
		else
		{
			ChatEntryVisibility = EVisibility::Hidden;
			OnFocuseReleasedEvent().Broadcast();
		}
	}

	void SetChatListVisibility(bool Visible)
	{
		if(Visible)
		{
			ChatListVisibility = IsChatMinimized() ? EVisibility::HitTestInvisible : EVisibility::Visible;
			ChatFadeDelay = ChatFadeInterval;
		}
		else
		{
			ChatListVisibility = EVisibility::Hidden;
		}
	}

	void HandleChatMessageReceived(TSharedRef< FFriendChatMessage > NewMessage)
	{
		if(!NewMessage->bIsFromSelf && ActiveTab.IsValid() && ActiveTab.Pin()->IsChannelSet(NewMessage->MessageType))
		{
			SetChatListVisibility(true);
		}
	}

	bool HandleTick(float DeltaTime)
	{
		if(IsFading())
		{
			// Made the fades independent and actually check the specific flag, i maintained the entry fades first ?			// If entry fade is enabled also made the time checks consistent.
			if (!FadeChatEntry || ChatEntryVisibility != EVisibility::Visible)
			{
				if (FadeChatList && ChatFadeDelay > 0)
				{
					ChatFadeDelay -= DeltaTime;
					if(ChatFadeDelay <= 0)
					{
						SetChatListVisibility(false);
						if (IsChatMinimizeEnabled() && IsChatAutoMinimizeEnabled())
						{
							CachedMinimize = ChatMinimized;
							ChatMinimized = true;
						}
					}
				}
			}
			if (FadeChatEntry && EntryFadeDelay > 0)
			{
				EntryFadeDelay -= DeltaTime;
				if(EntryFadeDelay <= 0)
				{
					SetChatEntryVisibility(false);
				}
			}
		}
		else if (IsChatMinimized())
		{
			if(ChatFadeDelay > 0)
			{
				ChatFadeDelay -= DeltaTime;
				if(ChatFadeDelay <= 0)
				{
					SetChatListVisibility(false);
				}
			}
		}
		return true;
	}

	void Initialize()
	{
		ChatService->OnChatMessageAdded().AddSP(this, &FChatDisplayServiceImpl::HandleChatMessageReceived);

		if(IsFading())
		{
			if (!TickDelegate.IsBound())
			{
				TickDelegate = FTickerDelegate::CreateSP(this, &FChatDisplayServiceImpl::HandleTick);
				TickerHandle = FTicker::GetCoreTicker().AddTicker(TickDelegate);
			}

			SetChatListVisibility(false);
		}
	}

	FChatDisplayServiceImpl(const TSharedRef<IChatCommunicationService>& InChatService, bool InChatMinimizeEnabled, bool InChatAutoMinimizeEnabled, bool InFadeChatList, bool InFadeChatEntry, float InListFadeTime, float InEntryFadeTime)
		: ChatService(InChatService)
		, FadeChatList(InFadeChatList)
		, FadeChatEntry(InFadeChatEntry)
		, ChatFadeInterval(InListFadeTime)
		, EntryFadeInterval(InEntryFadeTime)
		, ChatEntryVisibility(EVisibility::Visible)
		, ChatListVisibility(EVisibility::Visible)
		, IsChatActive(true)
		, ChatMinimizeEnabled(InChatMinimizeEnabled)
		, ChatAutoMinimizeEnabled(InChatAutoMinimizeEnabled)
		, ChatMinimized(false)
		, AutoReleaseFocus(false)
		, FadeBackgroundEnabled(true)
	{
	}

private:

	TSharedRef<IChatCommunicationService> ChatService;
	bool FadeChatList;
	bool FadeChatEntry;
	float ChatFadeDelay;
	float ChatFadeInterval;
	float EntryFadeDelay;
	float EntryFadeInterval;
	EVisibility ChatEntryVisibility;
	EVisibility ChatListVisibility;
	bool IsChatActive;
	bool ChatMinimizeEnabled;
	bool ChatAutoMinimizeEnabled;
	bool ChatMinimized;
	TOptional<bool> CachedMinimize;
	bool AutoReleaseFocus;
	bool FadeBackgroundEnabled;

	TOptional<EVisibility> TabVisibilityOverride;
	TOptional<EVisibility> EntryVisibilityOverride;
	TOptional<EVisibility> BackgroundVisibilityOverride;
	TOptional<EVisibility> ChatListVisibilityOverride;

	FChatListUpdated ChatListUpdatedEvent;
	FOnFriendsChatMessageCommitted ChatMessageCommittedEvent;
	FOnFriendsSendNetworkMessageEvent FriendsSendNetworkMessageEvent;
	FOnFocusReleasedEvent OnFocusReleasedEvent;

	FChatListSetFocus ChatSetFocusEvent;

	TWeakPtr<FChatViewModel> ActiveTab;

	// Delegate for which function we should use when we tick
	FTickerDelegate TickDelegate;
	// Handler for the tick function when active
	FDelegateHandle TickerHandle;

	friend FChatDisplayServiceFactory;
};

TSharedRef< FChatDisplayService > FChatDisplayServiceFactory::Create(const TSharedRef<IChatCommunicationService>& ChatService, bool ChatMinimizeEnabled, bool ChatAutoMinimizeEnabled, bool FadeChatList, bool FadeChatEntry, float ListFadeTime, float EntryFadeTime)
{
	TSharedRef< FChatDisplayServiceImpl > DisplayService = MakeShareable(new FChatDisplayServiceImpl(ChatService, ChatMinimizeEnabled, ChatAutoMinimizeEnabled, FadeChatList, FadeChatEntry, ListFadeTime, EntryFadeTime));
	DisplayService->Initialize();
	return DisplayService;
}