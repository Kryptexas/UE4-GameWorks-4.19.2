// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	FriendsAndChatManager.cpp: Implements the FFriendsAndChatManager class.
=============================================================================*/

#include "FriendsAndChatPrivatePCH.h"

#define LOCTEXT_NAMESPACE "FriendsAndChatManager"

/* FFriendsAndChatManager structors
 *****************************************************************************/

FFriendsAndChatManager::FFriendsAndChatManager( )
	: ManagerState ( EFriendsAndManagerState::Idle )
	, UpdateTimeInterval( 10.0f )
	, CurrentList( EFriendsDisplayLists::DefaultDisplay )
	, ConfirmedFriendsCount( 0 )
	, bIsInSession( false )
	, bIsInited( false )
{}

FFriendsAndChatManager::~FFriendsAndChatManager( )
{}

/* IFriendsAndChatManager interface
 *****************************************************************************/

void FFriendsAndChatManager::StartupManager()
{
}

void FFriendsAndChatManager::Init( FOnFriendsNotification& NotificationDelegate )
{
	// Clear existing data
	Logout();
	FriendsListNotificationDelegate = &NotificationDelegate;

	UpdateTimer = 0.0f;
	bIsInited = false;
	FFriendsMessageManager::Get()->SetMessagePolling( false );

	OnlineSubMcp = static_cast< FOnlineSubsystemMcp* >( IOnlineSubsystem::Get( TEXT( "MCP" ) ) );

	if (OnlineSubMcp != NULL &&
		OnlineSubMcp->GetMcpAccountMappingService().IsValid() &&
		OnlineSubMcp->GetIdentityInterface().IsValid())
	{
		OnlineIdentity = OnlineSubMcp->GetIdentityInterface();

		IOnlineUserPtr UserInterface = OnlineSubMcp->GetUserInterface();
		check(UserInterface.IsValid());

		FriendsInterface = OnlineSubMcp->GetFriendsInterface();
		check( FriendsInterface.IsValid() )

		// Create delegates for list refreshes
		OnReadFriendsCompleteDelegate = FOnReadFriendsListCompleteDelegate::CreateSP( this, &FFriendsAndChatManager::OnReadFriendsComplete );
		OnAcceptInviteCompleteDelegate = FOnAcceptInviteCompleteDelegate::CreateSP( this, &FFriendsAndChatManager::OnAcceptInviteComplete );
		OnSendInviteCompleteDelegate = FOnSendInviteCompleteDelegate::CreateSP( this, &FFriendsAndChatManager::OnSendInviteComplete );
		OnDeleteFriendsListCompleteDelegate = FOnDeleteFriendsListCompleteDelegate::CreateSP( this, &FFriendsAndChatManager::OnDeleteFriendsListComplete );
		OnDeleteFriendCompleteDelegate = FOnDeleteFriendCompleteDelegate::CreateSP( this, &FFriendsAndChatManager::OnDeleteFriendComplete );
		OnQueryUserIdFromIdentificationStringCompleteDelegate = FOnQueryUserIdFromIdentificationStringCompleteDelegate::CreateSP( this, &FFriendsAndChatManager::OnQueryUserIdFromIdentificationStringComplete );
		OnQueryUserInfoCompleteDelegate = FOnQueryUserInfoCompleteDelegate::CreateSP( this, &FFriendsAndChatManager::OnQueryUserInfoComplete );

		FriendsInterface->AddOnReadFriendsListCompleteDelegate( 0, OnReadFriendsCompleteDelegate );

		ManagerState = EFriendsAndManagerState::Idle;

		FriendsList.Empty();
		PendingFriendsList.Empty();

		if ( UpdateFriendsTickerDelegate.IsBound() == false )
		{
			UpdateFriendsTickerDelegate = FTickerDelegate::CreateSP( this, &FFriendsAndChatManager::Tick );
		}

		SetInSession( false );
		FTicker::GetCoreTicker().AddTicker( UpdateFriendsTickerDelegate );
	}
}

bool FFriendsAndChatManager::Tick( float Delta )
{
	if ( ManagerState == EFriendsAndManagerState::Idle )
	{
		UpdateTimer -= Delta;

		if ( FriendByNameRequests.Num() > 0 )
		{
			SetState(EFriendsAndManagerState::RequestingFriends );
		}
		else if ( PendingOutgoingDeleteFriendRequests.Num() > 0 )
		{
			SetState( EFriendsAndManagerState::DeletingFriends );
		}
		else if ( PendingOutgoingAcceptFriendRequests.Num() > 0 )
		{
			SetState( EFriendsAndManagerState::AcceptingFriendRequest );
		}
		else if ( PendingIncomingInvitesList.Num() > 0 )
		{
			SendFriendInviteNotification();
		}
		else if ( UpdateTimer < 0.0f )
		{
			SetState( EFriendsAndManagerState::RequestFriendsListRefresh );
			UpdateTimer = UpdateTimeInterval;
		}
	}
	return true;
}

void FFriendsAndChatManager::Logout()
{
	if ( FriendsInterface.IsValid() )
	{
		FriendsInterface->ClearOnReadFriendsListCompleteDelegate( 0, OnReadFriendsCompleteDelegate );
		FriendsInterface->ClearOnAcceptInviteCompleteDelegate( 0, OnAcceptInviteCompleteDelegate );
		FriendsInterface->ClearOnSendInviteCompleteDelegate( 0, OnSendInviteCompleteDelegate );
		FriendsInterface->ClearOnDeleteFriendsListCompleteDelegate( 0, OnDeleteFriendsListCompleteDelegate );
		FriendsInterface->ClearOnDeleteFriendCompleteDelegate( 0, OnDeleteFriendCompleteDelegate );

		if ( OnlineSubMcp != NULL )
		{
			FOnlineAccountMappingMcpPtr OnlineAccountMappingMcp = OnlineSubMcp->GetMcpAccountMappingService();
			if ( OnlineAccountMappingMcp.IsValid() )
			{
				OnlineAccountMappingMcp->ClearOnQueryUserIdFromIdentificationStringCompleteDelegate( OnQueryUserIdFromIdentificationStringCompleteDelegate );
				OnlineSubMcp->GetUserInterface()->ClearOnQueryUserInfoCompleteDelegate( 0, OnQueryUserInfoCompleteDelegate );
			}
		}
	}

	FriendsList.Empty();
	RecentPlayerList.Empty();
	PendingFriendsList.Empty();
	FriendByNameRequests.Empty();
	FilteredFriendsList.Empty();
	PendingOutgoingDeleteFriendRequests.Empty();
	PendingOutgoingAcceptFriendRequests.Empty();
	PendingIncomingInvitesList.Empty();
	NotifiedRequest.Empty();
	ConfirmedFriendsCount = 0;

	if ( FriendWindow.IsValid() )
	{
		FriendWindow->RequestDestroyWindow();
		FriendWindow = NULL;
	}

	if ( ChatWindow.IsValid() )
	{
		ChatWindow->RequestDestroyWindow();
		ChatWindow = NULL;
	}

	OnlineSubMcp = NULL;
	if ( UpdateFriendsTickerDelegate.IsBound() )
	{
		FTicker::GetCoreTicker().RemoveTicker( UpdateFriendsTickerDelegate );
	}
}

void FFriendsAndChatManager::GenerateFriendsWindow( TSharedPtr< const SWidget > InParentWidget, const FFriendsAndChatStyle* InStyle )
{
	const FVector2D DEFAULT_WINDOW_SIZE = FVector2D(308, 458);
	ParentWidget = InParentWidget;
	Style = *InStyle;

	if ( !FriendWindow.IsValid() )
	{
		FriendWindow = SNew( SWindow )
		.Title( LOCTEXT( "FFriendsAndChatManager_FriendsTitle", "Friends List") )
		.ClientSize( DEFAULT_WINDOW_SIZE )
		.ScreenPosition( FVector2D( 100, 100 ) )
		.AutoCenter( EAutoCenter::None )
		.SupportsMaximize( true )
		.SupportsMinimize( true )
		.CreateTitleBar( false )
		.SizingRule( ESizingRule::FixedSize )
		[
			SNew( SBorder )
			.VAlign( VAlign_Fill )
			.HAlign( HAlign_Fill )
			.BorderImage( &InStyle->Background )
			[
				SNew( SFriendsList )
				.FriendStyle( &Style )
				.OnCloseClicked( this, &FFriendsAndChatManager::OnCloseClicked )
				.OnMinimizeClicked( this, &FFriendsAndChatManager::OnMinimizeClicked )
			]
		];

		if ( ParentWidget.IsValid() )
		{
			FWidgetPath WidgetPath;
			FSlateApplication::Get().GeneratePathToWidgetChecked( ParentWidget.ToSharedRef(), WidgetPath );
			FriendWindow = FSlateApplication::Get().AddWindowAsNativeChild( FriendWindow.ToSharedRef(), WidgetPath.GetWindow() );
		}
	}
	else
	{
		FriendWindow->Restore();
	}
}

void FFriendsAndChatManager::SetInSession( bool bInSession )
{
	bIsInSession = bInSession;
	ReadListRequests.AddUnique( EFriendsLists::RecentPlayers );
}

bool FFriendsAndChatManager::IsInSession()
{
	return bIsInSession;
}

TSharedPtr< SWidget > FFriendsAndChatManager::GenerateFriendsListWidget( const FFriendsAndChatStyle* InStyle )
{
	if ( !FriendListWidget.IsValid() )
	{
		Style = *InStyle;
		SAssignNew( FriendListWidget, SFriendsList )
		.FriendStyle( &Style );
	}

	return FriendListWidget;
}

void FFriendsAndChatManager::GenerateChatWindow( TSharedPtr< FFriendStuct > FriendItem )
{
	const FVector2D DEFAULT_WINDOW_SIZE = FVector2D(308, 458);

	if ( !ChatWindow.IsValid() )
	{
		ChatWindow = SNew( SWindow )
		.Title( LOCTEXT( "FriendsAndChatManager_ChatTitle", "Chat Window") )
		.ClientSize( DEFAULT_WINDOW_SIZE )
		.ScreenPosition( FVector2D( 200, 100 ) )
		.AutoCenter( EAutoCenter::None )
		.SupportsMaximize( true )
		.SupportsMinimize( true )
		.CreateTitleBar( false )
		.SizingRule( ESizingRule::FixedSize )
		[
			SNew( SBorder )
			.VAlign( VAlign_Fill )
			.HAlign( HAlign_Fill )
			.BorderImage( &Style.Background )
			[
				SNew( SChatWindow )
				.FriendStyle( &Style )
				.FriendItem( FriendItem )
				.OnCloseClicked( this, &FFriendsAndChatManager::OnChatCloseClicked )
				.OnMinimizeClicked( this, &FFriendsAndChatManager::OnChatMinimizeClicked )
			]
		];

		if ( ParentWidget.IsValid() )
		{
			FWidgetPath WidgetPath;
			FSlateApplication::Get().GeneratePathToWidgetChecked( ParentWidget.ToSharedRef(), WidgetPath );
			ChatWindow = FSlateApplication::Get().AddWindowAsNativeChild( ChatWindow.ToSharedRef(), WidgetPath.GetWindow() );
		}
	}
}

void FFriendsAndChatManager::AcceptFriend( TSharedPtr< FFriendStuct > FriendItem )
{
	PendingOutgoingAcceptFriendRequests.Add( FriendItem->GetOnlineUser()->GetUserId() );
	FriendItem->SetPendingAccept();
	GenerateFriendsCount();
	RefreshList();
	ClearNotification( FriendItem );
}

void FFriendsAndChatManager::RejectFriend( TSharedPtr< FFriendStuct > FriendItem )
{
	NotifiedRequest.Remove( FriendItem->GetOnlineUser()->GetUserId() );
	PendingOutgoingDeleteFriendRequests.Add( FriendItem->GetOnlineUser()->GetUserId() );
	FriendsList.Remove( FriendItem );
	GenerateFriendsCount();
	RefreshList();
	ClearNotification( FriendItem );
}

void FFriendsAndChatManager::ClearNotification( TSharedPtr< FFriendStuct > FriendItem )
{
	if ( NotificationMessages.Num() > 0 )
	{
		if ( NotificationMessages[0]->GetUniqueID().Get() == FriendItem->GetUniqueID().Get() )
		{
			// Inform the UI that this friend has been rejected. Clear any notifications
			NotificationMessages[0]->SetHandled();
			FriendsListNotificationDelegate->Broadcast( NotificationMessages[0].ToSharedRef() );
			NotificationMessages.RemoveAt( 0 );
		}
	}
}

FReply FFriendsAndChatManager::HandleMessageAccepted( TSharedPtr< FFriendsAndChatMessage > ChatMessage, EFriendsResponseType::Type ResponseType )
{
	switch ( ResponseType )
	{
	case EFriendsResponseType::Response_Accept:
		{
			PendingOutgoingAcceptFriendRequests.Add( ChatMessage->GetUniqueID() );
			TSharedPtr< FFriendStuct > User = FindUser( ChatMessage->GetUniqueID(), EFriendsLists::ToString( EFriendsLists::Default ) );
			if ( User.IsValid() )
			{
				User->SetPendingAccept();
				GenerateFriendsCount();
				RefreshList();
			}
		}
		break;
	case EFriendsResponseType::Response_Reject:
		{
			NotifiedRequest.Remove( ChatMessage->GetUniqueID() );
			TSharedPtr< FFriendStuct > User = FindUser( ChatMessage->GetUniqueID(), EFriendsLists::ToString( EFriendsLists::Default ) );
			if ( User.IsValid() )
			{
				FriendsList.Remove( User );
				GenerateFriendsCount();
				RefreshList();
			}
			PendingOutgoingDeleteFriendRequests.Add( ChatMessage->GetUniqueID() );
		}
		break;
	}

	NotificationMessages.Remove( ChatMessage );

	return FReply::Handled();
}

/** Functor for comparing friends list */
struct FCompareGroupByName
{
	FORCEINLINE bool operator()( const TSharedPtr< FFriendStuct > A, const TSharedPtr< FFriendStuct > B ) const
	{
		check( A.IsValid() );
		check ( B.IsValid() );
		return ( A->GetName() > B->GetName() );
	}
};

bool FFriendsAndChatManager::UpdateFriendsList()
{
	bool bChanged = false;
	// Early check if list has changed
	if ( PendingFriendsList.Num() != FriendsList.Num() )
	{
		bChanged = true;
	}
	else
	{
		// Need to check each item
		FriendsList.Sort( FCompareGroupByName() );
		PendingFriendsList.Sort( FCompareGroupByName() );
		for ( int32 Index = 0; Index < FriendsList.Num(); Index++ )
		{
			if ( PendingFriendsList[Index]->IsUpdated() || *FriendsList[Index].Get() != *PendingFriendsList[Index].Get() )
			{
				bChanged = true;
				break;
			}
		}
	}

	if ( bChanged )
	{
		PendingIncomingInvitesList.Empty();

		for ( int32 Index = 0; Index < PendingFriendsList.Num(); Index++ )
		{
			PendingFriendsList[Index]->ClearUpdated();
			EInviteStatus::Type FriendStatus = PendingFriendsList[ Index ].Get()->GetOnlineFriend()->GetInviteStatus();
			if ( FriendStatus == EInviteStatus::PendingInbound )
			{
				if ( NotifiedRequest.Contains( PendingFriendsList[ Index ].Get()->GetUniqueID() ) == false )
				{
					PendingIncomingInvitesList.Add( PendingFriendsList[ Index ] );
					NotifiedRequest.Add( PendingFriendsList[ Index ]->GetUniqueID() );
				}
			}
		}
		FriendByNameInvites.Empty();
		FriendsList = PendingFriendsList;
		GenerateFriendsCount();
		RefreshList();
	}

	PendingFriendsList.Empty();

	if ( bIsInited == false )
	{
		bIsInited = true;
		FFriendsMessageManager::Get()->SetMessagePolling( true );
	}

	return bChanged;
}

bool FFriendsAndChatManager::UpdateRecentPlayerList()
{
	bool bChanged = false;
	// Early check if list has changed
	if ( PendingFriendsList.Num() != RecentPlayerList.Num() )
	{
		bChanged = true;
	}
	else
	{
		// Need to check each item
		RecentPlayerList.Sort( FCompareGroupByName() );
		PendingFriendsList.Sort( FCompareGroupByName() );
		for ( int32 Index = 0; Index < RecentPlayerList.Num(); Index++ )
		{
			if ( *RecentPlayerList[Index].Get() != *PendingFriendsList[Index].Get() )
			{
				bChanged = true;
				break;
			}
		}
	}

	RecentPlayerList = PendingFriendsList;
	PendingFriendsList.Empty();

	return bChanged;
}

int32 FFriendsAndChatManager::GetFilteredFriendsList( TArray< TSharedPtr< FFriendStuct > >& OutFriendsList )
{
	OutFriendsList = FilteredFriendsList;
	return OutFriendsList.Num();
}

int32 FFriendsAndChatManager::GetFilteredOutgoingFriendsList( TArray< TSharedPtr< FFriendStuct > >& OutFriendsList )
{
	OutFriendsList = FilteredOutgoingList;
	return OutFriendsList.Num();
}

int32 FFriendsAndChatManager::GetFriendCount()
{
	return ConfirmedFriendsCount;
}

void FFriendsAndChatManager::GenerateFriendsCount()
{
	ConfirmedFriendsCount = 0;
	for ( int32 Index = 0; Index < FriendsList.Num(); Index++ )
	{
		if ( FriendsList[Index]->GetInviteStatus() == EInviteStatus::Accepted )
		{
			ConfirmedFriendsCount++;
		}
	}
}

void FFriendsAndChatManager::DeleteFriend( TSharedPtr< FFriendStuct > FriendItem )
{
	TSharedRef<FUniqueNetId> UserID = FriendItem->GetOnlineFriend()->GetUserId();
	NotifiedRequest.Remove( UserID );
	PendingOutgoingDeleteFriendRequests.Add( UserID );
	FriendsList.Remove( FriendItem );
	GenerateFriendsCount();
	RefreshList();
}

FReply FFriendsAndChatManager::OnCloseClicked()
{
	if ( FriendWindow.IsValid() )
	{
		FriendWindow->RequestDestroyWindow();
		FriendWindow = NULL;
	}
	return FReply::Handled();
}

FReply FFriendsAndChatManager::OnMinimizeClicked()
{
	if ( FriendWindow.IsValid() )
	{
		FriendWindow->Minimize();
	}
	return FReply::Handled();
}

FReply FFriendsAndChatManager::OnChatCloseClicked()
{
	if ( ChatWindow.IsValid() )
	{
		ChatWindow->RequestDestroyWindow();
		ChatWindow = NULL;
	}
	return FReply::Handled();
}

FReply FFriendsAndChatManager::OnChatMinimizeClicked()
{
	if ( ChatWindow.IsValid() )
	{
		ChatWindow->Minimize();
	}
	return FReply::Handled();
}

void FFriendsAndChatManager::RefreshList()
{
	switch ( CurrentList )
	{
	case EFriendsDisplayLists::DefaultDisplay:
		{
			BuildFriendsList();
		}
		break;
	case EFriendsDisplayLists::RecentPlayersDisplay:
		{
			BuildRecentPlayersList();
		}
		break;
	case EFriendsDisplayLists::FriendRequestsDisplay:
		{
			BuildRequestIncomingPlayersList();
			BuildRequestOutgoingPlayersList();
		}
		break;
	}
}

void FFriendsAndChatManager::BuildFriendsList()
{
	FilteredFriendsList.Empty();

	for( auto Iter( FriendsList.CreateConstIterator() ); Iter; ++Iter )
	{
		const TSharedPtr< FFriendStuct > Friend = *Iter;
		if ( Friend->GetInviteStatus() == EInviteStatus::Accepted )
		{
			FilteredFriendsList.Add( Friend );
		}
	}

	OnFriendsListUpdatedDelegate.Broadcast();
}

void FFriendsAndChatManager::BuildRecentPlayersList()
{
	FilteredFriendsList.Empty();

	for( auto Iter( RecentPlayerList.CreateConstIterator() ); Iter; ++Iter )
	{
		const TSharedPtr< FFriendStuct > Friend = *Iter;
		FilteredFriendsList.Add( Friend );
	}

	OnFriendsListUpdatedDelegate.Broadcast();
}

void FFriendsAndChatManager::BuildRequestIncomingPlayersList()
{
	FilteredFriendsList.Empty();

	for( auto Iter( FriendsList.CreateConstIterator() ); Iter; ++Iter )
	{
		const TSharedPtr< FFriendStuct > Friend = *Iter;
		if ( Friend->GetInviteStatus() == EInviteStatus::PendingInbound )
		{
			FilteredFriendsList.Add( Friend );
		}
	}
}

void FFriendsAndChatManager::BuildRequestOutgoingPlayersList()
{
	FilteredOutgoingList.Empty();

	for( auto Iter( FriendsList.CreateConstIterator() ); Iter; ++Iter )
	{
		TSharedPtr< FFriendStuct > Friend = *Iter;
		if ( Friend->GetInviteStatus() == EInviteStatus::PendingOutbound )
		{
			FilteredOutgoingList.Add( Friend );
		}
	}

	for( auto Iter( FriendByNameRequests.CreateConstIterator() ); Iter; ++Iter )
	{
		const FString FriendName = *Iter;
		TSharedPtr< FFriendStuct > PendingFriend = MakeShareable( new FFriendStuct( FriendName ) );
		PendingFriend->ClearUpdated();
		PendingFriend->SetPendingInvite();
		FilteredOutgoingList.Add( PendingFriend );
	}

	for( auto Iter( FriendByNameInvites.CreateConstIterator() ); Iter; ++Iter )
	{
		const FString FriendName = *Iter;
		TSharedPtr< FFriendStuct > PendingFriend = MakeShareable( new FFriendStuct( FriendName ) );
		PendingFriend->ClearUpdated();
		PendingFriend->SetPendingInvite();
		FilteredOutgoingList.Add( PendingFriend );
	}

	OnFriendsListUpdatedDelegate.Broadcast();
}

void FFriendsAndChatManager::RequestFriend( const FText& FriendName )
{
	if ( !FriendName.IsEmpty() )
	{
		if ( !FindUserID( FriendName.ToString() ).IsValid() )
		{
			FriendByNameRequests.AddUnique( *FriendName.ToString() );
			RefreshList();
		}
	}
}

void FFriendsAndChatManager::SendFriendRequests()
{
	// Invite Friends
	FOnlineAccountMappingMcpPtr OnlineAccountMappingMcp = OnlineSubMcp->GetMcpAccountMappingService();
	TSharedPtr<FUniqueNetId> UserId = OnlineIdentity->GetUniquePlayerId(0);

	OnlineAccountMappingMcp->AddOnQueryUserIdFromIdentificationStringCompleteDelegate(OnQueryUserIdFromIdentificationStringCompleteDelegate);

	for ( int32 Index = 0; Index < FriendByNameRequests.Num(); Index++ )
	{
		OnlineAccountMappingMcp->QueryUserIdFromIdentificationString( *UserId, FriendByNameRequests[Index] );
	}
}

EFriendsAndManagerState::Type FFriendsAndChatManager::GetManagerState()
{
	return ManagerState;
}

void FFriendsAndChatManager::SetListSelect( EFriendsDisplayLists::Type ListSelectType )
{
	if ( CurrentList != ListSelectType )
	{
		CurrentList = ListSelectType;
		RefreshList();
	}
}

TSharedPtr< FUniqueNetId > FFriendsAndChatManager::FindUserID( const FString& InUsername )
{
	for ( int32 Index = 0; Index < FriendsList.Num(); Index++ )
	{
		if ( FriendsList[ Index ]->GetOnlineUser()->GetDisplayName() == InUsername )
		{
			return FriendsList[ Index ]->GetUniqueID();
		}
	}
	return NULL;
}

bool FFriendsAndChatManager::IsPendingInvite( const FString& InUsername )
{
	return FriendByNameRequests.Contains( InUsername ) || FriendByNameInvites.Contains( InUsername );
}

TSharedPtr< FFriendStuct > FFriendsAndChatManager::FindUser( TSharedRef<FUniqueNetId> InUserID, const FString& ListName )
{
	TArray< TSharedPtr< FFriendStuct > >& SearchList = ListName == EFriendsLists::ToString( EFriendsLists::Default ) ? FriendsList : RecentPlayerList;

	for ( int32 Index = 0; Index < SearchList.Num(); Index++ )
	{
		if ( SearchList[Index]->GetUniqueID().Get() == InUserID.Get() )
		{
			return SearchList[Index];
		}
	}

	return NULL;
}

void FFriendsAndChatManager::SetState( EFriendsAndManagerState::Type NewState )
{
	ManagerState = NewState;

	switch ( NewState )
	{
	case EFriendsAndManagerState::Idle:
		{
			// Do nothing in this state
		}
		break;
	case EFriendsAndManagerState::RequestFriendsListRefresh:
		{
			ReadListRequests.AddUnique( EFriendsLists::Default );
			SetState( EFriendsAndManagerState::RequestingFriendsList );
		}
		break;
	case EFriendsAndManagerState::RequestingFriendsList:
		{
			check( ReadListRequests.Num() > 0 )
 			FriendsInterface->ReadFriendsList( 0, EFriendsLists::ToString( ReadListRequests[0] ) );
		}
		break;
	case EFriendsAndManagerState::ProcessFriendsList:
		{
			if ( UpdateFriendsList() && CurrentList == EFriendsDisplayLists::DefaultDisplay )
			{
				RefreshList();
			}
			ReadListRequests.Remove( EFriendsLists::Default );
			if ( ReadListRequests.Num() > 0 )
			{
				SetState( EFriendsAndManagerState::RequestingFriendsList );
			}
			else
			{
				SetState( EFriendsAndManagerState::Idle );
			}
		}
		break;
	case EFriendsAndManagerState::ProcessRecentPlayersList:
		{
			if ( UpdateRecentPlayerList()  && CurrentList == EFriendsDisplayLists::RecentPlayersDisplay )
			{
				BuildRecentPlayersList();
			}
			ReadListRequests.Remove( EFriendsLists::RecentPlayers );
			if ( ReadListRequests.Num() > 0 )
			{
				SetState( EFriendsAndManagerState::RequestingFriendsList );
			}
			else
			{
				SetState( EFriendsAndManagerState::Idle );
			}
		}
		break;
	case EFriendsAndManagerState::RequestingFriends:
		{
			SendFriendRequests();
		}
		break;
	case EFriendsAndManagerState::DeletingFriends:
		{
			FriendsInterface->AddOnDeleteFriendCompleteDelegate( 0, OnDeleteFriendCompleteDelegate );
			FriendsInterface->DeleteFriend( 0, PendingOutgoingDeleteFriendRequests[0].Get(), EFriendsLists::ToString( EFriendsLists::Default ) );
		}
		break;
	case EFriendsAndManagerState::AcceptingFriendRequest:
		{
			FriendsInterface->AddOnAcceptInviteCompleteDelegate( 0, OnAcceptInviteCompleteDelegate );
			FriendsInterface->AcceptInvite( 0, PendingOutgoingAcceptFriendRequests[0].Get(), EFriendsLists::ToString( EFriendsLists::Default ) );
		}
		break;
	default:
		break;
	}
}

void FFriendsAndChatManager::SendFriendInviteNotification()
{
	check( PendingIncomingInvitesList[ 0 ].IsValid() )

	FString InviteMessage = PendingIncomingInvitesList[ 0 ]->GetName();
	InviteMessage += LOCTEXT( "FFriendsAndChatManager_InviteRequest", " Requested you as a friend" ).ToString();
	TSharedPtr< FFriendsAndChatMessage > ChatMessage = MakeShareable( new FFriendsAndChatMessage( InviteMessage, PendingIncomingInvitesList[0]->GetUniqueID() ) );
	ChatMessage->SetSelfHandle( false );
	ChatMessage->SetButtonCallback( FOnClicked::CreateSP( this, &FFriendsAndChatManager::HandleMessageAccepted, ChatMessage, EFriendsResponseType::Response_Accept ) );
	ChatMessage->SetButtonCallback( FOnClicked::CreateSP( this, &FFriendsAndChatManager::HandleMessageAccepted, ChatMessage, EFriendsResponseType::Response_Reject ) );
	ChatMessage->SetButtonCallback( FOnClicked::CreateSP( this, &FFriendsAndChatManager::HandleMessageAccepted, ChatMessage, EFriendsResponseType::Response_Ignore ) );

	// Only allow one friend notification at a time
	if ( NotificationMessages.Num() == 0 )
	{
		NotificationMessages.Add( ChatMessage );
		FriendsListNotificationDelegate->Broadcast( ChatMessage.ToSharedRef() );
	}
	PendingIncomingInvitesList.RemoveAt( 0 );
}

void FFriendsAndChatManager::OnQueryUserIdFromIdentificationStringComplete(bool bWasSuccessful, const FUniqueNetId& RequestingUserId, const FString& IdentificationString, const FUniqueNetId& IdentifiedUserId, const FString& Error)
{
	check( OnlineSubMcp != NULL && OnlineSubMcp->GetMcpAccountMappingService().IsValid() );

	if ( bWasSuccessful && IdentifiedUserId.IsValid() )
	{
		TSharedPtr<FUniqueNetId> FriendId = OnlineIdentity->CreateUniquePlayerId( IdentifiedUserId.ToString() );
		// Don't allow the user to add themselves as friends
		TSharedPtr<FUniqueNetId> UserId = OnlineIdentity->GetUniquePlayerId(0);
		if ( UserId.IsValid() && OnlineIdentity->GetUserAccount( *UserId )->GetDisplayName() != IdentificationString )
		{
			PendingOutgoingFriendRequests.Add( FriendId.ToSharedRef() );
		}
	}
	FriendByNameInvites.AddUnique( IdentificationString );
	FriendByNameRequests.Remove( IdentificationString );
	if ( FriendByNameRequests.Num() == 0 )
	{
		FOnlineAccountMappingMcpPtr OnlineAccountMappingMcp = OnlineSubMcp->GetMcpAccountMappingService();
		OnlineAccountMappingMcp->ClearOnQueryUserIdFromIdentificationStringCompleteDelegate(OnQueryUserIdFromIdentificationStringCompleteDelegate);

		if ( PendingOutgoingFriendRequests.Num() > 0 )
		{
			FriendsInterface->AddOnSendInviteCompleteDelegate( 0, OnSendInviteCompleteDelegate );
			for ( int32 Index = 0; Index < PendingOutgoingFriendRequests.Num(); Index++ )
			{
				FriendsInterface->SendInvite( 0, PendingOutgoingFriendRequests[Index].Get(), EFriendsLists::ToString( EFriendsLists::Default ) );
			}
		}
		else
		{
			RefreshList();
			SetState( EFriendsAndManagerState::Idle );
		}
	}
}

void FFriendsAndChatManager::OnSendInviteComplete( int32 LocalPlayer, bool bWasSuccessful, const FUniqueNetId& FriendId, const FString& ListName, const FString& ErrorStr )
{
	PendingOutgoingFriendRequests.RemoveAt( 0 );

	if ( PendingOutgoingFriendRequests.Num() == 0 )
	{
		FriendsInterface->ClearOnSendInviteCompleteDelegate( 0, OnSendInviteCompleteDelegate );
		SetState( EFriendsAndManagerState::Idle );
	}
}

void FFriendsAndChatManager::OnReadFriendsComplete( int32 LocalPlayer, bool bWasSuccessful, const FString& ListName, const FString& ErrorStr )
{
	TArray< TSharedRef<FOnlineFriend> > Friends;
	FriendsInterface->GetFriendsList( 0, ListName, Friends );
	
	if ( Friends.Num() > 0 )
	{
		for ( int32 Index = 0; Index < Friends.Num(); Index++ )
		{
			const FOnlineFriend& Friend = *Friends[Index];
			TSharedPtr< FFriendStuct > ExistingFriend = FindUser( Friend.GetUserId(), ListName );
			if ( ExistingFriend.IsValid() )
			{
				if ( Friends[Index]->GetInviteStatus() != ExistingFriend->GetOnlineFriend()->GetInviteStatus() )
				{
					ExistingFriend->SetOnlineFriend( Friends[Index] );
				}
				PendingFriendsList.Add( ExistingFriend );
			}
			else
			{
				QueryUserIds.Add( Friend.GetUserId() );
			}
		}
	}

	check(OnlineSubMcp != NULL && OnlineSubMcp->GetMcpAccountMappingService().IsValid());
	IOnlineUserPtr UserInterface = OnlineSubMcp->GetUserInterface();

	if ( QueryUserIds.Num() > 0 )
	{
		OnlineSubMcp->GetUserInterface()->AddOnQueryUserInfoCompleteDelegate( 0, OnQueryUserInfoCompleteDelegate );
		UserInterface->QueryUserInfo( 0, QueryUserIds );
	}
	else
	{
		if ( ReadListRequests[0] == EFriendsLists::Default )
		{
			SetState( EFriendsAndManagerState::ProcessFriendsList );
		}
		else
		{
			SetState( EFriendsAndManagerState::ProcessRecentPlayersList );
		}
	}
}

void FFriendsAndChatManager::OnQueryUserInfoComplete( int32 LocalPlayer, bool bWasSuccessful, const TArray< TSharedRef<class FUniqueNetId> >& UserIds, const FString& ErrorStr )
{
	check(OnlineSubMcp != NULL && OnlineSubMcp->GetMcpAccountMappingService().IsValid());
	IOnlineUserPtr UserInterface = OnlineSubMcp->GetUserInterface();

	OnlineSubMcp->GetUserInterface()->ClearOnQueryUserInfoCompleteDelegate(0, OnQueryUserInfoCompleteDelegate);

	EFriendsDisplayLists::Type DisplayList = EFriendsDisplayLists::DefaultDisplay;
	if ( ReadListRequests[0] != EFriendsLists::Default )
	{
		DisplayList = EFriendsDisplayLists::RecentPlayersDisplay;
	}

	for ( int32 UserIdx=0; UserIdx < UserIds.Num(); UserIdx++ )
	{
		TSharedPtr<FOnlineFriend> OnlineFirend = FriendsInterface->GetFriend( 0, *UserIds[UserIdx], EFriendsLists::ToString( ReadListRequests[0] ) );
		TSharedPtr<FOnlineUser> OnlineUser = OnlineSubMcp->GetUserInterface()->GetUserInfo( LocalPlayer, *UserIds[UserIdx] );

		if ( OnlineFirend.IsValid() && OnlineUser.IsValid() )
		{
			TSharedPtr< FFriendStuct > FriendItem = MakeShareable( new FFriendStuct( OnlineFirend, OnlineUser, DisplayList ) );
			PendingFriendsList.Add( FriendItem );
		}
		else
		{
			UE_LOG(LogOnline, Log, TEXT("PlayerId=%s not found"), *UserIds[UserIdx]->ToDebugString());
		}
	}

	QueryUserIds.Empty();

	if ( ReadListRequests[0] == EFriendsLists::Default )
	{
		SetState( EFriendsAndManagerState::ProcessFriendsList );
	}
	else
	{
		SetState( EFriendsAndManagerState::ProcessRecentPlayersList );
	}
}

void FFriendsAndChatManager::OnAcceptInviteComplete( int32 LocalPlayer, bool bWasSuccessful, const FUniqueNetId& FriendId, const FString& ListName, const FString& ErrorStr )
{
	PendingOutgoingAcceptFriendRequests.RemoveAt( 0 );

	// Do something with an accepted invite
	FriendsInterface->ClearOnAcceptInviteCompleteDelegate( 0, OnAcceptInviteCompleteDelegate );
	if ( PendingOutgoingAcceptFriendRequests.Num() > 0 )
	{
		SetState( EFriendsAndManagerState::AcceptingFriendRequest );
	}
	else
	{
		RefreshList();
		SetState( EFriendsAndManagerState::Idle );
	}
}

void FFriendsAndChatManager::OnDeleteFriendsListComplete( int32 LocalPlayer, bool bWasSuccessful, const FString& ListName, const FString& ErrorStr )
{
	FriendsInterface->ClearOnDeleteFriendsListCompleteDelegate( 0, OnDeleteFriendsListCompleteDelegate );
	SetState( EFriendsAndManagerState::Idle );
}

void FFriendsAndChatManager::OnDeleteFriendComplete( int32 LocalPlayer, bool bWasSuccessful, const FUniqueNetId& DeletedFriendID, const FString& ListName, const FString& ErrorStr )
{
	PendingOutgoingDeleteFriendRequests.RemoveAt( 0 );

	FriendsInterface->ClearOnDeleteFriendCompleteDelegate( 0, OnDeleteFriendCompleteDelegate );
	if ( PendingOutgoingDeleteFriendRequests.Num() > 0 )
	{
		SetState( EFriendsAndManagerState::DeletingFriends );
	}
	else
	{
		RefreshList();
		SetState( EFriendsAndManagerState::Idle );
	}
}

/* FFriendsAndChatManager system singletons
*****************************************************************************/
TSharedPtr< FFriendsAndChatManager > FFriendsAndChatManager::SingletonInstance = NULL;

TSharedRef< FFriendsAndChatManager > FFriendsAndChatManager::Get()
{
	if ( !SingletonInstance.IsValid() )
	{
		SingletonInstance = MakeShareable( new FFriendsAndChatManager() );
		SingletonInstance->StartupManager();
	}
	return SingletonInstance.ToSharedRef();
}

void FFriendsAndChatManager::Shutdown()
{
	SingletonInstance.Reset();
}

#undef LOCTEXT_NAMESPACE
