// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "SFriendsStatusCombo.h"
#include "SFriendsAndChatCombo.h"
#include "SFriendUserHeader.h"
#include "FriendsUserViewModel.h"
#include "FriendsStatusViewModel.h"

#define LOCTEXT_NAMESPACE ""

/**
 * Declares the Friends Status display widget
 */
class SFriendsStatusComboImpl : public SFriendsStatusCombo
{
public:

	void Construct(const FArguments& InArgs, const TSharedRef<FFriendsStatusViewModel>& InViewModel, const TSharedRef<FFriendsUserViewModel>& InUserViewModel)
	{
		FriendStyle = *InArgs._FriendStyle;
		ViewModel = InViewModel;
		UserViewModel = InUserViewModel;

		FFriendsStatusViewModel* ViewModelPtr = &InViewModel.Get();

		const TArray<FFriendsStatusViewModel::FOnlineState>& StatusOptions = ViewModelPtr->GetStatusList();
		SFriendsAndChatCombo::FItemsArray ComboMenuItems;
		for (const auto& StatusOption : StatusOptions)
		{
			if (StatusOption.bIsDisplayed)
			{
				ComboMenuItems.AddItem(StatusOption.DisplayText, GetStatusBrush(StatusOption.State), FName(*StatusOption.DisplayText.ToString()), true, StatusOption.State == EOnlinePresenceState::Online ? true : false);
			}
		}

		SUserWidget::Construct(SUserWidget::FArguments()
		[
			SNew(SFriendsAndChatCombo)
			.ComboStyle(&FriendStyle.FriendsComboStyle)
			.ButtonText(ViewModelPtr, &FFriendsStatusViewModel::GetStatusText)
			.bShowIcon(true)
			.bOnline(this, &SFriendsStatusComboImpl::IsStatusEnabled)
			.DropdownItems(ComboMenuItems)
			.bSetButtonTextToSelectedItem(false)
			.bAutoCloseWhenClicked(true)
			.OnDropdownItemClicked(this, &SFriendsStatusComboImpl::HandleStatusChanged)
		 	.Cursor(EMouseCursor::Hand)
			.Content()
			[
				SNew(SBox)
				.HeightOverride(FriendStyle.FriendsComboStyle.StatusButtonSize.Y)
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
					.Padding(FMargin(0, 0, 25, 0))
					[
						SNew(SFriendUserHeader, InUserViewModel)
						.ShowUserName(true)
						.FriendStyle(&FriendStyle)
						.FontStyle(&FriendStyle.FriendsNormalFontStyle)
					]
				]
			]
		]);
	}

private:

	bool IsStatusEnabled() const
	{
		return ViewModel->GetOnlineStatus() != EOnlinePresenceState::Offline;
	}

	void HandleStatusChanged(FName ItemTag)
	{
		if (ViewModel.IsValid())
		{
			const TArray<FFriendsStatusViewModel::FOnlineState>& StatusOptions = ViewModel->GetStatusList();
			
			const FFriendsStatusViewModel::FOnlineState* FoundStatePtr = StatusOptions.FindByPredicate([ItemTag](const FFriendsStatusViewModel::FOnlineState& InOnlineState) -> bool
			{
				return InOnlineState.DisplayText.ToString() == ItemTag.ToString();
			});

			EOnlinePresenceState::Type OnlineState = EOnlinePresenceState::Offline;

			if (FoundStatePtr != nullptr)
			{
				OnlineState =  FoundStatePtr->State;
			}

			ViewModel->SetOnlineStatus(OnlineState);
		}
	}

	const FSlateBrush* GetStatusBrush(EOnlinePresenceState::Type OnlineState) const
	{
		switch (OnlineState)
		{	
			case EOnlinePresenceState::Away:
			case EOnlinePresenceState::ExtendedAway:
				return &FriendStyle.FriendsListStyle.AwayBrush;
			case EOnlinePresenceState::Chat:
			case EOnlinePresenceState::DoNotDisturb:
			case EOnlinePresenceState::Online:
				return &FriendStyle.FriendsListStyle.OnlineBrush;
			case EOnlinePresenceState::Offline:
			default:
				return &FriendStyle.FriendsListStyle.OfflineBrush;
		};
	}

private:
	/** Holds the style to use when making the widget. */
	FFriendsAndChatStyle FriendStyle;
	TSharedPtr<SMenuAnchor> ActionMenu;
	TSharedPtr<FFriendsStatusViewModel> ViewModel;
	TSharedPtr<FFriendsUserViewModel> UserViewModel;
};

TSharedRef<SFriendsStatusCombo> SFriendsStatusCombo::New()
{
	return MakeShareable(new SFriendsStatusComboImpl());
}

#undef LOCTEXT_NAMESPACE
