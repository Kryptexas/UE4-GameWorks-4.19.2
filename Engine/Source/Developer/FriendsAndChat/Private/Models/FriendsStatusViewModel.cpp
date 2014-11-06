// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "FriendsStatusViewModel.h"

class FFriendsStatusViewModelImpl
	: public FFriendsStatusViewModel
{
public:
	virtual bool GetOnlineStatus() const override
	{
		return FriendsAndChatManager.Pin()->GetUserIsOnline();
	}

	virtual void SetOnlineStatus(const bool bOnlineStatus) override
	{
		FriendsAndChatManager.Pin()->SetUserIsOnline(bOnlineStatus);
	}

	virtual TArray<TSharedRef<FText> > GetStatusList() const override
	{
		return OnlineStateArray;
	}

	virtual FText GetStatusText() const override
	{
		return FriendsAndChatManager.Pin()->GetUserIsOnline() ? FText::FromString("Online") : FText::FromString("Away");
	}

private:
	FFriendsStatusViewModelImpl(
		const TSharedRef<FFriendsAndChatManager>& FriendsAndChatManager
		)
		: FriendsAndChatManager(FriendsAndChatManager)
	{
		OnlineStateArray.Add(MakeShareable(new FText(NSLOCTEXT("OnlineState", "OnlineState_Online", "Online"))));
		OnlineStateArray.Add(MakeShareable(new FText(NSLOCTEXT("OnlineState", "OnlineState_Away", "Away"))));
	}

private:
	TWeakPtr<FFriendsAndChatManager> FriendsAndChatManager;
	TArray<TSharedRef<FText> > OnlineStateArray;

private:
	friend FFriendsStatusViewModelFactory;
};

TSharedRef< FFriendsStatusViewModel > FFriendsStatusViewModelFactory::Create(
	const TSharedRef<FFriendsAndChatManager>& FriendsAndChatManager
	)
{
	TSharedRef< FFriendsStatusViewModelImpl > ViewModel(new FFriendsStatusViewModelImpl(FriendsAndChatManager));
	return ViewModel;
}