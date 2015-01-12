// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "SFriendsStatus.h"
#include "SFriendsAndChatCombo.h"
#include "FriendsStatusViewModel.h"

#define LOCTEXT_NAMESPACE "SFriendsStatus"

namespace FriendsStatusImplDefs
{
	const static FName OnlineTag("Online");
	const static FName AwayTag("Away");
}

/**
 * Declares the Friends Status display widget
*/
class SFriendsStatusImpl : public SFriendsStatus
{
public:

	void Construct(const FArguments& InArgs, const TSharedRef<FFriendsStatusViewModel>& ViewModel)
	{
		FriendStyle = *InArgs._FriendStyle;
		this->ViewModel = ViewModel;

		FFriendsStatusViewModel* ViewModelPtr = &ViewModel.Get();

		SUserWidget::Construct(SUserWidget::FArguments()
		[
			SNew(SFriendsAndChatCombo)
			.FriendStyle(&FriendStyle)
			.ButtonText(ViewModelPtr, &FFriendsStatusViewModel::GetStatusText)
			.DropdownItems
			(
				SFriendsAndChatCombo::FItemsArray()
				+ SFriendsAndChatCombo::FItemData(FText::FromString(FriendsStatusImplDefs::OnlineTag.ToString()), FriendsStatusImplDefs::OnlineTag, true)
				+ SFriendsAndChatCombo::FItemData(FText::FromString(FriendsStatusImplDefs::AwayTag.ToString()), FriendsStatusImplDefs::AwayTag, true)
			)
			.bSetButtonTextToSelectedItem(true)
			.bAutoCloseWhenClicked(true)
			.ContentWidth(FriendStyle.StatusButtonSize.X)
			.OnDropdownItemClicked(this, &SFriendsStatusImpl::HandleStatusChanged)
		]);
	}

private:
	void HandleStatusChanged(FName ItemTag)
	{
		EOnlinePresenceState::Type OnlineState = EOnlinePresenceState::Offline;

		if (ItemTag == FriendsStatusImplDefs::OnlineTag)
		{
			OnlineState = EOnlinePresenceState::Online;
		}
		else if (ItemTag == FriendsStatusImplDefs::AwayTag)
		{
			OnlineState = EOnlinePresenceState::Away;
		}

		ViewModel->SetOnlineStatus(OnlineState);
	}


	const FSlateBrush* GetStatusBrush() const
	{
		switch (ViewModel->GetOnlineStatus())
		{	
		case EOnlinePresenceState::Away:
		case EOnlinePresenceState::ExtendedAway:
			return &FriendStyle.AwayBrush;
		case EOnlinePresenceState::Chat:
		case EOnlinePresenceState::DoNotDisturb:
		case EOnlinePresenceState::Online:
			return &FriendStyle.OnlineBrush;
		case EOnlinePresenceState::Offline:
		default:
			return &FriendStyle.OfflineBrush;
		};
	}

private:
	/** Holds the style to use when making the widget. */
	FFriendsAndChatStyle FriendStyle;
	TSharedPtr<SMenuAnchor> ActionMenu;
	TSharedPtr<FFriendsStatusViewModel> ViewModel;
};

TSharedRef<SFriendsStatus> SFriendsStatus::New()
{
	return MakeShareable(new SFriendsStatusImpl());
}

#undef LOCTEXT_NAMESPACE
