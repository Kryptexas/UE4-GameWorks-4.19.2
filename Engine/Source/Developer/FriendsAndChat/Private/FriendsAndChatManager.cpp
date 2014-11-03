// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "SFriendsContainer.h"
#include "FriendsViewModel.h"
#include "ChatViewModel.h"

#define LOCTEXT_NAMESPACE "FriendsAndChatManager"


/* FFriendsAndChatManager structors
 *****************************************************************************/

FFriendsAndChatManager::FFriendsAndChatManager( )
	: ManagerState ( EFriendsAndManagerState::Idle )
	, UpdateTimeInterval( 10.0f )
	, bIsInSession( false )
	, bIsInited( false )
{ }


FFriendsAndChatManager::~FFriendsAndChatManager( )
{ }


/* IFriendsAndChatManager interface
 *****************************************************************************/

void FFriendsAndChatManager::Login()
{
	// Clear existing data
	Logout();

	UpdateTimer = 0.0f;
	bIsInited = false;
	FFriendsMessageManager::Get()->SetMessagePolling( false );

	OnlineSubMcp = static_cast< FOnlineSubsystemMcp* >( IOnlineSubsystem::Get( TEXT( "MCP" ) ) );

	if (OnlineSubMcp != nullptr &&
		OnlineSubMcp->GetMcpAccountMappingService().IsValid() &&
		OnlineSubMcp->GetIdentityInterface().IsValid())
	{
		OnlineIdentity = OnlineSubMcp->GetIdentityInterface();

		IOnlineUserPtr UserInterface = OnlineSubMcp->GetUserInterface();
		check(UserInterface.IsValid());

		FriendsInterface = OnlineSubMcp->GetFriendsInterface();
		check( FriendsInterface.IsValid() )

		// Create delegates for list refreshes
		OnReadFriendsCompleteDelegate = FOnReadFriendsListCompleteDelegate::CreateSP( this, &FFriendsAndChatManager::OnReadFriendsListComplete );
		OnAcceptInviteCompleteDelegate = FOnAcceptInviteCompleteDelegate::CreateSP( this, &FFriendsAndChatManager::OnAcceptInviteComplete );
		OnSendInviteCompleteDelegate = FOnSendInviteCompleteDelegate::CreateSP( this, &FFriendsAndChatManager::OnSendInviteComplete );
		OnDeleteFriendCompleteDelegate = FOnDeleteFriendCompleteDelegate::CreateSP( this, &FFriendsAndChatManager::OnDeleteFriendComplete );
		OnQueryUserIdMappingCompleteDelegate = FOnQueryUserIdMappingCompleteDelegate::CreateSP( this, &FFriendsAndChatManager::OnQueryUserIdMappingComplete );
		OnQueryUserInfoCompleteDelegate = FOnQueryUserInfoCompleteDelegate::CreateSP( this, &FFriendsAndChatManager::OnQueryUserInfoComplete );
		OnPresenceReceivedCompleteDelegate = FOnPresenceReceivedDelegate::CreateSP(this, &FFriendsAndChatManager::OnPresenceReceived);

		FriendsInterface->AddOnReadFriendsListCompleteDelegate( 0, OnReadFriendsCompleteDelegate );
		FriendsInterface->AddOnAcceptInviteCompleteDelegate( 0, OnAcceptInviteCompleteDelegate );
		FriendsInterface->AddOnDeleteFriendCompleteDelegate( 0, OnDeleteFriendCompleteDelegate );
		FriendsInterface->AddOnSendInviteCompleteDelegate( 0, OnSendInviteCompleteDelegate );
		UserInterface->AddOnQueryUserInfoCompleteDelegate(0, OnQueryUserInfoCompleteDelegate);
		OnlineSubMcp->GetPresenceInterface()->AddOnPresenceReceivedDelegate(OnPresenceReceivedCompleteDelegate);

		FOnlinePersonaMcpPtr OnlinePersonaMcp = OnlineSubMcp->GetMcpPersonaService();
		OnlinePersonaMcp->AddOnQueryUserIdMappingCompleteDelegate(OnQueryUserIdMappingCompleteDelegate);

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
			SetState(EFriendsAndManagerState::RequestingFriendName );
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
		FriendsInterface->ClearOnDeleteFriendCompleteDelegate( 0, OnDeleteFriendCompleteDelegate );

		if ( OnlineSubMcp != nullptr )
		{
			OnlineSubMcp->GetPresenceInterface()->ClearOnPresenceReceivedDelegate(OnPresenceReceivedCompleteDelegate);
			FOnlinePersonaMcpPtr OnlinePersonaMcp = OnlineSubMcp->GetMcpPersonaService();
			if (OnlinePersonaMcp.IsValid())
			{
				OnlinePersonaMcp->ClearOnQueryUserIdMappingCompleteDelegate(OnQueryUserIdMappingCompleteDelegate);
				OnlineSubMcp->GetUserInterface()->ClearOnQueryUserInfoCompleteDelegate( 0, OnQueryUserInfoCompleteDelegate );
			}
		}
	}

	FriendsList.Empty();
	PendingFriendsList.Empty();
	FriendByNameRequests.Empty();
	FilteredFriendsList.Empty();
	PendingOutgoingDeleteFriendRequests.Empty();
	PendingOutgoingAcceptFriendRequests.Empty();
	PendingIncomingInvitesList.Empty();
	NotifiedRequest.Empty();

	if ( FriendWindow.IsValid() )
	{
		FriendWindow->RequestDestroyWindow();
		FriendWindow = nullptr;
	}

	if ( ChatWindow.IsValid() )
	{
		ChatWindow->RequestDestroyWindow();
		ChatWindow = nullptr;
	}

	OnlineSubMcp = nullptr;
	if ( UpdateFriendsTickerDelegate.IsBound() )
	{
		FTicker::GetCoreTicker().RemoveTicker( UpdateFriendsTickerDelegate );
	}
}

void FFriendsAndChatManager::CreateFriendsListWidget( TSharedPtr< const SWidget > InParentWidget, const FFriendsAndChatStyle* InStyle )
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
		.SizingRule( ESizingRule::FixedSize )
		.SupportsMaximize( false )
		.SupportsMinimize( false )
		.SupportsTransparency( true )
		.InitialOpacity( 1.0f )
		.bDragAnywhere( true )
		.CreateTitleBar( false );

		BuildFriendsUI();

		if ( ParentWidget.IsValid() )
		{
			FWidgetPath WidgetPath;
			FSlateApplication::Get().GeneratePathToWidgetChecked( ParentWidget.ToSharedRef(), WidgetPath );
			FriendWindow = FSlateApplication::Get().AddWindowAsNativeChild( FriendWindow.ToSharedRef(), WidgetPath.GetWindow() );
		}
	}
	else if(!FriendWindow->IsVisible())
	{
		FriendWindow->Restore();
		BuildFriendsUI();
	}

	// Clear notifications
	OnFriendsNotification().Broadcast(false);
}


void FFriendsAndChatManager::SetInSession( bool bInSession )
{
	bIsInSession = bInSession;
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
		SAssignNew(FriendListWidget, SFriendsContainer, FFriendsViewModelFactory::Create(SharedThis(this)))
		.FriendStyle( &Style )
		.Method(SMenuAnchor::UseCurrentWindow);
	}

	// Clear notifications
	OnFriendsNotification().Broadcast(false);

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
				SNew(SChatWindow, FChatViewModelFactory::Create())
				.FriendStyle( &Style )
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
	PendingOutgoingAcceptFriendRequests.Add( FUniqueNetIdString(FriendItem->GetOnlineUser()->GetUserId()->ToString()) );
	FriendItem->SetPendingAccept();
	RefreshList();
	OnFriendsNotification().Broadcast(false);
}


void FFriendsAndChatManager::RejectFriend( TSharedPtr< FFriendStuct > FriendItem )
{
	NotifiedRequest.Remove( FriendItem->GetOnlineUser()->GetUserId() );
	PendingOutgoingDeleteFriendRequests.Add(FUniqueNetIdString(FriendItem->GetOnlineUser()->GetUserId().Get().ToString()));
	FriendsList.Remove( FriendItem );
	RefreshList();
	OnFriendsNotification().Broadcast(false);
}

FReply FFriendsAndChatManager::HandleMessageAccepted( TSharedPtr< FFriendsAndChatMessage > ChatMessage, EFriendsResponseType::Type ResponseType )
{
	switch ( ResponseType )
	{
	case EFriendsResponseType::Response_Accept:
		{
			PendingOutgoingAcceptFriendRequests.Add(FUniqueNetIdString(ChatMessage->GetUniqueID()->ToString()));
			TSharedPtr< FFriendStuct > User = FindUser(ChatMessage->GetUniqueID());
			if ( User.IsValid() )
			{
				User->SetPendingAccept();
				RefreshList();
			}
		}
		break;
	case EFriendsResponseType::Response_Reject:
		{
			NotifiedRequest.Remove( ChatMessage->GetUniqueID() );
			TSharedPtr< FFriendStuct > User = FindUser( ChatMessage->GetUniqueID());
			if ( User.IsValid() )
			{
				FriendsList.Remove( User );
				RefreshList();
			}
			PendingOutgoingDeleteFriendRequests.Add(FUniqueNetIdString(ChatMessage->GetUniqueID().Get().ToString()));
		}
		break;
	}

	NotificationMessages.Remove( ChatMessage );

	return FReply::Handled();
}

bool FFriendsAndChatManager::UpdateFriendsList()
{
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
	}

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

void FFriendsAndChatManager::DeleteFriend( TSharedPtr< FFriendStuct > FriendItem )
{
	TSharedRef<FUniqueNetId> UserID = FriendItem->GetOnlineFriend()->GetUserId();
	NotifiedRequest.Remove( UserID );
	PendingOutgoingDeleteFriendRequests.Add(FUniqueNetIdString(UserID.Get().ToString()));
	FriendsList.Remove( FriendItem );
	FriendItem->SetPendingDelete();
	RefreshList();
	// Clear notifications
	OnFriendsNotification().Broadcast(false);
}


FReply FFriendsAndChatManager::OnCloseClicked()
{
	if ( FriendWindow.IsValid() )
	{
		FriendWindow->RequestDestroyWindow();
		FriendWindow = nullptr;
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
		ChatWindow = nullptr;
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
	BuildFriendsList();
}

void FFriendsAndChatManager::BuildFriendsList()
{
	FilteredFriendsList.Empty();

	for( const auto& Friend : FriendsList)
	{
		if( !Friend->IsPendingDelete())
		{
			FilteredFriendsList.Add( Friend );
		}
	}

	OnFriendsListUpdatedDelegate.Broadcast();
}

void FFriendsAndChatManager::BuildFriendsUI()
{
	check(FriendWindow.IsValid());

	FriendWindow->SetContent(
	SNew(SBorder)
	.BorderImage( &Style.Background )
	.VAlign( VAlign_Fill )
	.HAlign( HAlign_Fill )
	.Padding(0)
	[
		SNew(SFriendsContainer, FFriendsViewModelFactory::Create(SharedThis(this)))
		.FriendStyle( &Style )
		.OnCloseClicked( this, &FFriendsAndChatManager::OnCloseClicked )
		.OnMinimizeClicked( this, &FFriendsAndChatManager::OnMinimizeClicked )
		.Method(SMenuAnchor::CreateNewWindow)
	]);
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
	FOnlinePersonaMcpPtr OnlinePersonaMcp = OnlineSubMcp->GetMcpPersonaService();
	TSharedPtr<FUniqueNetId> UserId = OnlineIdentity->GetUniquePlayerId(0);

	for ( int32 Index = 0; Index < FriendByNameRequests.Num(); Index++ )
	{
		OnlinePersonaMcp->QueryUserIdMapping(*UserId, FriendByNameRequests[Index]);
	}
}


EFriendsAndManagerState::Type FFriendsAndChatManager::GetManagerState()
{
	return ManagerState;
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
	return nullptr;
}


bool FFriendsAndChatManager::IsPendingInvite( const FString& InUsername )
{
	return FriendByNameRequests.Contains( InUsername ) || FriendByNameInvites.Contains( InUsername );
}


TSharedPtr< FFriendStuct > FFriendsAndChatManager::FindUser( TSharedRef<FUniqueNetId> InUserID)
{
	for ( const auto& Friend : FriendsList)
	{
		if ( Friend->GetUniqueID().Get() == InUserID.Get() )
		{
			return Friend;
		}
	}

	return nullptr;
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
 			if(FriendsInterface->ReadFriendsList( 0, EFriendsLists::ToString( EFriendsLists::Default ) ))
			{
				SetState(EFriendsAndManagerState::RequestingFriendsList);
			}
			else
			{
				SetState(EFriendsAndManagerState::Idle);
			}
		}
		break;
	case EFriendsAndManagerState::RequestingFriendsList:
		{
			// Do Nothing
		}
		break;
	case EFriendsAndManagerState::ProcessFriendsList:
		{
			if ( UpdateFriendsList() )
			{
				RefreshList();
			}
			SetState( EFriendsAndManagerState::Idle );
		}
		break;
	case EFriendsAndManagerState::RequestingFriendName:
		{
			SendFriendRequests();
		}
		break;
	case EFriendsAndManagerState::DeletingFriends:
		{
			FriendsInterface->DeleteFriend( 0, PendingOutgoingDeleteFriendRequests[0], EFriendsLists::ToString( EFriendsLists::Default ) );
		}
		break;
	case EFriendsAndManagerState::AcceptingFriendRequest:
		{
			FriendsInterface->AcceptInvite( 0, PendingOutgoingAcceptFriendRequests[0], EFriendsLists::ToString( EFriendsLists::Default ) );
		}
		break;
	default:
		break;
	}
}


void FFriendsAndChatManager::SendFriendInviteNotification()
{
	PendingIncomingInvitesList.Empty();
	FriendsListNotificationDelegate.Broadcast(true);
}


void FFriendsAndChatManager::OnQueryUserIdMappingComplete(bool bWasSuccessful, const FUniqueNetId& RequestingUserId, const FString& DisplayName, const FUniqueNetId& IdentifiedUserId, const FString& Error)
{
	check( OnlineSubMcp != nullptr && OnlineSubMcp->GetMcpPersonaService().IsValid() );

	if ( bWasSuccessful && IdentifiedUserId.IsValid() )
	{
		TSharedPtr<FUniqueNetId> FriendId = OnlineIdentity->CreateUniquePlayerId( IdentifiedUserId.ToString() );
		// Don't allow the user to add themselves as friends
		TSharedPtr<FUniqueNetId> UserId = OnlineIdentity->GetUniquePlayerId(0);
		if ( UserId.IsValid() && OnlineIdentity->GetUserAccount( *UserId )->GetDisplayName() != DisplayName )
		{
			PendingOutgoingFriendRequests.Add( FriendId.ToSharedRef() );
		}
		FriendByNameInvites.AddUnique(DisplayName);
	}

	FriendByNameRequests.Remove( DisplayName );
	if ( FriendByNameRequests.Num() == 0 )
	{
		if ( PendingOutgoingFriendRequests.Num() > 0 )
		{
			for ( int32 Index = 0; Index < PendingOutgoingFriendRequests.Num(); Index++ )
			{
				FriendsInterface->SendInvite( 0, PendingOutgoingFriendRequests[Index].Get(), EFriendsLists::ToString( EFriendsLists::Default ) );
			}
		}
		else
		{
			RefreshList();
			SetState(EFriendsAndManagerState::Idle);
		}
	}
}


void FFriendsAndChatManager::OnSendInviteComplete( int32 LocalPlayer, bool bWasSuccessful, const FUniqueNetId& FriendId, const FString& ListName, const FString& ErrorStr )
{
	PendingOutgoingFriendRequests.RemoveAt( 0 );

	if ( PendingOutgoingFriendRequests.Num() == 0 )
	{
		RefreshList();
		SetState(EFriendsAndManagerState::Idle);
	}
}


void FFriendsAndChatManager::OnReadFriendsListComplete( int32 LocalPlayer, bool bWasSuccessful, const FString& ListName, const FString& ErrorStr )
{
	TArray< TSharedRef<FOnlineFriend> > Friends;
	bool bReadyToChangeState = true;

	if ( FriendsInterface->GetFriendsList(0, ListName, Friends) )
	{
		if (Friends.Num() > 0)
		{
			for ( const auto& Friend : Friends)
			{
				TSharedPtr< FFriendStuct > ExistingFriend = FindUser(Friend->GetUserId());
				if ( ExistingFriend.IsValid() )
				{
					if (Friend->GetInviteStatus() != ExistingFriend->GetOnlineFriend()->GetInviteStatus() || ExistingFriend->IsPendingAccepted() && Friend->GetInviteStatus() == EInviteStatus::Accepted)
					{
						ExistingFriend->SetOnlineFriend(Friend);
					}
					PendingFriendsList.Add(ExistingFriend);
				}
				else
				{
					QueryUserIds.Add(Friend->GetUserId());
				}
			}
		}

		check(OnlineSubMcp != nullptr && OnlineSubMcp->GetMcpPersonaService().IsValid());
		IOnlineUserPtr UserInterface = OnlineSubMcp->GetUserInterface();

		if ( QueryUserIds.Num() > 0 )
		{
			UserInterface->QueryUserInfo(0, QueryUserIds);
			// OnQueryUserInfoComplete will handle state change
			bReadyToChangeState = false;
		}
	}
	else
	{
		UE_LOG(LogOnline, Warning, TEXT("Failed to obtain Friends List %s"), *ListName);
	}

	if (bReadyToChangeState)
	{
		SetState(EFriendsAndManagerState::ProcessFriendsList);
	}
}


void FFriendsAndChatManager::OnQueryUserInfoComplete( int32 LocalPlayer, bool bWasSuccessful, const TArray< TSharedRef<class FUniqueNetId> >& UserIds, const FString& ErrorStr )
{
	check(OnlineSubMcp != nullptr && OnlineSubMcp->GetMcpPersonaService().IsValid());
	IOnlineUserPtr UserInterface = OnlineSubMcp->GetUserInterface();

	for ( int32 UserIdx=0; UserIdx < UserIds.Num(); UserIdx++ )
	{
		TSharedPtr<FOnlineFriend> OnlineFriend = FriendsInterface->GetFriend( 0, *UserIds[UserIdx], EFriendsLists::ToString( EFriendsLists::Default ) );
		TSharedPtr<FOnlineUser> OnlineUser = OnlineSubMcp->GetUserInterface()->GetUserInfo( LocalPlayer, *UserIds[UserIdx] );

		if (OnlineFriend.IsValid() && OnlineUser.IsValid())
		{
			TSharedPtr< FFriendStuct > FriendItem = MakeShareable(new FFriendStuct(OnlineFriend, OnlineUser, EFriendsDisplayLists::DefaultDisplay));
			PendingFriendsList.Add( FriendItem );
		}
		else
		{
			UE_LOG(LogOnline, Log, TEXT("PlayerId=%s not found"), *UserIds[UserIdx]->ToDebugString());
		}
	}

	QueryUserIds.Empty();

	SetState( EFriendsAndManagerState::ProcessFriendsList );
}

void FFriendsAndChatManager::OnPresenceReceived( const FUniqueNetId& UserId, const TSharedRef<FOnlineUserPresence>& Presence)
{
	for (const auto& Friend : FriendsList)
	{
		if( Friend->GetUniqueID().Get() == UserId)
		{
			// May need to do something here
		}
	}
}

void FFriendsAndChatManager::OnAcceptInviteComplete( int32 LocalPlayer, bool bWasSuccessful, const FUniqueNetId& FriendId, const FString& ListName, const FString& ErrorStr )
{
	PendingOutgoingAcceptFriendRequests.Remove(FUniqueNetIdString(FriendId.ToString()));

	// Do something with an accepted invite
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

void FFriendsAndChatManager::OnDeleteFriendComplete( int32 LocalPlayer, bool bWasSuccessful, const FUniqueNetId& DeletedFriendID, const FString& ListName, const FString& ErrorStr )
{
	PendingOutgoingDeleteFriendRequests.Remove(FUniqueNetIdString(DeletedFriendID.ToString()));

	RefreshList();

	if ( PendingOutgoingDeleteFriendRequests.Num() > 0 )
	{
		SetState( EFriendsAndManagerState::DeletingFriends );
	}
	else
	{
		SetState(EFriendsAndManagerState::Idle);
	}
}


/* FFriendsAndChatManager system singletons
*****************************************************************************/

TSharedPtr< FFriendsAndChatManager > FFriendsAndChatManager::SingletonInstance = nullptr;

TSharedRef< FFriendsAndChatManager > FFriendsAndChatManager::Get()
{
	if ( !SingletonInstance.IsValid() )
	{
		SingletonInstance = MakeShareable( new FFriendsAndChatManager() );
	}
	return SingletonInstance.ToSharedRef();
}


void FFriendsAndChatManager::Shutdown()
{
	SingletonInstance.Reset();
}


#undef LOCTEXT_NAMESPACE
