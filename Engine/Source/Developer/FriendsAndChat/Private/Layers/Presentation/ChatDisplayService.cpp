// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "ChatDisplayService.h"

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

		if(IsFading())
		{
			SetChatEntryVisibility(false);
			SetChatListVisibility(true);
		}
	}

	virtual EVisibility GetEntryBarVisibility() const override
	{
		return ChatEntryVisibility;
	}

	virtual EVisibility GetChatListVisibility() const override
	{
		return ChatListVisibility;
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
		}
	}

	void SetChatListVisibility(bool Visible)
	{
		if(Visible)
		{
			ChatListVisibility = EVisibility::Visible;
			ChatFadeDelay = ChatFadeInterval;
		}
		else
		{
			ChatListVisibility = EVisibility::Hidden;
		}
	}

	void HandleChatMessageReceived(EChatMessageType::Type ChatType, TSharedPtr<IFriendItem> FriendItem)
	{
		SetChatListVisibility(true);
	}

	bool HandleTick(float DeltaTime)
	{
		if(IsFading())
		{
			if(ChatEntryVisibility != EVisibility::Visible)
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
			else
			{
				EntryFadeDelay -= DeltaTime;
				if(EntryFadeDelay <= 0)
				{
					SetChatEntryVisibility(false);
				}
			}
		}
		return true;
	}

	void Initialize()
	{
		FriendsAndChatManager->OnChatMessageRecieved().AddSP(this, &FChatDisplayServiceImpl::HandleChatMessageReceived);

		if(IsFading())
		{
			if (!TickDelegate.IsBound())
			{
				TickDelegate = FTickerDelegate::CreateSP(this, &FChatDisplayServiceImpl::HandleTick);
			}
			TickerHandle = FTicker::GetCoreTicker().AddTicker(TickDelegate);
			SetChatListVisibility(false);
		}
	}

	FChatDisplayServiceImpl(const TSharedRef<FFriendsAndChatManager>& InFriendsAndChatManager, bool InFadeChatList, bool InFadeChatEntry, float InListFadeTime, float InEntryFadeTime)
		: FriendsAndChatManager(InFriendsAndChatManager)
		, FadeChatList(InFadeChatList)
		, FadeChatEntry(InFadeChatEntry)
		, ChatFadeInterval(InListFadeTime)
		, EntryFadeInterval(InEntryFadeTime)
		, ChatEntryVisibility(EVisibility::Visible)
		, ChatListVisibility(EVisibility::Visible)
		, IsChatActive(true)
	{
	}

private:

	TSharedRef<FFriendsAndChatManager> FriendsAndChatManager;
	bool FadeChatList;
	bool FadeChatEntry;
	float ChatFadeDelay;
	float ChatFadeInterval;
	float EntryFadeDelay;
	float EntryFadeInterval;
	EVisibility ChatEntryVisibility;
	EVisibility ChatListVisibility;
	bool IsChatActive;
	
	FChatListUpdated ChatListUpdatedEvent;
	FOnFriendsChatMessageCommitted ChatMessageCommittedEvent;
	FOnFriendsSendNetworkMessageEvent FriendsSendNetworkMessageEvent;
	FChatListSetFocus ChatSetFocusEvent;

	// Delegate for which function we should use when we tick
	FTickerDelegate TickDelegate;
	// Handler for the tick function when active
	FDelegateHandle TickerHandle;

	friend FChatDisplayServiceFactory;
};

TSharedRef< FChatDisplayService > FChatDisplayServiceFactory::Create(const TSharedRef<FFriendsAndChatManager>& FriendsAndChatManager, bool FadeChatList, bool FadeChatEntry, float ListFadeTime, float EntryFadeTime)
{
	TSharedRef< FChatDisplayServiceImpl > DisplayService = MakeShareable(new FChatDisplayServiceImpl(FriendsAndChatManager, FadeChatList, FadeChatEntry, ListFadeTime, EntryFadeTime));
	DisplayService->Initialize();
	return DisplayService;
}