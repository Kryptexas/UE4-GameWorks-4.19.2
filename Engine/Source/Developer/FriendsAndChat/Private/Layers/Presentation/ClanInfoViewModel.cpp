// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "ClanInfoViewModel.h"
#include "ClanInfo.h"
#include "ClanMemberList.h"
#include "FriendListViewModel.h"
#include "FriendsNavigationService.h"

class FClanInfoViewModelImpl
	: public FClanInfoViewModel
{
public:

	virtual FText GetClanTitle() override
	{
		return ClanInfo->GetTitle();
	}

	virtual int32 GetMemberCount() override
	{
		return ClanInfo->GetMemberList().Num();
	}

	virtual TSharedRef< FFriendListViewModel > GetFriendListViewModel() override
	{
		return FFriendListViewModelFactory::Create(FriendsListFactory->Create(ClanInfo), EFriendsDisplayLists::ClanMemberDisplay);
	}

	virtual FText GetListCountText() const override
	{
		return FText::AsNumber(ClanInfo->GetMemberList().Num());
	}

	virtual FText GetClanBrushName() const override
	{
		return ClanInfo->GetClanBrushName();
	}

	virtual void EnumerateActions(TArray<EClanActionType::Type>& OUTActions) override
	{
		// TODO Antony.Carter - Add enumerations on a context specific basis
		OUTActions.Add(EClanActionType::AcceptClanRequest);
		OUTActions.Add(EClanActionType::LeaveClan);
		OUTActions.Add(EClanActionType::CancelClanRequest);
		OUTActions.Add(EClanActionType::IgnoreClanRequest);
		OUTActions.Add(EClanActionType::SetPrimaryClan);
	}

	DECLARE_DERIVED_EVENT(FClanInfoViewModelImpl, FClanInfoViewModel::FOpenClanDetails, FOpenClanDetails);
	virtual FOpenClanDetails& OpenClanDetails() override
	{
		return OpenClanDetailsEvent;
	}

private:
	void Initialize()
	{

	}

private:
	FClanInfoViewModelImpl(
	const TSharedRef<IClanInfo>& InClanInfo,
	const TSharedRef<IFriendListFactory>& InFriendsListFactory,
	const TSharedRef<FFriendsAndChatManager>& InFriendsAndChatManager)
		: ClanInfo(InClanInfo)
		, FriendsListFactory(InFriendsListFactory)
		, FriendsAndChatManager(InFriendsAndChatManager)
	{
	}

	FOpenClanDetails OpenClanDetailsEvent;
	TSharedRef<IClanInfo> ClanInfo;
	TSharedRef<IFriendListFactory> FriendsListFactory;
	TWeakPtr<FFriendsAndChatManager> FriendsAndChatManager;
	friend FClanInfoViewModelFactory;
};

TSharedRef< FClanInfoViewModel > FClanInfoViewModelFactory::Create(
	const TSharedRef<IClanInfo>& ClanInfo,
	const TSharedRef<IFriendListFactory>& FriendsListFactory,
	const TSharedRef<FFriendsAndChatManager>& FriendsAndChatManager
	)
{
	TSharedRef< FClanInfoViewModelImpl > ViewModel(new FClanInfoViewModelImpl(ClanInfo, FriendsListFactory, FriendsAndChatManager));
	ViewModel->Initialize();
	return ViewModel;
}