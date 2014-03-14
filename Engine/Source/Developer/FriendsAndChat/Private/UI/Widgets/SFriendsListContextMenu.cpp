// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SFriendsListContextMenu.cpp: Implements the SFriendsListContextMenu class.
=============================================================================*/

#include "FriendsAndChatPrivatePCH.h"

#define LOCTEXT_NAMESPACE "SFriendsListContextMenu"

void SFriendsListContextMenu::Construct( const FArguments& InArgs )
{
	FriendItem = InArgs._FriendItem;

	ChildSlot
	[
		MakeContextMenu()
	];
}

TSharedRef<SWidget> SFriendsListContextMenu::MakeContextMenu( )
{
	static const FText MenuAddFriend( LOCTEXT( "FriendListContextMenu_AddFriend", "ADD FRIEND" ) );
	static const FText MenuChat( LOCTEXT( "FriendListContextMenu_Chat", "CHAT" ) );
	static const FText MenuJoinGame( LOCTEXT( "FriendListContextMenu_JoinGame", "JOIN GAME" ) );
	static const FText MenuInviteToGame( LOCTEXT( "FriendListContextMenu_InviteToGame", "INVITE TO GAME" ) );
	static const FText MenuDeleteFriend( LOCTEXT( "FriendListContextMenu_DeleteFriend", "DELETE FRIEND" ) );
	static const FText MenuAcceptFriend( LOCTEXT( "FriendListContextMenu_AcceptFriend", "ACCEPT FRIEND" ) );
	static const FText MenuRejectFriend( LOCTEXT( "FriendListContextMenu_RejectFriend", "REJECT FRIEND" ) );

	FMenuBuilder MenuBuilder(true, NULL);

	const FText ContentHeader = FText::Format( LOCTEXT( "FriendListContextMenu_Options", "{0} Options"), FText::FromString( FriendItem->GetName() ) );

	MenuBuilder.BeginSection("FriendOptions", ContentHeader );
	{
		if ( FriendItem->GetListType() == EFriendsDisplayLists::RecentPlayersDisplay )
		{
			if ( !FFriendsAndChatManager::Get()->FindUserID( FriendItem->GetName() ).IsValid() )
			{
				MenuBuilder.AddMenuEntry( MenuAddFriend, FText::GetEmpty(), FSlateIcon(), FUIAction( FExecuteAction::CreateRaw( this, &SFriendsListContextMenu::OnAddFriendClicked ) ) );
			}
			else
			{
				return SNullWidget::NullWidget;
			}
		}
		else if( !FriendItem->GetOnlineFriend().IsValid() )
		{
			// No context menu for this. On delete or add list
			return SNullWidget::NullWidget;
		}
		else if ( FriendItem->GetInviteStatus() == EInviteStatus::Accepted )
		{
			// Chat is not ready yet. Can be tested though.
			if ( FParse::Param( FCommandLine::Get(), TEXT( "TestFriendsChat" ) ) )
			{
				MenuBuilder.AddMenuEntry( MenuChat, FText::GetEmpty(), FSlateIcon(), FUIAction( FExecuteAction::CreateRaw( this, &SFriendsListContextMenu::OnChatClicked ) ) );
			}
			if ( FFriendsAndChatManager::Get()->IsInSession() )
			{
				MenuBuilder.AddMenuEntry( MenuInviteToGame, FText::GetEmpty(), FSlateIcon(), FUIAction( FExecuteAction::CreateRaw( this, &SFriendsListContextMenu::OnInviteToGameClicked ) ) );
			}
			MenuBuilder.AddMenuEntry( MenuDeleteFriend, FText::GetEmpty(), FSlateIcon(), FUIAction( FExecuteAction::CreateRaw( this, &SFriendsListContextMenu::OnRemoveFriendClicked ) ) );
		}
		else if ( FriendItem->GetInviteStatus() == EInviteStatus::PendingOutbound )
		{
			MenuBuilder.AddMenuEntry( MenuDeleteFriend, FText::GetEmpty(), FSlateIcon(), FUIAction( FExecuteAction::CreateRaw( this, &SFriendsListContextMenu::OnRemoveFriendClicked ) ) );
		}
		else if ( FriendItem->GetInviteStatus() == EInviteStatus::PendingInbound )
		{
			MenuBuilder.AddMenuEntry( MenuAcceptFriend, FText::GetEmpty(), FSlateIcon(), FUIAction( FExecuteAction::CreateRaw( this, &SFriendsListContextMenu::OnAcceptFriendClicked ) ) );
			MenuBuilder.AddMenuEntry( MenuRejectFriend, FText::GetEmpty(), FSlateIcon(), FUIAction( FExecuteAction::CreateRaw( this, &SFriendsListContextMenu::OnRejectFriendClicked ) ) );
		}
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

void SFriendsListContextMenu::OnAddFriendClicked()
{
	if ( FriendItem.IsValid() )
	{
		FFriendsAndChatManager::Get()->RequestFriend( FText::FromString( FriendItem->GetName() ) );
	}
}

void SFriendsListContextMenu::OnChatClicked()
{
	if ( FriendItem.IsValid() )
	{
		FFriendsAndChatManager::Get()->GenerateChatWindow( FriendItem );
	}
}

void SFriendsListContextMenu::OnRemoveFriendClicked()
{
	if ( FriendItem.IsValid() && FriendItem->GetOnlineFriend().IsValid() )
	{
		FFriendsAndChatManager::Get()->DeleteFriend( FriendItem );
	}
}

void SFriendsListContextMenu::OnInviteToGameClicked()
{
	if ( FriendItem.IsValid() && FriendItem->GetOnlineFriend().IsValid() )
	{
		FFriendsMessageManager::Get()->InviteFriendToGame( FriendItem->GetOnlineFriend()->GetUserId() );
	}
}

void SFriendsListContextMenu::OnJoinGameClicked()
{
	if ( FriendItem.IsValid() && FriendItem->GetOnlineFriend().IsValid() )
	{
		FFriendsMessageManager::Get()->RequestJoinAGame( FriendItem->GetOnlineFriend()->GetUserId() );
	}
}

void SFriendsListContextMenu::OnAcceptFriendClicked()
{
	if ( FriendItem.IsValid() && FriendItem->GetOnlineUser().IsValid() )
	{
		FFriendsAndChatManager::Get()->AcceptFriend( FriendItem );
	}
}

void SFriendsListContextMenu::OnRejectFriendClicked()
{
	if ( FriendItem.IsValid() && FriendItem->GetOnlineUser().IsValid() )
	{
		FFriendsAndChatManager::Get()->RejectFriend( FriendItem );
	}
}
#undef LOCTEXT_NAMESPACE
