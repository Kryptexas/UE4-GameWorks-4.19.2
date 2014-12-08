// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "FriendsUserSettingsViewModel.h"

// ToDO: Read and write these values from the respective calling app.

class FFriendsUserSettingsViewModelImpl
	: public FFriendsUserSettingsViewModel
{
public:

private:
	FFriendsUserSettingsViewModelImpl(
		const TSharedRef<FFriendsAndChatManager>& FriendsAndChatManager
		)
		: FriendsAndChatManager(FriendsAndChatManager)
		, bShowNotifications(true)
	{
	}

	virtual void HandleCheckboxStateChanged(ESlateCheckBoxState::Type NewState, EUserSettngsType::Type OptionType) override
	{
		switch(OptionType)
		{
			case EUserSettngsType::ShowNotifications:
			{
				bShowNotifications = NewState == ESlateCheckBoxState::Checked ? true : false;
			}
			break;
		};
	}

	virtual void EnumerateUserSettings(TArray<EUserSettngsType::Type>& UserSettings) override
	{
		UserSettings.Add(EUserSettngsType::ShowNotifications);
	}

	virtual ESlateCheckBoxState::Type GetOptionCheckState(EUserSettngsType::Type Option) const override
	{
		switch(Option)
		{
			case EUserSettngsType::ShowNotifications:
			{
				return bShowNotifications ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked; break;
			}
			default:
			return ESlateCheckBoxState::Undetermined;
		};
	}

private:
	TWeakPtr<FFriendsAndChatManager> FriendsAndChatManager;
	TArray<TSharedRef<FText> > OnlineStateArray;
	bool bShowNotifications;

private:
	friend FFriendsUserSettingsViewModelFactory;
};

TSharedRef< FFriendsUserSettingsViewModel > FFriendsUserSettingsViewModelFactory::Create(
	const TSharedRef<FFriendsAndChatManager>& FriendsAndChatManager
	)
{
	TSharedRef< FFriendsUserSettingsViewModelImpl > ViewModel(new FFriendsUserSettingsViewModelImpl(FriendsAndChatManager));
	return ViewModel;
}