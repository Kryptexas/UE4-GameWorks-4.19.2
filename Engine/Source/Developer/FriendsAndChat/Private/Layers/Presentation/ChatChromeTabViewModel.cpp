// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "ChatChromeTabViewModel.h"
#include "ChatViewModel.h"
#include "ChatDisplayService.h"

class FChatChromeTabViewModelImpl
	: public FChatChromeTabViewModel
{
public:

	FChatChromeTabViewModelImpl(const TSharedRef<FChatViewModel>& InChatViewModel)
		: ChatViewModel(InChatViewModel)
	{
	}

	virtual bool IsTabVisible() const override
	{
		if (!ChatViewModel.IsValid())
		{
			return false;
		}

		if (ChatViewModel->GetDefaultChannelType() == EChatMessageType::Party)
		{
			return ChatViewModel->IsInPartyChat() ||  ChatViewModel->IsInGameChat();
		}

		if (ChatViewModel->GetDefaultChannelType() == EChatMessageType::Whisper)
		{
			return (ChatViewModel->IsWhisperFriendSet() || ChatViewModel->GetMessageCount() > 0 || ChatViewModel->IsOverrideDisplaySet());
		}

		return true;
	}

	virtual const FText GetTabText() const override
	{
		return ChatViewModel->GetChatGroupText(false);
	}

	virtual FText GetMessageNotificationsText() const override
	{
		int32 Messages = ChatViewModel->GetUnreadChannelMessageCount();
		if (Messages < 10)
		{
			return FText::AsNumber(Messages);
		}
		const FText Count = FText::AsNumber(9);
		return FText::Format(NSLOCTEXT("","PlusNumber","{0}+"), Count);
	}

	virtual const EChatMessageType::Type GetTabID() const override
	{
		return ChatViewModel->GetDefaultChannelType();
	}

	virtual const FSlateBrush* GetTabImage() const override
	{
		return nullptr;
	}

	virtual const TSharedPtr<FChatViewModel> GetChatViewModel() const override
	{
		return ChatViewModel;
	}

	virtual TSharedRef<IChatTabViewModel> Clone(TSharedRef<IChatDisplayService> ChatDisplayService,  TArray<TSharedRef<ICustomSlashCommand> >* CustomSlashCommands) override
	{
		TSharedRef< FChatChromeTabViewModelImpl > ViewModel(new FChatChromeTabViewModelImpl(ChatViewModel->Clone(ChatDisplayService, CustomSlashCommands)));
		return ViewModel;
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