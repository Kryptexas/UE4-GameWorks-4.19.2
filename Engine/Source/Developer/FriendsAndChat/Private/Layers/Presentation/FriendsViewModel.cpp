// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "FriendsViewModel.h"
#include "FriendViewModel.h"
#include "FriendsStatusViewModel.h"
#include "FriendListViewModel.h"
#include "FriendsUserViewModel.h"
#include "ClanCollectionViewModel.h"
#include "ClanRepository.h"
#include "IFriendList.h"
#include "FriendsNavigationService.h"

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

	virtual TSharedRef< FFriendListViewModel > GetFriendListViewModel(EFriendsDisplayLists::Type ListType) override
	{
		return FFriendListViewModelFactory::Create(FriendsListFactory->Create(ListType), ListType);
	}

	virtual TSharedRef< FClanCollectionViewModel > GetClanCollectionViewModel() override
	{
		return FClanCollectionViewModelFactory::Create(ClanRepository, FriendsListFactory, FriendsAndChatManager.Pin().ToSharedRef());
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
			TSharedPtr<FFriendsAndChatManager> FriendsAndChatManagerPtr = FriendsAndChatManager.Pin();
			return FriendsAndChatManagerPtr->IsInGlobalChat() && FriendsAndChatManagerPtr->IsInLauncher() ? EVisibility::Visible : EVisibility::Collapsed;
		}
		return EVisibility::Collapsed;
	}

	virtual void DisplayGlobalChatWindow() const override
	{
		if (FriendsAndChatManager.IsValid())
		{
			FriendsAndChatManager.Pin()->OpenGlobalChat();
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
		NavigationService->OnChatViewChanged().RemoveAll(this);
	}

	FFriendsViewModelImpl(
		const TSharedRef<FFriendsAndChatManager>& InFriendsAndChatManager,
		const TSharedRef<IClanRepository>& InClanRepository,
		const TSharedRef<IFriendListFactory>& InFriendsListFactory,
		const TSharedRef<FFriendsNavigationService>& InNavigationService
		)
		: FriendsAndChatManager(InFriendsAndChatManager)
		, ClanRepository(InClanRepository)
		, FriendsListFactory(InFriendsListFactory)
		, NavigationService(InNavigationService)
		, bIsPerformingAction(false)
	{
	}

private:
	TWeakPtr<FFriendsAndChatManager> FriendsAndChatManager;
	TSharedRef<IClanRepository> ClanRepository;
	TSharedRef<IFriendListFactory> FriendsListFactory;
	TSharedRef<FFriendsNavigationService> NavigationService;
	bool bIsPerformingAction;

private:
	friend FFriendsViewModelFactory;
};

TSharedRef< FFriendsViewModel > FFriendsViewModelFactory::Create(
	const TSharedRef<FFriendsAndChatManager>& FriendsAndChatManager,
	const TSharedRef<IClanRepository>& ClanRepository,
	const TSharedRef<IFriendListFactory>& FriendsListFactory,
	const TSharedRef<FFriendsNavigationService>& NavigationService
	)
{
	TSharedRef< FFriendsViewModelImpl > ViewModel(new FFriendsViewModelImpl(FriendsAndChatManager, ClanRepository, FriendsListFactory, NavigationService));
	return ViewModel;
}
