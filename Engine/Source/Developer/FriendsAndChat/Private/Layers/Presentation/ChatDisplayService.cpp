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
		OnChatListSetFocus().Broadcast();
	}

	virtual void ChatEntered() override
	{
		SetChatEntryVisibility(true);
	}

	virtual void MessageCommitted() override
	{
		OnChatMessageCommitted().Broadcast();

		// Here i needed to make the visibility changes based on the actual flag for that section
		if (FadeChatEntry)
		{
			SetChatEntryVisibility(false);
		}
		// However! If the we don't set chat entry invisible i still want to brodcast a Focus Relase
		else
		{
			OnFocuseReleasedEvent().Broadcast();
		}
		// Seems like i just want to do this all the time, i just sent a message why not show the text.
		SetChatListVisibility(true);
		if(ChatMinimized)
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

		return ChatMinimized ? EVisibility::Collapsed : ChatEntryVisibility;
	}

	virtual EVisibility GetChatListVisibility() const override
	{
		if(ChatListVisibilityOverride.IsSet())
		{
			return ChatListVisibilityOverride.GetValue();
		}
		return ChatListVisibility;
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

	virtual void ToggleChatMinimized() override
	{
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
			ChatListVisibility = ChatMinimized ? EVisibility::HitTestInvisible : EVisibility::Visible;
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
						ChatMinimized = true;
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
		else if(ChatMinimized)
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

	FChatDisplayServiceImpl(const TSharedRef<IChatCommunicationService>& InChatService, bool InFadeChatList, bool InFadeChatEntry, float InListFadeTime, float InEntryFadeTime)
		: ChatService(InChatService)
		, FadeChatList(InFadeChatList)
		, FadeChatEntry(InFadeChatEntry)
		, ChatFadeInterval(InListFadeTime)
		, EntryFadeInterval(InEntryFadeTime)
		, ChatEntryVisibility(EVisibility::Visible)
		, ChatListVisibility(EVisibility::Visible)
		, IsChatActive(true)
		, ChatMinimized(false)
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
	bool ChatMinimized;

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

TSharedRef< FChatDisplayService > FChatDisplayServiceFactory::Create(const TSharedRef<IChatCommunicationService>& ChatService, bool FadeChatList, bool FadeChatEntry, float ListFadeTime, float EntryFadeTime)
{
	TSharedRef< FChatDisplayServiceImpl > DisplayService = MakeShareable(new FChatDisplayServiceImpl(ChatService, FadeChatList, FadeChatEntry, ListFadeTime, EntryFadeTime));
	DisplayService->Initialize();
	return DisplayService;
}