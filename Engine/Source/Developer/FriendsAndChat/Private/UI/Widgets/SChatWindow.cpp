// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SChatWindow.cpp: Implements the SChatWindow class.
	Displays the chat window
=============================================================================*/

#include "FriendsAndChatPrivatePCH.h"

#define LOCTEXT_NAMESPACE "SChatWindow"

void SChatWindow::Construct( const FArguments& InArgs )
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
	FriendItem = InArgs._FriendItem;

	OnMinimizeClicked = InArgs._OnMinimizeClicked;

	FFriendsMessageManager::Get()->OnChatListUpdated().AddSP( this, &SChatWindow::RefreshChatList );

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
				.Text(  FText::FromString( FriendItem->GetName() ) )
			]
			+SVerticalBox::Slot()
			.FillHeight( 1.0f )
			.VAlign( VAlign_Fill )
			[
				SNew( SBorder )
				[
					SAssignNew( ChatListView, SListView< TSharedPtr< FFriendsAndChatMessage > > )
					.ItemHeight( 18 )
					.ListItemsSource( &ChatItems )
					.SelectionMode( ESelectionMode::Single)
					.OnGenerateRow( this, &SChatWindow::OnGenerateWidgetForChat )
				]
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			.VAlign( VAlign_Bottom )
			[
				SNew( SBorder )
				[
					SNew( SHorizontalBox )
					+SHorizontalBox::Slot()
					.HAlign( HAlign_Fill )
					.FillWidth( 1.0f )
					[
						SAssignNew( ChatBox, SEditableTextBox )
						.HintText( FText::FromString( "Say" ) )
					]
					+SHorizontalBox::Slot()
					.HAlign( HAlign_Right )
					[
						SNew( SButton )
						.OnClicked( this, &SChatWindow::ChatButton_OnClicked )
						[
							SNew( STextBlock )
							.Text( FText::FromString( "SEND" ) )
						]
					]
				]
			]
		]
		+SOverlay::Slot()
		.VAlign( VAlign_Top )
		.HAlign( HAlign_Fill )
		[
			SNew(SDraggableBar)
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
				.IsFocusable( false )
				.ContentPadding( 0 )
				.Cursor( EMouseCursor::Default )
				.ButtonStyle( &FriendStyle.MinimizeButtonStyle )
				.OnClicked( this, &SChatWindow::MinimizeButton_OnClicked )
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding( 5.0f )
			[
				SNew( SButton )
				.IsFocusable( false )
				.ContentPadding( 0 )
				.Cursor( EMouseCursor::Default )
				.ButtonStyle(  &FriendStyle.CloseButtonStyle  )
				.OnClicked( this, &SChatWindow::CloseButton_OnClicked )
			]
		]
	];

	RefreshChatList();
}

FReply SChatWindow::CloseButton_OnClicked()
{
	if ( OnCloseClicked.IsBound() )
	{
		OnCloseClicked.Execute();
	}
	return FReply::Handled();
}

FReply SChatWindow::MinimizeButton_OnClicked()
{
	if (OnMinimizeClicked.IsBound())
	{
		OnMinimizeClicked.Execute();
	}
	return FReply::Handled();
}

FReply SChatWindow::ChatButton_OnClicked()
{
	
	FFriendsMessageManager::Get()->SendMessage( FriendItem, ChatBox->GetText() );
	return FReply::Handled();
}

TSharedRef< ITableRow > SChatWindow::OnGenerateWidgetForChat( TSharedPtr< FFriendsAndChatMessage > Message, const TSharedRef< STableViewBase >& OwnerTable)
{
	check(Message.IsValid());

	return SNew(STableRow<TSharedPtr<FString> >, OwnerTable)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0)
			[
				SNew(STextBlock)
				.Text( Message->GetMessage() )
			]
		];
}

void SChatWindow::RefreshChatList()
{
	ChatItems = FFriendsMessageManager::Get()->GetChatMessages();
	ChatListView->RequestListRefresh();
}

#undef LOCTEXT_NAMESPACE
