// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "FriendsViewModel.h"
#include "SFriendsList.h"
#include "SFriendsListContainer.h"
#include "SFriendsContainer.h"
#include "SFriendRequest.h"
#include "SFriendsStatus.h"
#include "SFriendsUserSettings.h"

#define LOCTEXT_NAMESPACE "SFriendsContainer"

/**
 * Declares the Friends List display widget
*/
class SFriendsContainerImpl : public SFriendsContainer
{
public:

	void Construct(const FArguments& InArgs, const TSharedRef<FFriendsViewModel>& ViewModel)
	{
		this->ViewModel = ViewModel;
		FriendStyle = *InArgs._FriendStyle;

		// Set up titles
		const FText SearchStringLabel = LOCTEXT( "FriendList_SearchLabel", "[Enter Display Name]" );

		MenuMethod = InArgs._Method;

		SUserWidget::Construct(SUserWidget::FArguments()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.VAlign(VAlign_Top)
			.HAlign(HAlign_Fill)
			[
				SNew(SBorder)
				.Padding(20.0f)
				.BorderImage(&FriendStyle.FriendContainerHeader)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					.AutoWidth()
					[
						SNew(SFriendsStatus, ViewModel->GetStatusViewModel())
						.FriendStyle(&FriendStyle)
						.Method(MenuMethod)
					]
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Fill)
					.Padding(FMargin(10,0,0,0))
					[
						SNew(SOverlay)
						+ SOverlay::Slot()
						[
							SNew(SSpacer).Size(FVector2D(400.0f, 0.0f))
						]
						+ SOverlay::Slot()
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Fill)
						[
							SNew(SBorder)
							.Padding(3.0f)
							.Visibility(this, &SFriendsContainerImpl::AddFriendVisibility)
							.BorderImage(&FriendStyle.AddFriendEditBorder)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								[
									SAssignNew(FriendNameTextBox, SEditableTextBox)
									.HintText(LOCTEXT("AddFriendHint", "Add friend by account name or email"))
									.Style(&FriendStyle.AddFriendEditableTextStyle)
									.OnTextCommitted(this, &SFriendsContainerImpl::HandleFriendEntered)
								]
								+ SHorizontalBox::Slot()
									.AutoWidth()
									[
										SNew(SButton)
										.ButtonStyle(&FriendStyle.AddFriendCloseButtonStyle)
										.OnClicked(this, &SFriendsContainerImpl::HandleAddFriendButtonClicked)
									]
							]
						]
						+ SOverlay::Slot()
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Right)
						[
							SNew(SButton)
							.ContentPadding(FMargin(0.0f, 5.0f))
							.ButtonStyle(&FriendStyle.FriendListActionButtonStyle)
							.OnClicked(this, &SFriendsContainerImpl::HandleAddFriendButtonClicked)
							.Visibility(this, &SFriendsContainerImpl::AddFriendActionVisibility)
							[
								SNew(SImage)
								.Image(&FriendStyle.FriendsAddImageBrush)
							]
						]
					]
				]
			]
			+ SVerticalBox::Slot()
			[
				SNew(SBorder)
				.Padding(20.0f)
				.BorderImage(&FriendStyle.FriendContainerBackground)
				[
					SNew(SScrollBox)
					+SScrollBox::Slot()
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.VAlign(VAlign_Top)
						[
							SNew(SFriendsListContainer, ViewModel->GetFriendListViewModel(EFriendsDisplayLists::GameInviteDisplay))
							.FriendStyle(&FriendStyle)
							.Method(MenuMethod)
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.VAlign(VAlign_Top)
						[
							SNew(SFriendsListContainer, ViewModel->GetFriendListViewModel(EFriendsDisplayLists::FriendRequestsDisplay))
							.FriendStyle(&FriendStyle)
							.Method(MenuMethod)
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.VAlign(VAlign_Top)
						[
							SNew(SFriendsListContainer, ViewModel->GetFriendListViewModel(EFriendsDisplayLists::DefaultDisplay))
							.FriendStyle(&FriendStyle)
							.Method(MenuMethod)
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.VAlign(VAlign_Top)
						[
							SNew(SFriendsListContainer, ViewModel->GetFriendListViewModel(EFriendsDisplayLists::RecentPlayersDisplay))
							.FriendStyle(&FriendStyle)
							.Method(MenuMethod)
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.VAlign(VAlign_Top)
						[
							SNew(SFriendsListContainer, ViewModel->GetFriendListViewModel(EFriendsDisplayLists::OutgoingFriendInvitesDisplay))
							.FriendStyle(&FriendStyle)
							.Method(MenuMethod)
						]
					]
				]
			]
		]);
	}

private:


	FReply HandleAddFriendButtonClicked()
	{
		FriendNameTextBox->SetText(FText::GetEmpty());
		ViewModel->PerformAction();
		return FReply::Handled();
	}

	void HandleFriendEntered(const FText& CommentText, ETextCommit::Type CommitInfo)
	{
		if (CommitInfo == ETextCommit::OnEnter)
		{
			ViewModel->RequestFriend(CommentText);
			HandleAddFriendButtonClicked();
		}
	}

	EVisibility AddFriendVisibility() const
	{
		return ViewModel->IsPerformingAction() ? EVisibility::Visible : EVisibility::Collapsed;
	}

	EVisibility AddFriendActionVisibility() const
	{
		return ViewModel->IsPerformingAction() ? EVisibility::Collapsed : EVisibility::Visible;
	}

private:

	// Holds the Friends List view model
	TSharedPtr<FFriendsViewModel> ViewModel;

	// Holds the menu method - Full screen requires use owning window or crashes.
	SMenuAnchor::EMethod MenuMethod;

	/** Holds the list of friends. */
	TArray< TSharedPtr< IFriendItem > > FriendsList;

	/** Holds the list of outgoing invites. */
	TArray< TSharedPtr< IFriendItem > > OutgoingFriendsList;

	/** Holds the tree view of the Friends list. */
	TSharedPtr< SListView< TSharedPtr< IFriendItem > > > FriendsListView;

	/** Holds the style to use when making the widget. */
	FFriendsAndChatStyle FriendStyle;

	/** Holds the recent players check box. */
	TSharedPtr< SCheckBox > RecentPlayersButton;

	/** Holds the recent players check box. */
	TSharedPtr< SCheckBox > FriendRequestsButton;

	/** Holds the default friends check box. */
	TSharedPtr< SCheckBox > DefaultPlayersButton;

	// Holds the Friends add text box
	TSharedPtr< SEditableTextBox > FriendNameTextBox;

};

TSharedRef<SFriendsContainer> SFriendsContainer::New()
{
	return MakeShareable(new SFriendsContainerImpl());
}

#undef LOCTEXT_NAMESPACE
