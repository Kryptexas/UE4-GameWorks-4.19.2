// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "SInviteItem.h"
#include "FriendViewModel.h"

#define LOCTEXT_NAMESPACE "SInviteItem"

class SInviteItemImpl : public SInviteItem
{
public:

	void Construct(const FArguments& InArgs, const TSharedRef<FFriendViewModel>& ViewModel)
	{
		FriendStyle = *InArgs._FriendStyle;
		this->ViewModel = ViewModel;
		FFriendViewModel* ViewModelPtr = this->ViewModel.Get();

		SUserWidget::Construct(SUserWidget::FArguments()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Left)
			.AutoWidth()
			[
				SNew(SImage)
				.Image(&FriendStyle.FriendImageBrush)
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Left)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(3)
				[
					SNew(STextBlock)
					.Text(ViewModel->GetFriendName())
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.Padding(3)
					.AutoWidth()
					[
						SNew(SButton)
						.ButtonStyle(&FriendStyle.FriendListActionButtonStyle)
						.ContentPadding(FMargin(15.0f, 0.f))
						.OnClicked(this, &SInviteItemImpl::PerformAction, EFriendActionType::AcceptFriendRequest)
						[
							SNew(STextBlock)
							.ColorAndOpacity(FLinearColor::White)
							.Font(FriendStyle.FriendsFontStyleSmall)
							.Text(EFriendActionType::ToText(EFriendActionType::AcceptFriendRequest))
						]
					]
					+ SHorizontalBox::Slot()
					.Padding(3)
					.AutoWidth()
					[
						SNew(SButton)
						.ContentPadding(FMargin(15.0f, 0.f))
						.ButtonStyle(&FriendStyle.FriendListActionButtonStyle)
						.OnClicked(this, &SInviteItemImpl::PerformAction, EFriendActionType::IgnoreFriendRequest)
						[
							SNew(STextBlock)
							.ColorAndOpacity(FLinearColor::White)
							.Font(FriendStyle.FriendsFontStyleSmall)
							.Text(EFriendActionType::ToText(EFriendActionType::IgnoreFriendRequest))
						]
					]
					+ SHorizontalBox::Slot()
					.Padding(3)
					.AutoWidth()
					[
						SNew(SButton)
						.ContentPadding(FMargin(15.0f, 0.f))
						.ButtonStyle(&FriendStyle.FriendListActionButtonStyle)
						.OnClicked(this, &SInviteItemImpl::PerformAction, EFriendActionType::BlockFriend)
						[
							SNew(STextBlock)
							.ColorAndOpacity(FLinearColor::White)
							.Font(FriendStyle.FriendsFontStyleSmall)
							.Text(EFriendActionType::ToText(EFriendActionType::BlockFriend))
						]
					]
				]
			]
		]);
	}

private:

	FReply PerformAction(EFriendActionType::Type FriendAction)
	{
		ViewModel->PerformAction(FriendAction);
		return FReply::Handled();
	}

private:
	/** Holds the style to use when making the widget. */
	FFriendsAndChatStyle FriendStyle;
	TSharedPtr<FFriendViewModel> ViewModel;
};

TSharedRef<SInviteItem> SInviteItem::New()
{
	return MakeShareable(new SInviteItemImpl());
}

#undef LOCTEXT_NAMESPACE
