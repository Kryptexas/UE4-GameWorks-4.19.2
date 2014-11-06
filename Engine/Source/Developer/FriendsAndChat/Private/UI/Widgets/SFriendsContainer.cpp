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
		// Dragable bar for the Friends list
		class SDraggableBar : public SBox
		{
			SLATE_BEGIN_ARGS(SDraggableBar)
			{}
			SLATE_DEFAULT_SLOT(FArguments, Content)
			SLATE_END_ARGS()
			void Construct( const FArguments& InArgs )
			{
				SBox::Construct(
					SBox::FArguments()
					.Padding( 12.f )
					[
						InArgs._Content.Widget
					]
				);
				this->Cursor = EMouseCursor::CardinalCross;
			}

			virtual EWindowZone::Type GetWindowZoneOverride() const override
			{
				return EWindowZone::TitleBar;
			}
		};

		FriendStyle = *InArgs._FriendStyle;
		OnCloseClicked = InArgs._OnCloseClicked;
		OnMinimizeClicked = InArgs._OnMinimizeClicked;

		EVisibility MinimizeButtonVisibility = OnMinimizeClicked.IsBound() ? EVisibility::Visible : EVisibility::Collapsed;
		EVisibility CloseButtonVisibility = OnCloseClicked.IsBound() ? EVisibility::Visible : EVisibility::Collapsed;

		// Set up titles
		const FText SearchStringLabel = LOCTEXT( "FriendList_SearchLabel", "[Enter Display Name]" );

		MenuMethod = InArgs._Method;

		SUserWidget::Construct(SUserWidget::FArguments()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.VAlign(VAlign_Top)
			[
				SNew(SOverlay)
				.Visibility(MinimizeButtonVisibility)
				+SOverlay::Slot()
				.VAlign( VAlign_Top )
				.HAlign( HAlign_Fill )
				[
					SNew( SBorder )
					.BorderImage(&FriendStyle.TitleBarBrush)
					.BorderBackgroundColor(FLinearColor::White)
					[
						SNew(SDraggableBar)
					]
				]
				+SOverlay::Slot()
				.VAlign( VAlign_Top )
				.HAlign( HAlign_Right )
				[
					SNew( SHorizontalBox )
					+SHorizontalBox::Slot()
					.HAlign( HAlign_Right )
					.AutoWidth()
					.Padding( 5.0f )
					[
						SNew( SButton )
						.Visibility( MinimizeButtonVisibility )
						.IsFocusable( false )
						.ContentPadding( 0 )
						.Cursor( EMouseCursor::Default )
						.ButtonStyle( &FriendStyle.MinimizeButtonStyle )
						.OnClicked(this, &SFriendsContainerImpl::MinimizeButton_OnClicked)
					]
					+SHorizontalBox::Slot()
					.AutoWidth()
					.Padding( 5.0f )
					[
						SNew( SButton )
						.Visibility( CloseButtonVisibility )
						.IsFocusable( false )
						.ContentPadding( 0 )
						.Cursor( EMouseCursor::Default )
						.ButtonStyle(  &FriendStyle.CloseButtonStyle  )
						.OnClicked( this, &SFriendsContainerImpl::CloseButton_OnClicked )
					]
				]
			]
			+SVerticalBox::Slot()
			.VAlign(VAlign_Top)
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.AutoWidth()
				.Padding(5)
				[
					SNew(STextBlock)
					.Font(FriendStyle.FriendsFontStyle)
					.Text(FText::FromString("Social"))
				]
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Center)
				.AutoWidth()
				[
					SNew(SSpacer)
					.Size(FVector2D(100,0))
				]
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.AutoWidth()
				[
					SNew(SFriendsStatus, ViewModel->GetStatusViewModel())
					.FriendStyle(&FriendStyle)
					.Method(MenuMethod)
				]
				+ SHorizontalBox::Slot()
				[
					SNew(SSpacer)
				]
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				.Padding(5.f)
				.AutoWidth()
				[
					SNew(SFriendsUserSettings, ViewModel->GetUserSettingsViewModel())
					.FriendStyle(&FriendStyle)
					.Method(MenuMethod)
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.VAlign(VAlign_Top)
			[
				SNew(SSeparator)
				.Orientation(Orient_Horizontal)
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			.VAlign(VAlign_Top)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.Padding(FMargin(5,0))
				[
					SAssignNew(FriendNameTextBox, SEditableTextBox)
					.HintText(LOCTEXT("AddFriendHint", "Add friend by account name or email address"))
					.Font(FriendStyle.FriendsFontStyle)
					.OnTextCommitted(this, &SFriendsContainerImpl::HandleFriendEntered)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(5.f)
				.VAlign(VAlign_Center)
				[
					SNew(SButton)
					.ButtonStyle(&FriendStyle.FriendListActionButtonStyle)
					.OnClicked(this, &SFriendsContainerImpl::HandleAddFriendButtonClicked)
					[
						SNew(STextBlock)
						.ColorAndOpacity(FLinearColor::White)
						.Font(FriendStyle.FriendsFontStyle)
						.Text(FText::FromString("Add"))
					]
				]
			]
			+ SVerticalBox::Slot()
			.Padding(FMargin(0,5))
			[
				SNew(SBorder)
				[
					SNew(SScrollBox)
					+SScrollBox::Slot()
					[
						SNew(SVerticalBox)
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
							SNew(SFriendsListContainer, ViewModel->GetFriendListViewModel(EFriendsDisplayLists::GameInviteDisplay))
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

	FReply CloseButton_OnClicked()
	{
		if ( OnCloseClicked.IsBound() )
		{
			OnCloseClicked.Execute();
		}
		return FReply::Handled();
	}

	FReply MinimizeButton_OnClicked()
	{
		if (OnMinimizeClicked.IsBound())
		{
			OnMinimizeClicked.Execute();
		}
		return FReply::Handled();
	}

	FReply HandleAddFriendButtonClicked()
	{
		ViewModel->RequestFriend(FriendNameTextBox->GetText());
		FriendNameTextBox->SetText(FText::GetEmpty());
		return FReply::Handled();
	}

	void HandleFriendEntered(const FText& CommentText, ETextCommit::Type CommitInfo)
	{
		if (CommitInfo == ETextCommit::OnEnter)
		{
			HandleAddFriendButtonClicked();
		}
	}

private:

	// Holds the Friends List view model
	TSharedPtr<FFriendsViewModel> ViewModel;

	// Holds the menu method - Full screen requires use owning window or crashes.
	SMenuAnchor::EMethod MenuMethod;

	/** Holds the list of friends. */
	TArray< TSharedPtr< FFriendStuct > > FriendsList;

	/** Holds the list of outgoing invites. */
	TArray< TSharedPtr< FFriendStuct > > OutgoingFriendsList;

	/** Holds the tree view of the Friends list. */
	TSharedPtr< SListView< TSharedPtr< FFriendStuct > > > FriendsListView;

	/** Holds the style to use when making the widget. */
	FFriendsAndChatStyle FriendStyle;

	/** Holds the delegate for when the close button is clicked. */
	FOnClicked OnCloseClicked;

	/** Holds the delegate for when the minimize button is clicked. */
	FOnClicked OnMinimizeClicked;

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
