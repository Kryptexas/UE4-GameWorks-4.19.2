// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SFriendsList.cpp: Implements the SFriendsList class.
	Displays the list of friends
=============================================================================*/

#include "FriendsAndChatPrivatePCH.h"

#define LOCTEXT_NAMESPACE "SFriendsList"

void SFriendsList::Construct( const FArguments& InArgs )
{
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

		virtual EWindowZone::Type GetWindowZoneOverride() const OVERRIDE
		{
			return EWindowZone::TitleBar;
		}
	};

	FriendStyle = *InArgs._FriendStyle;
	OnCloseClicked = InArgs._OnCloseClicked;
	OnMinimizeClicked = InArgs._OnMinimizeClicked;

	EVisibility MinimizeButtonVisibility = OnMinimizeClicked.IsBound() ? EVisibility::Visible : EVisibility::Collapsed;
	EVisibility CloseButtonVisibility = OnCloseClicked.IsBound() ? EVisibility::Visible : EVisibility::Collapsed;

	// Get the friends list
	FFriendsAndChatManager::Get()->GetFilteredFriendsList( FriendsList );

	// Add delegate to call when the Friends list updates
	FFriendsAndChatManager::Get()->OnFriendsListUpdated().AddSP( this, &SFriendsList::RefreshFriendsList );

	// Set up titles
	const FText SearchStringLabel = LOCTEXT( "FriendList_SearchLabel", "[Enter Display Name]" );

	// Set default friends list 
	CurrentList = EFriendsDisplayLists::DefaultDisplay;

	ChildSlot
	[
		SNew( SOverlay )
		+SOverlay::Slot()
		[
			SNew( SVerticalBox )
			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding( 8 )
			.VAlign( VAlign_Top )
			[
				SNew( STextBlock )
				.Font( FriendStyle.FriendsFontStyle )
				.Text( this, &SFriendsList::GetListCountText )
			]
			+SVerticalBox::Slot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Fill)
			.AutoHeight()
			[
				SNew( SHorizontalBox )
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SAssignNew( DefaultPlayersButton, SCheckBox )
					.Style( &FriendStyle.CheckBoxToggleStyle )
					.Type( ESlateCheckBoxType::ToggleButton )
					.IsChecked( CurrentList == EFriendsDisplayLists::DefaultDisplay )
					.OnCheckStateChanged( this, &SFriendsList::DefaultListSelect_OnChanged )
					.ForegroundColor( this, &SFriendsList::GetMenuTextColor, EFriendsDisplayLists::DefaultDisplay )
					[
						SNew( STextBlock )
						.Font( FriendStyle.FriendsFontStyle )
						.Text( LOCTEXT("FriendsList", "Friends").ToString() )
					]
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SAssignNew( RecentPlayersButton, SCheckBox )
					.Style( &FriendStyle.CheckBoxToggleStyle )
					.Type( ESlateCheckBoxType::ToggleButton )
					.IsChecked( CurrentList == EFriendsDisplayLists::RecentPlayersDisplay )
					.OnCheckStateChanged(this, &SFriendsList::RecentListSelect_OnChanged )
					.ForegroundColor( this, &SFriendsList::GetMenuTextColor, EFriendsDisplayLists::RecentPlayersDisplay )
					[
						SNew(STextBlock)
						.Font(FriendStyle.FriendsFontStyle)
						.Text(LOCTEXT("RecentPlayers", "Recent").ToString())
					]
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SAssignNew( FriendRequestsButton, SCheckBox )
					.Style( &FriendStyle.CheckBoxToggleStyle )
					.Type( ESlateCheckBoxType::ToggleButton )
					.IsChecked( CurrentList == EFriendsDisplayLists::FriendRequestsDisplay )
					.OnCheckStateChanged(this, &SFriendsList::RequestListSelect_OnChanged )
					.ForegroundColor( this, &SFriendsList::GetMenuTextColor, EFriendsDisplayLists::FriendRequestsDisplay )
					[
						SNew( STextBlock )
						.Font( FriendStyle.FriendsFontStyle )
						.Text( LOCTEXT("FriendRequest", "Requests").ToString() )
					]
				]
			]
			+SVerticalBox::Slot()
			.FillHeight( 1.0f )
			.VAlign( VAlign_Fill )
			[
				SAssignNew( DisplayBox, SVerticalBox )
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			.VAlign( VAlign_Bottom )
			[
				SNew( SHorizontalBox )
				+SHorizontalBox::Slot()
				[
					SAssignNew( FriendNameTextBox, SEditableTextBox )
					.Font( FriendStyle.FriendsFontStyle )
					.OnTextCommitted( this, &SFriendsList::HandleFriendEntered )
					.BackgroundColor( FLinearColor::Black )
					.ForegroundColor( FLinearColor::White )
					.HintText( SearchStringLabel )
				]
				+SHorizontalBox::Slot()
				.HAlign( HAlign_Right )
				.VAlign( VAlign_Center )
				.AutoWidth()
				[
					SNew( SButton )
					.ButtonStyle( &FriendStyle.SearchButtonStyle )
					.OnClicked( this, &SFriendsList::SearchFriend_OnClicked )
				]
			]
		]
		+SOverlay::Slot()
		.VAlign( VAlign_Top )
		.HAlign( HAlign_Fill )
		[
			SNew( SHorizontalBox )
			.Visibility( MinimizeButtonVisibility )
			+SHorizontalBox::Slot()
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
				.OnClicked( this, &SFriendsList::MinimizeButton_OnClicked )
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
				.OnClicked( this, &SFriendsList::CloseButton_OnClicked )
			]
		]
	];

	RefreshFriendsList();
	FFriendsAndChatManager::Get()->SetListSelect( CurrentList );
}

TSharedRef<ITableRow> SFriendsList::HandleGenerateFriendWidget( TSharedPtr< FFriendStuct > InItem, const TSharedRef< STableViewBase >& OwnerTable )
{
	return SNew( SFriendsItem, OwnerTable )
		.FriendItem( InItem )
		.FriendStyle( &FriendStyle );
}

void SFriendsList::RefreshFriendsList()
{
	FriendsList.Empty();
	FFriendsAndChatManager::Get()->GetFilteredFriendsList( FriendsList );

	DisplayBox->ClearChildren();

	if ( CurrentList == EFriendsDisplayLists::FriendRequestsDisplay )
	{
		OutgoingFriendsList.Empty();
		FFriendsAndChatManager::Get()->GetFilteredOutgoingFriendsList( OutgoingFriendsList );

		EVisibility OutGoingVisibility = OutgoingFriendsList.Num() ? EVisibility::Visible : EVisibility::Collapsed;
		EVisibility IncomingVisibility = FriendsList.Num() ? EVisibility::Visible : EVisibility::Collapsed;
		EVisibility SeperatorVisibility = OutGoingVisibility == EVisibility::Visible && IncomingVisibility == EVisibility::Visible ? EVisibility::Visible : EVisibility::Collapsed;

		DisplayBox->AddSlot()
		[
			SNew( SScrollBox )
			+ SScrollBox::Slot()
			[
				SNew( SVerticalBox)
				+SVerticalBox::Slot()
				.Padding( 0, 5 )
				.AutoHeight()
				[
					SNew( STextBlock )
					.Visibility( IncomingVisibility )
					.TextStyle( &FriendStyle.TextStyle )
					.ColorAndOpacity( FriendStyle.MenuUnsetColor )
					.Text( FText::FromString( "Incoming" ) )
				]
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew( SListView< TSharedPtr< FFriendStuct > > )
					.Visibility( IncomingVisibility )
					.ItemHeight( 40.0f )
					.ListItemsSource( &FriendsList )
					.OnGenerateRow( this, &SFriendsList::HandleGenerateFriendWidget )
					.SelectionMode( ESelectionMode::None )
				]
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding( 10 )
				[
					SNew( SSeparator )
					.Visibility( SeperatorVisibility )
				]
				+SVerticalBox::Slot()
				.Padding( 0, 5 )
				.AutoHeight()
				[
					SNew( STextBlock )
					.Visibility( OutGoingVisibility )
					.Text( FText::FromString( "Outgoing" ) )
					.TextStyle( &FriendStyle.TextStyle )
					.ColorAndOpacity( FriendStyle.MenuUnsetColor )
				]
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew( SListView< TSharedPtr< FFriendStuct > > )
					.Visibility( OutGoingVisibility )
					.ItemHeight( 40.0f )
					.ListItemsSource( &OutgoingFriendsList )
					.OnGenerateRow( this, &SFriendsList::HandleGenerateFriendWidget )
					.SelectionMode( ESelectionMode::None )
				]
			]
		];
	}
	else
	{
		DisplayBox->AddSlot()
		[
			SNew( SListView< TSharedPtr< FFriendStuct > > )
			.ItemHeight( 40.0f )
			.ListItemsSource( &FriendsList )
			.OnGenerateRow( this, &SFriendsList::HandleGenerateFriendWidget )
			.SelectionMode( ESelectionMode::None )
		];
	}
}

FString SFriendsList::GetListCountText() const
{
	static const FString ListCount = LOCTEXT( "FriendList_Title", "FRIENDS : " ).ToString();
	return ListCount + FString::Printf( TEXT( "%d" ), FFriendsAndChatManager::Get()->GetFriendCount() );
}

FReply SFriendsList::SearchFriend_OnClicked()
{
	if ( FriendNameTextBox.IsValid() )
	{
		FFriendsAndChatManager::Get()->RequestFriend( FriendNameTextBox->GetText() );
	}
	FriendNameTextBox->SetText( FText::GetEmpty() );
	return FReply::Handled();
}

void SFriendsList::HandleFriendEntered(const FText& CommentText, ETextCommit::Type CommitInfo)
{
	if (CommitInfo == ETextCommit::OnEnter)
	{
		SearchFriend_OnClicked();
	}
}

FReply SFriendsList::CloseButton_OnClicked()
{
	if ( OnCloseClicked.IsBound() )
	{
		OnCloseClicked.Execute();
	}
	return FReply::Handled();
}

FReply SFriendsList::MinimizeButton_OnClicked()
{
	if (OnMinimizeClicked.IsBound())
	{
		OnMinimizeClicked.Execute();
	}
	return FReply::Handled();
}

void SFriendsList::DefaultListSelect_OnChanged( ESlateCheckBoxState::Type CheckState )
{
	if ( CheckState == ESlateCheckBoxState::Checked )
	{
		if ( CurrentList != EFriendsDisplayLists::DefaultDisplay )
		{
			CurrentList = EFriendsDisplayLists::DefaultDisplay;
			FFriendsAndChatManager::Get()->SetListSelect( CurrentList );
			UpdateButtonState();
		}
	}
	else if ( CurrentList == EFriendsDisplayLists::DefaultDisplay )
	{
		DefaultPlayersButton->ToggleCheckedState();
	}
}

void SFriendsList::RecentListSelect_OnChanged( ESlateCheckBoxState::Type CheckState )
{
	if ( CheckState == ESlateCheckBoxState::Checked )
	{
		if ( CurrentList != EFriendsDisplayLists::RecentPlayersDisplay )
		{
			CurrentList = EFriendsDisplayLists::RecentPlayersDisplay;
			FFriendsAndChatManager::Get()->SetListSelect( CurrentList );
			UpdateButtonState();
		}
	}
	else if ( CurrentList == EFriendsDisplayLists::RecentPlayersDisplay )
	{
		RecentPlayersButton->ToggleCheckedState();
	}
}

void SFriendsList::RequestListSelect_OnChanged( ESlateCheckBoxState::Type CheckState )
{
	if ( CheckState == ESlateCheckBoxState::Checked )
	{
		if ( CurrentList != EFriendsDisplayLists::FriendRequestsDisplay )
		{
			CurrentList = EFriendsDisplayLists::FriendRequestsDisplay;
			FFriendsAndChatManager::Get()->SetListSelect( CurrentList );
			UpdateButtonState();
		}
	}
	else if ( CurrentList == EFriendsDisplayLists::FriendRequestsDisplay )
	{
		FriendRequestsButton->ToggleCheckedState();
	}
}

void SFriendsList::UpdateButtonState()
{
	if ( CurrentList != EFriendsDisplayLists::DefaultDisplay &&
		DefaultPlayersButton->IsChecked() )
	{
		DefaultPlayersButton->ToggleCheckedState();
	}
	
	if ( CurrentList != EFriendsDisplayLists::RecentPlayersDisplay &&
		RecentPlayersButton->IsChecked() )
	{
		RecentPlayersButton->ToggleCheckedState();
	}

	if ( CurrentList != EFriendsDisplayLists::FriendRequestsDisplay &&
		FriendRequestsButton->IsChecked() )
	{
		FriendRequestsButton->ToggleCheckedState();
	}
}

FSlateColor SFriendsList::GetMenuTextColor(EFriendsDisplayLists::Type ListType) const
{
	if(ListType == CurrentList)
	{
		return FriendStyle.MenuSetColor;
	}
	else
	{
		return FriendStyle.MenuUnsetColor;
	}
}

#undef LOCTEXT_NAMESPACE
