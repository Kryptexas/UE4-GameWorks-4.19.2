// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "FriendsChatMarkupService.h"
#include "ChatTipViewModel.h"

class FChatTipViewModelImpl
	: public FChatTipViewModel
{
public:

	~FChatTipViewModelImpl()
	{
		MarkupService->OnInputUpdated().RemoveAll(this);
	}

	DECLARE_DERIVED_EVENT(FChatTipViewModelImpl, FChatTipViewModel::FChatTipAvailable, FChatTipAvailable)
	virtual FChatTipAvailable& OnChatTipAvailable() override
	{
		return ChatTipAvailableEvent;
	}

	virtual TArray<TSharedRef<IChatTip> >& GetChatTips() override
	{
		return MarkupService->GetChatTips();
	}

	virtual TSharedPtr<IChatTip> GetActiveTip() override
	{
		return MarkupService->GetActiveTip();
	}

private:

	void HandleChatInputChanged()
	{
		OnChatTipAvailable().Broadcast();
	}

	void Initialize()
	{
		MarkupService->OnInputUpdated().AddSP(this, &FChatTipViewModelImpl::HandleChatInputChanged);
	}

	FChatTipViewModelImpl(const TSharedRef<FFriendsChatMarkupService>& InMarkupService)
		: MarkupService(InMarkupService)
	{
	}

private:

	TSharedRef<FFriendsChatMarkupService> MarkupService;
	FChatTipAvailable ChatTipAvailableEvent;
	friend FChatTipViewModelFactory;
};

TSharedRef< FChatTipViewModel > FChatTipViewModelFactory::Create(const TSharedRef<FFriendsChatMarkupService>& MarkupService)
{
	TSharedRef< FChatTipViewModelImpl > ViewModel(new FChatTipViewModelImpl(MarkupService));
	ViewModel->Initialize();
	return ViewModel;
}