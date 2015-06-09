// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "SFriendUserHeader.h"
#include "IUserInfo.h"

#define LOCTEXT_NAMESPACE "SFriendUserHeader"

class SFriendUserHeaderImpl : public SFriendUserHeader
{
public:

	void Construct(const FArguments& InArgs, const TSharedRef<const IUserInfo>& InViewModel)
	{
		FriendStyle = *InArgs._FriendStyle;
		FriendsFontStyle = *InArgs._FontStyle;
		this->ViewModel = InViewModel;

		SUserWidget::Construct(SUserWidget::FArguments()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Left)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SBox)
					.WidthOverride(FriendStyle.FriendsListStyle.UserPresenceImageSize.X)
					.HeightOverride(FriendStyle.FriendsListStyle.UserPresenceImageSize.Y)
					[
						SNew(SImage)
						.Image(this, &SFriendUserHeaderImpl::GetPresenceBrush)
					]
				]
				+ SOverlay::Slot()
				.VAlign(VAlign_Top)
				.HAlign(HAlign_Right)
				[
					SNew(SImage)
					.Image(this, &SFriendUserHeaderImpl::GetStatusBrush)
				]
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.Padding(10, 0, 10, 2)
			[
				SNew(STextBlock)
				.Visibility(InArgs._ShowUserName ? EVisibility::Visible : EVisibility::Collapsed)
				.Font(FriendsFontStyle.FriendsFontLarge)
				.ColorAndOpacity(FriendsFontStyle.DefaultFontColor)
				.Text(this, &SFriendUserHeaderImpl::GetDisplayName)
			]
		]);
	}

private:

	const FSlateBrush* GetPresenceBrush() const
	{
		if (ViewModel->GetOnlineStatus() != EOnlinePresenceState::Offline)
		{
			FString ClientId = ViewModel->GetClientId();
			if (!ClientId.IsEmpty())
			{
				//@todo samz - better way of finding known ids
				if (ClientId == FFriendItem::FortniteClientId)
				{
					return &FriendStyle.FriendsListStyle.FortniteImageBrush;
				}
				else if (FFriendItem::LauncherClientIds.Contains(ClientId))
				{
					return &FriendStyle.FriendsListStyle.LauncherImageBrush;
				}
				else if (ClientId == FFriendItem::UnrealTournamentClientId)
				{
					return &FriendStyle.FriendsListStyle.UTImageBrush;
				}
			}
		}
		return &FriendStyle.FriendsListStyle.FriendImageBrush;
	}

	const FSlateBrush* GetStatusBrush() const
	{
		switch (ViewModel->GetOnlineStatus())
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

	FText GetDisplayName() const
	{
		return FText::FromString(ViewModel->GetName());
	}

private:

	TSharedPtr<const IUserInfo> ViewModel;

	/** Holds the style to use when making the widget. */
	FFriendsAndChatStyle FriendStyle;
	FFriendsFontStyle FriendsFontStyle;
};

TSharedRef<SFriendUserHeader> SFriendUserHeader::New()
{
	return MakeShareable(new SFriendUserHeaderImpl());
}

#undef LOCTEXT_NAMESPACE
