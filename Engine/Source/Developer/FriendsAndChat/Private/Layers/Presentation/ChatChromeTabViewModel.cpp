// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "ChatChromeTabViewModel.h"
#include "ChatViewModel.h"

class FChatChromeTabViewModelImpl
	: public FChatChromeTabViewModel
{
public:

	FChatChromeTabViewModelImpl(const TSharedRef<FChatViewModel>& InChatViewModel)
		: ChatViewModel(InChatViewModel)
	{
	}

	bool IsTabVisible() const override
	{
		if (!ChatViewModel.IsValid())
		{
			return false;
		}

		if(ChatViewModel->GetChatChannelType() == EChatMessageType::Whisper)
		{
			return (ChatViewModel->IsWhisperFriendSet() || ChatViewModel->GetMessageCount() > 0);
		}

		return true;
	}

	const FText GetTabText() const override
	{
		return ChatViewModel->GetChatGroupText(false);
	}

	virtual const EChatMessageType::Type GetTabID() const override
	{
		return ChatViewModel->GetChatChannelType();
	}

	const FSlateBrush* GetTabImage() const override
	{
		return nullptr;
	}

	const TSharedPtr<FChatViewModel> GetChatViewModel() const override
	{
		return ChatViewModel;
	}

	DECLARE_DERIVED_EVENT(FChatChromeTabViewModelImpl, IChatTabViewModel::FChatTabVisibilityChangedEvent, FChatTabVisibilityChangedEvent)
	virtual FChatTabVisibilityChangedEvent& OnTabVisibilityChanged() override
	{
		return TabVisibilityChangedEvent;
	}

	TSharedPtr<FChatViewModel> ChatViewModel;

private:

	FChatTabVisibilityChangedEvent TabVisibilityChangedEvent;


	friend FChatChromeTabViewModelFactory;
};

TSharedRef< FChatChromeTabViewModel > FChatChromeTabViewModelFactory::Create(const TSharedRef<FChatViewModel>& ChatViewModel)
{
	TSharedRef< FChatChromeTabViewModelImpl > ViewModel(new FChatChromeTabViewModelImpl(ChatViewModel));
	return ViewModel;
}