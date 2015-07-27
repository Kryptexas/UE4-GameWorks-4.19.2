// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "FriendsUserViewModel.h"

#include "FriendsService.h"

class FFriendsUserViewModelImpl
	: public FFriendsUserViewModel
{
public:

	virtual ~FFriendsUserViewModelImpl() override
	{
		Uninitialize();
	}

	virtual const EOnlinePresenceState::Type GetOnlineStatus() const override
	{
		return FriendsService->GetOnlineStatus();
	}

	virtual const FString GetClientId() const override
	{
		FString ClientId = FriendsService->GetUserClientId();
		return ClientId;
	}

	virtual const FString GetName() const override
	{
		FString Nickname = FriendsService->GetUserNickname();
		return Nickname;
	}

private:

	void Uninitialize()
	{
	}

	FFriendsUserViewModelImpl(const TSharedRef<FFriendsService>& InFriendsService)
	: FriendsService(InFriendsService)
	{
	}

private:
	TSharedRef<FFriendsService> FriendsService;

private:
	friend FFriendsUserViewModelFactory;
};

TSharedRef< FFriendsUserViewModel > FFriendsUserViewModelFactory::Create(const TSharedRef<FFriendsService>& FriendsService)
{
	TSharedRef< FFriendsUserViewModelImpl > ViewModel(new FFriendsUserViewModelImpl(FriendsService));
	return ViewModel;
}