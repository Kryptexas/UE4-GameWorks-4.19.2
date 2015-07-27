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

#include "FriendsService.h"
#include "MessageService.h"
#include "GameAndPartyService.h"

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
		return FFriendsUserViewModelFactory::Create(FriendsService);
	}

	virtual TSharedRef< FFriendsStatusViewModel > GetStatusViewModel() override
	{
		return FFriendsStatusViewModelFactory::Create(FriendsService);
	}

	virtual TSharedRef< FFriendListViewModel > GetFriendListViewModel(EFriendsDisplayLists::Type ListType) override
	{
		return FFriendListViewModelFactory::Create(FriendsListFactory->Create(ListType), ListType);
	}

	virtual TSharedRef< FClanCollectionViewModel > GetClanCollectionViewModel() override
	{
		return FClanCollectionViewModelFactory::Create(ClanRepository, FriendsListFactory);
	}

	virtual void RequestFriend(const FText& FriendName) const override
	{
		FriendsService->RequestFriend(FriendName);
	}

	virtual EVisibility GetGlobalChatButtonVisibility() const override
	{
		return MessageService->IsInGlobalChat() && GameAndPartyService->IsInLauncher() ? EVisibility::Visible : EVisibility::Collapsed;
	}

	virtual void DisplayGlobalChatWindow() const override
	{
		if (MessageService->IsInGlobalChat())
		{
			MessageService->OpenGlobalChat();
			NavigationService->ChangeViewChannel(EChatMessageType::Global);
		}
	}

	virtual const FString GetName() const override
	{
		FString Nickname = FriendsService->GetUserNickname();
		return Nickname;
	}

	~FFriendsViewModelImpl()
	{
		Uninitialize();
	}

private:

	void Uninitialize()
	{
		NavigationService->OnChatViewChanged().RemoveAll(this);
	}

	FFriendsViewModelImpl(
		const TSharedRef<FFriendsService>& InFriendsService,
		const TSharedRef<FGameAndPartyService>& InGameAndPartyService,
		const TSharedRef<FMessageService>& InMessageService,
		const TSharedRef<IClanRepository>& InClanRepository,
		const TSharedRef<IFriendListFactory>& InFriendsListFactory,
		const TSharedRef<FFriendsNavigationService>& InNavigationService
		)
		: FriendsService(InFriendsService)
		, GameAndPartyService(InGameAndPartyService)
		, MessageService(InMessageService)
		, ClanRepository(InClanRepository)
		, FriendsListFactory(InFriendsListFactory)
		, NavigationService(InNavigationService)
		, bIsPerformingAction(false)
	{
	}

private:
	TSharedRef<FFriendsService> FriendsService;
	TSharedRef<FGameAndPartyService> GameAndPartyService;
	TSharedRef<FMessageService> MessageService;
	TSharedRef<IClanRepository> ClanRepository;
	TSharedRef<IFriendListFactory> FriendsListFactory;
	TSharedRef<FFriendsNavigationService> NavigationService;
	bool bIsPerformingAction;

private:
	friend FFriendsViewModelFactory;
};

TSharedRef< FFriendsViewModel > FFriendsViewModelFactory::Create(
	const TSharedRef<FFriendsService>& FriendsService,
	const TSharedRef<FGameAndPartyService>& GameAndPartyService,
	const TSharedRef<FMessageService>& MessageService,
	const TSharedRef<IClanRepository>& ClanRepository,
	const TSharedRef<IFriendListFactory>& FriendsListFactory,
	const TSharedRef<FFriendsNavigationService>& NavigationService
	)
{
	TSharedRef< FFriendsViewModelImpl > ViewModel(new FFriendsViewModelImpl(FriendsService, GameAndPartyService, MessageService, ClanRepository, FriendsListFactory, NavigationService));
	return ViewModel;
}
