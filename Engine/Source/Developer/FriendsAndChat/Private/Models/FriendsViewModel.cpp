// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "FriendsViewModel.h"
#include "FriendViewModel.h"
#include "FriendsStatusViewModel.h"
#include "FriendsUserSettingsViewModel.h"
#include "FriendListViewModel.h"
#include "FriendsUserViewModel.h"

class FFriendsViewModelImpl
	: public FFriendsViewModel
{
public:

	virtual bool IsPerformingAction() const override
	{
		return bIsPerformingAction;
	}

	virtual void PerformAction() override
	{
		bIsPerformingAction = !bIsPerformingAction;
	}

	virtual TSharedRef< class FFriendsUserViewModel > GetUserViewModel() override
	{
		return FFriendsUserViewModelFactory::Create(FriendsAndChatManager.Pin().ToSharedRef());
	}

	virtual TSharedRef< FFriendsStatusViewModel > GetStatusViewModel() override
	{
		return FFriendsStatusViewModelFactory::Create(FriendsAndChatManager.Pin().ToSharedRef());
	}

	virtual TSharedRef< FFriendsUserSettingsViewModel > GetUserSettingsViewModel() override
	{
		return FFriendsUserSettingsViewModelFactory::Create(FriendsAndChatManager.Pin().ToSharedRef());
	}

	virtual TSharedRef< FFriendListViewModel > GetFriendListViewModel(EFriendsDisplayLists::Type ListType) override
	{
		return FFriendListViewModelFactory::Create(SharedThis(this), ListType);
	}

	virtual void RequestFriend(const FText& FriendName) const override
	{
		if (FriendsAndChatManager.IsValid())
		{
			FriendsAndChatManager.Pin()->RequestFriend(FriendName);
		}
	}

	virtual EVisibility GetGlobalChatButtonVisibility() const override
	{
		if (FriendsAndChatManager.IsValid())
		{
			return FriendsAndChatManager.Pin()->HasGlobalChatPermission() ? EVisibility::Visible : EVisibility::Collapsed;
		}
		return EVisibility::Collapsed;
	}

	virtual void JoinGlobalChat() const override
	{
		if (FriendsAndChatManager.IsValid())
		{
			FriendsAndChatManager.Pin()->JoinPublicChatRoom(TEXT("Fortnite"));
		}
	}

	virtual const FString GetName() const override
	{
		FString Nickname;
		TSharedPtr<FFriendsAndChatManager> ManagerPinned = FriendsAndChatManager.Pin();
		if (ManagerPinned.IsValid())
		{
			Nickname = ManagerPinned->GetUserNickname();
		}
		return Nickname;
	}

	~FFriendsViewModelImpl()
	{
		Uninitialize();
	}

private:

	void Uninitialize()
	{
		if( FriendsAndChatManager.IsValid())
		{
			FriendsAndChatManager.Reset();
		}
	}

	FFriendsViewModelImpl(
		const TSharedRef<FFriendsAndChatManager>& FriendsAndChatManager
		)
		: FriendsAndChatManager(FriendsAndChatManager)
		, bIsPerformingAction(false)
	{
	}

private:
	TWeakPtr<FFriendsAndChatManager> FriendsAndChatManager;
	bool bIsPerformingAction;

private:
	friend FFriendsViewModelFactory;
};

TSharedRef< FFriendsViewModel > FFriendsViewModelFactory::Create(
	const TSharedRef<FFriendsAndChatManager>& FriendsAndChatManager
	)
{
	TSharedRef< FFriendsViewModelImpl > ViewModel(new FFriendsViewModelImpl(FriendsAndChatManager));
	return ViewModel;
}
