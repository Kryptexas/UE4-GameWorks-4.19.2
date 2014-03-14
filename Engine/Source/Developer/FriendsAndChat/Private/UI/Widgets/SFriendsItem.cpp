// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SFriendsItem.cpp: Implements the SFriendsItem class.
	Displays Friend widget in friends list
=============================================================================*/

#include "FriendsAndChatPrivatePCH.h"

#define LOCTEXT_NAMESPACE "SFriendsItem"

void SFriendsItem::Construct( const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView )
{
	STableRow< TSharedPtr<FFriendStuct> >::ConstructInternal(
		STableRow::FArguments()
		.ShowSelection(true),
		InOwnerTableView
		);

	FriendItem = InArgs._FriendItem;
	FriendStyle = *InArgs._FriendStyle;

	ChildSlot
	[
		SNew( SHorizontalBox )
		+SHorizontalBox::Slot()
		.Padding( 10, 0 )
		.AutoWidth()
		[
			SAssignNew( MenuButton, SComboButton )
			.ContentPadding(FMargin(6.0f, 2.0f))
			.OnGetMenuContent( this, &SFriendsItem::OnGetMenuContent )
			.MenuPlacement( EMenuPlacement::MenuPlacement_ComboBox )
			.Method( SMenuAnchor::UseCurrentWindow )
		]
		+SHorizontalBox::Slot()
		[
			SNew( STextBlock )
			.TextStyle( &FriendStyle.TextStyle )
			.ColorAndOpacity( FriendStyle.MenuUnsetColor )
			.Text( FText::FromString( FriendItem->GetName() ) )
		]
	];
}

TSharedRef<SWidget> SFriendsItem::OnGetMenuContent()
{
	TArray< EFriendsActionType::Type > FriendActions;

	if ( FriendItem.IsValid() )
	{
		if ( FriendItem->GetListType() == EFriendsDisplayLists::RecentPlayersDisplay )
		{
			if ( !FFriendsAndChatManager::Get()->FindUserID( FriendItem->GetName() ).IsValid() && FFriendsAndChatManager::Get()->IsPendingInvite( FriendItem->GetName() ) == false )
			{
				FriendActions.Add( EFriendsActionType::Add_Friend );
			}
		}
		else if ( FriendItem->GetInviteStatus() == EInviteStatus::Accepted )
		{
			if ( FFriendsAndChatManager::Get()->IsInSession() )
			{
				FriendActions.Add(EFriendsActionType::Invite_To_Game );
			}
			FriendActions.Add( EFriendsActionType::Delete_Friend );
		}
		else if ( FriendItem->GetInviteStatus() == EInviteStatus::PendingOutbound && FriendItem->GetOnlineFriend().IsValid() )
		{
			FriendActions.Add( EFriendsActionType::Cancel_Request );
		}
		else if ( FriendItem->GetInviteStatus() == EInviteStatus::PendingInbound )
		{
			FriendActions.Add( EFriendsActionType::Accept_Friend );
			FriendActions.Add( EFriendsActionType::Reject_Friend );
		}
	}

	if ( FriendActions.Num() == 0 )
	{
		return SNullWidget::NullWidget;
	}

	TSharedPtr< SVerticalBox > DisplayBox;
	SAssignNew( DisplayBox, SVerticalBox );

	for( auto Iter(FriendActions.CreateConstIterator()); Iter; Iter++ )
	{
		DisplayBox->AddSlot()
		.AutoHeight()
		[
			SNew( SButton )
			.OnClicked( this, &SFriendsItem::OnOptionSelected, *Iter )
			[
				SNew( STextBlock )
				.Text( GetActionText( *Iter ) )
				.TextStyle( &FriendStyle.TextStyle )
				.ColorAndOpacity( FriendStyle.MenuSetColor )
			]
		];
	}
	
	return DisplayBox.ToSharedRef();
}

FReply SFriendsItem::OnOptionSelected( EFriendsActionType::Type FriendActionType )
{
	switch( FriendActionType )
	{
		case EFriendsActionType::Add_Friend:
			{
				FFriendsAndChatManager::Get()->RequestFriend( FText::FromString( FriendItem->GetName() ) );
			}
			break;
		case EFriendsActionType::Delete_Friend:
			{
				FFriendsAndChatManager::Get()->DeleteFriend( FriendItem );
			}
			break;
		case EFriendsActionType::Cancel_Request:
			{
				FFriendsAndChatManager::Get()->DeleteFriend( FriendItem );
			}
			break;
		case EFriendsActionType::Invite_To_Game:
			{
				FFriendsMessageManager::Get()->InviteFriendToGame( FriendItem->GetOnlineFriend()->GetUserId() );
			}
			break;
		case EFriendsActionType::Join_Game:
			{
				FFriendsMessageManager::Get()->RequestJoinAGame( FriendItem->GetOnlineFriend()->GetUserId() );
			}
			break;
		case EFriendsActionType::Reject_Friend:
			{
				FFriendsAndChatManager::Get()->DeleteFriend( FriendItem );
			}
			break;
		case EFriendsActionType::Accept_Friend:
			{
				FFriendsAndChatManager::Get()->AcceptFriend( FriendItem );
			}
			break;
	}
	MenuButton->SetIsOpen(false);
	return FReply::Handled();
}

const FText& SFriendsItem::GetActionText( EFriendsActionType::Type FriendActionType )
{
	static const FText AddFriend = LOCTEXT( "SFriendsItem_AddFriendAction", "Add Friend" );
	static const FText DeleteFriend = LOCTEXT( "SFriendsItem_DeleteFriendAction", "Delete Friend" );
	static const FText CancelRequest = LOCTEXT( "SFriendsItem_CancelAddFriendAction", "Cancel Request" );
	static const FText InviteToGame = LOCTEXT( "SFriendsItem_InviteAction", "Invite To Game" );
	static const FText JoinGame = LOCTEXT( "SFriendsItem_JoinGameAction", "Join Game" );
	static const FText RejectFriend = LOCTEXT( "SFriendsItem_RejectFriendAction", "Reject Friend Request" );
	static const FText AcceptFriend = LOCTEXT( "SFriendsItem_AcceptFriendAction", "Accept Friend Request" );

	switch( FriendActionType )
	{
	case EFriendsActionType::Add_Friend: return AddFriend;
	case EFriendsActionType::Delete_Friend: return DeleteFriend;
	case EFriendsActionType::Cancel_Request: return CancelRequest;
	case EFriendsActionType::Invite_To_Game: return InviteToGame;
	case EFriendsActionType::Join_Game: return JoinGame;
	case EFriendsActionType::Reject_Friend: return RejectFriend;
	case EFriendsActionType::Accept_Friend: return AcceptFriend;
	}

	return FText::GetEmpty();
}

#undef LOCTEXT_NAMESPACE
