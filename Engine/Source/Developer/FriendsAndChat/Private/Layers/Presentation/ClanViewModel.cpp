// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "ClanViewModel.h"
#include "ClanInfoViewModel.h"
#include "ClanRepository.h"
#include "ClanInfo.h"
#include "IFriendList.h"

class FClanViewModelImpl
	: public FClanViewModel
{
public:

	virtual FText GetClanTitle() override
	{
		if(ClanList.Num() > 0)
		{
			return ClanList[0]->GetTitle();
		}
		return FText::GetEmpty();
	}

	virtual TSharedPtr<FClanInfoViewModel> GetClanInfoViewModel() override
	{
		if(ClanList.Num() > 0)
		{
			return FClanInfoViewModelFactory::Create(ClanList[0], FriendsListFactory, FriendsAndChatManager.Pin().ToSharedRef());
		}
		return nullptr;
	}


	virtual void EnumerateJoinedClans(TArray<TSharedRef<IClanInfo>>& OUTClanList) override
	{
		for (const auto& ClanItem : ClanList)
		{
			OUTClanList.Add(ClanItem);
		}
	}

private:
	void Initialize()
	{
		ClanRepository->EnumerateClanList(ClanList);
	}

private:
	FClanViewModelImpl(
	const TSharedRef<IClanRepository>& InClanRepository,
	const TSharedRef<IFriendListFactory>& InFriendsListFactory,
	const TSharedRef<FFriendsAndChatManager>& InFriendsAndChatManager)
		: ClanRepository(InClanRepository)
		, FriendsListFactory(InFriendsListFactory)
		, FriendsAndChatManager(InFriendsAndChatManager)
	{
	}

	TSharedRef<IClanRepository> ClanRepository;
	TArray<TSharedRef<IClanInfo>> ClanList;
	TSharedRef<IFriendListFactory> FriendsListFactory;
	TWeakPtr<FFriendsAndChatManager> FriendsAndChatManager;

	friend FClanViewModelFactory;
};

TSharedRef< FClanViewModel > FClanViewModelFactory::Create(
	const TSharedRef<IClanRepository>& ClanRepository,
	const TSharedRef<IFriendListFactory>& FriendsListFactory,
	const TSharedRef<FFriendsAndChatManager>& FriendsAndChatManager
	)
{
	TSharedRef< FClanViewModelImpl > ViewModel(new FClanViewModelImpl(ClanRepository, FriendsListFactory, FriendsAndChatManager));
	ViewModel->Initialize();
	return ViewModel;
}