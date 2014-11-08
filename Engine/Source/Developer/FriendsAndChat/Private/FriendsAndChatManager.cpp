// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "SFriendsContainer.h"
#include "SChatWindow.h"
#include "FriendsViewModel.h"
#include "ChatViewModel.h"
#include "SNotificationList.h"
#include "SWindowTitleBar.h"

#define LOCTEXT_NAMESPACE "FriendsAndChatManager"


/* FFriendsAndChatManager structors
 *****************************************************************************/

FFriendsAndChatManager::FFriendsAndChatManager( )
	: MessageManager(FFriendsMessageManagerFactory::Create())
	, ManagerState ( EFriendsAndManagerState::Idle )
	, bIsInSession( false )
	, bIsInited( false )
	, bRequiresListRefresh(false)
{
}


FFriendsAndChatManager::~FFriendsAndChatManager( )
{ }


/* IFriendsAndChatManager interface
 *****************************************************************************/

void FFriendsAndChatManager::Login()
{
	// Clear existing data
	Logout();

	bIsInited = false;

	OnlineSubMcp = static_cast< FOnlineSubsystemMcp* >( IOnlineSubsystem::Get( TEXT( "MCP" ) ) );

	if (OnlineSubMcp != nullptr &&
		OnlineSubMcp->GetMcpAccountMappingService().IsValid() &&
		OnlineSubMcp->GetIdentityInterface().IsValid())
	{
		OnlineIdentity = OnlineSubMcp->GetIdentityInterface();

		if(OnlineIdentity->GetUniquePlayerId(0).IsValid())
		{
			IOnlineUserPtr UserInterface = OnlineSubMcp->GetUserInterface();
			check(UserInterface.IsValid());

			FriendsInterface = OnlineSubMcp->GetFriendsInterface();
			check( FriendsInterface.IsValid() )

			// Create delegates for list refreshes
			OnReadFriendsCompleteDelegate = FOnReadFriendsListCompleteDelegate::CreateSP( this, &FFriendsAndChatManager::OnReadFriendsListComplete );
			OnFriendsListChangedDelegate = FOnFriendsChangeDelegate::CreateSP(this, &FFriendsAndChatManager::OnFriendsListChanged);
			OnAcceptInviteCompleteDelegate = FOnAcceptInviteCompleteDelegate::CreateSP( this, &FFriendsAndChatManager::OnAcceptInviteComplete );
			OnSendInviteCompleteDelegate = FOnSendInviteCompleteDelegate::CreateSP( this, &FFriendsAndChatManager::OnSendInviteComplete );
			OnDeleteFriendCompleteDelegate = FOnDeleteFriendCompleteDelegate::CreateSP( this, &FFriendsAndChatManager::OnDeleteFriendComplete );
			OnQueryUserIdMappingCompleteDelegate = FOnQueryUserIdMappingCompleteDelegate::CreateSP( this, &FFriendsAndChatManager::OnQueryUserIdMappingComplete );
			OnQueryUserInfoCompleteDelegate = FOnQueryUserInfoCompleteDelegate::CreateSP( this, &FFriendsAndChatManager::OnQueryUserInfoComplete );
			OnPresenceReceivedCompleteDelegate = FOnPresenceReceivedDelegate::CreateSP(this, &FFriendsAndChatManager::OnPresenceReceived);
			OnPresenceUpdatedCompleteDelegate = IOnlinePresence::FOnPresenceTaskCompleteDelegate::CreateSP(this, &FFriendsAndChatManager::OnPresenceUpdated);
			OnFriendInviteReceivedDelegate = FOnInviteReceivedDelegate::CreateSP(this, &FFriendsAndChatManager::OnFriendInviteReceived);
			OnFriendRemovedDelegate = FOnFriendRemovedDelegate::CreateSP(this, &FFriendsAndChatManager::OnFriendRemoved);
			OnFriendInviteRejected = FOnInviteRejectedDelegate::CreateSP(this, &FFriendsAndChatManager::OnInviteRejected);
			OnFriendInviteAccepted = FOnInviteAcceptedDelegate::CreateSP(this, &FFriendsAndChatManager::OnInviteAccepted);

			FriendsInterface->AddOnFriendsChangeDelegate(0, OnFriendsListChangedDelegate);
			FriendsInterface->AddOnInviteReceivedDelegate(OnFriendInviteReceivedDelegate);
			FriendsInterface->AddOnFriendRemovedDelegate(OnFriendRemovedDelegate);
			FriendsInterface->AddOnInviteRejectedDelegate(OnFriendInviteRejected);
			FriendsInterface->AddOnInviteAcceptedDelegate(OnFriendInviteAccepted);
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

			SetState(EFriendsAndManagerState::RequestFriendsListRefresh);

			MessageManager->LogIn();
			for (auto RoomName : ChatRoomstoJoin)
			{
				MessageManager->JoinPublicRoom(RoomName);
			}
		}
		else
		{
			SetState(EFriendsAndManagerState::OffLine);
		}
	}
}

void FFriendsAndChatManager::Logout()
{
	if ( FriendsInterface.IsValid() )
	{
		FriendsInterface->ClearOnFriendsChangeDelegate(0, OnFriendsListChangedDelegate);
		FriendsInterface->ClearOnInviteReceivedDelegate(OnFriendInviteReceivedDelegate);
		FriendsInterface->ClearOnFriendRemovedDelegate(OnFriendRemovedDelegate);
		FriendsInterface->ClearOnInviteRejectedDelegate(OnFriendInviteRejected);
		FriendsInterface->ClearOnInviteAcceptedDelegate(OnFriendInviteAccepted);
		FriendsInterface->ClearOnReadFriendsListCompleteDelegate( 0, OnReadFriendsCompleteDelegate );
		FriendsInterface->ClearOnAcceptInviteCompleteDelegate( 0, OnAcceptInviteCompleteDelegate );
		FriendsInterface->ClearOnDeleteFriendCompleteDelegate( 0, OnDeleteFriendCompleteDelegate );
		FriendsInterface->ClearOnSendInviteCompleteDelegate( 0, OnSendInviteCompleteDelegate );

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

	MessageManager->LogOut();

	OnlineSubMcp = nullptr;
	if ( UpdateFriendsTickerDelegate.IsBound() )
	{
		FTicker::GetCoreTicker().RemoveTicker( UpdateFriendsTickerDelegate );
	}
}

void FFriendsAndChatManager::SetUserSettings(FFriendsAndChatSettings UserSettings)
{
	this->UserSettings = UserSettings;
}


void FFriendsAndChatManager::SetInSession( bool bInSession )
{
	bIsInSession = bInSession;
}

void FFriendsAndChatManager::InsertNetworkChatMessage(const FString InMessage)
{

}

void FFriendsAndChatManager::JoinPublicChatRoom(const FString& RoomName)
{
	if (!RoomName.IsEmpty())
	{
		ChatRoomstoJoin.AddUnique(RoomName);
		if (MessageManager.IsValid())
		{
			MessageManager->JoinPublicRoom(RoomName);
		}
	}
}

// UI Creation

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

void FFriendsAndChatManager::BuildFriendsUI()
{
	check(FriendWindow.IsValid());

	TSharedPtr<SWindowTitleBar> TitleBar;
	FriendWindow->SetContent(
		SNew(SBorder)
		.BorderImage( &Style.Background )
		.VAlign( VAlign_Fill )
		.HAlign( HAlign_Fill )
		.Padding(0)
		[
			SNew(SOverlay)
			+SOverlay::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(TitleBar, SWindowTitleBar, FriendWindow.ToSharedRef(), nullptr, HAlign_Center)
					//.Style(FPortalStyle::Get(), "Window")
					.ShowAppIcon(false)
					.Title(FText::GetEmpty())
				]
				+ SVerticalBox::Slot()
				[
					SNew(SFriendsContainer, FFriendsViewModelFactory::Create(SharedThis(this)))
					.FriendStyle( &Style )
					.Method(SMenuAnchor::CreateNewWindow)
				]
			]
			+SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Bottom)
			[
				SAssignNew(FriendsNotificationBox, SNotificationList)
			]
		]
	);
}


TSharedPtr< SWidget > FFriendsAndChatManager::GenerateFriendsListWidget( const FFriendsAndChatStyle* InStyle )
{
	if ( !FriendListWidget.IsValid() )
	{
		Style = *InStyle;
		SAssignNew(FriendListWidget, SOverlay)
		+SOverlay::Slot()
		[
			 SNew(SFriendsContainer, FFriendsViewModelFactory::Create(SharedThis(this)))
			.FriendStyle( &Style )
			.Method(SMenuAnchor::UseCurrentWindow)
		]
		+SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Bottom)
		[
			SAssignNew(FriendsNotificationBox, SNotificationList)
		];
	}

	// Clear notifications
	OnFriendsNotification().Broadcast(false);

	return FriendListWidget;
}


TSharedPtr< SWidget > FFriendsAndChatManager::GenerateChatWidget(const FFriendsAndChatStyle* InStyle)
{
	check(MessageManager.IsValid());
	if ( !ChatWidget.IsValid() )
	{
		if( !ChatViewModel.IsValid())
		{
			ChatViewModel = FChatViewModelFactory::Create(MessageManager.ToSharedRef());
		}

		Style = *InStyle;
		SAssignNew(ChatWidget, SChatWindow, ChatViewModel.ToSharedRef())
		.FriendStyle( &Style )
		.Method(SMenuAnchor::UseCurrentWindow);
	}
	return ChatWidget;
}


void FFriendsAndChatManager::GenerateChatWindow( TSharedPtr< FFriendStuct > FriendItem )
{
	const FVector2D DEFAULT_WINDOW_SIZE = FVector2D(400, 300);

	check(MessageManager.IsValid());

	if(!ChatViewModel.IsValid())
	{
		ChatViewModel = FChatViewModelFactory::Create(MessageManager.ToSharedRef());
	}

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
		.SizingRule( ESizingRule::FixedSize );

		TSharedPtr<SWindowTitleBar> TitleBar;

		ChatWindow->SetContent(
			SNew( SBorder )
			.VAlign( VAlign_Fill )
			.HAlign( HAlign_Fill )
			.BorderImage( &Style.Background )
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(TitleBar, SWindowTitleBar, ChatWindow.ToSharedRef(), nullptr, HAlign_Center)
					//.Style(FPortalStyle::Get(), "Window")
					.ShowAppIcon(false)
					.Title(FText::GetEmpty())
				]
				+ SVerticalBox::Slot()
				[
					SNew(SChatWindow, ChatViewModel.ToSharedRef())
					.FriendStyle( &Style )
				]
			]);

		if ( ParentWidget.IsValid() )
		{
			FWidgetPath WidgetPath;
			FSlateApplication::Get().GeneratePathToWidgetChecked( ParentWidget.ToSharedRef(), WidgetPath );
			ChatWindow = FSlateApplication::Get().AddWindowAsNativeChild( ChatWindow.ToSharedRef(), WidgetPath.GetWindow() );
		}
	}

	ChatViewModel->SetChatFriend(FriendItem);
}

// Actions

void FFriendsAndChatManager::SetUserIsOnline(bool bIsOnline)
{
	if ( OnlineSubMcp != nullptr )
	{
		TSharedPtr<FUniqueNetId> UserId = OnlineIdentity->GetUniquePlayerId(0);
		FOnlineUserPresenceStatus NewStatus;
		NewStatus.State = bIsOnline ? EOnlinePresenceState::Online : EOnlinePresenceState::Away;
		OnlineSubMcp->GetPresenceInterface()->SetPresence(*UserId.Get(), NewStatus, OnPresenceUpdatedCompleteDelegate);
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

void FFriendsAndChatManager::RequestFriend( const FText& FriendName )
{
	if ( !FriendName.IsEmpty() )
	{
		if ( !FindUserID( FriendName.ToString() ).IsValid() )
		{
			FriendByNameRequests.AddUnique( *FriendName.ToString() );
		}
		else
		{
			AddFriendsToast(FText::FromString("Friend already requested"));
		}
	}
}

// Process action responses

FReply FFriendsAndChatManager::HandleMessageAccepted( TSharedPtr< FFriendsAndChatMessage > ChatMessage, EFriendsResponseType::Type ResponseType )
{
	switch ( ResponseType )
	{
	case EFriendsResponseType::Response_Accept:
		{
			PendingOutgoingAcceptFriendRequests.Add(FUniqueNetIdString(ChatMessage->GetUniqueID()->ToString()));
			TSharedPtr< FFriendStuct > User = FindUser(ChatMessage->GetUniqueID().Get());
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
			TSharedPtr< FFriendStuct > User = FindUser( ChatMessage->GetUniqueID().Get());
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

// Getters

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

bool FFriendsAndChatManager::IsInSession()
{
	return bIsInSession;
}

bool FFriendsAndChatManager::GetUserIsOnline()
{
	if (OnlineSubMcp != nullptr)
	{
		TSharedPtr<FOnlineUserPresence> Presence;
		TSharedPtr<FUniqueNetId> UserId = OnlineIdentity->GetUniquePlayerId(0);
		if(UserId.IsValid())
		{
			OnlineSubMcp->GetPresenceInterface()->GetCachedPresence(*UserId.Get(), Presence);
			if(Presence.IsValid())
			{
				return Presence->Status.State == EOnlinePresenceState::Online;
			}
		}
	}
	return false;
}

// List processing

void FFriendsAndChatManager::RequestListRefresh()
{
	bRequiresListRefresh = true;
}

bool FFriendsAndChatManager::Tick( float Delta )
{
	if ( ManagerState == EFriendsAndManagerState::Idle )
	{
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
		else if (bRequiresListRefresh)
		{
			bRequiresListRefresh = false;
			SetState( EFriendsAndManagerState::RequestFriendsListRefresh );
		}
	}
	return true;
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
				RequestListRefresh();
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
			if ( ProcessFriendsList() )
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

void FFriendsAndChatManager::OnReadFriendsListComplete( int32 LocalPlayer, bool bWasSuccessful, const FString& ListName, const FString& ErrorStr )
{
	PreProcessList(ListName);
}

void FFriendsAndChatManager::PreProcessList(const FString& ListName)
{
	TArray< TSharedRef<FOnlineFriend> > Friends;
	bool bReadyToChangeState = true;

	if ( FriendsInterface->GetFriendsList(0, ListName, Friends) )
	{
		if (Friends.Num() > 0)
		{
			for ( const auto& Friend : Friends)
			{
				TSharedPtr< FFriendStuct > ExistingFriend = FindUser(Friend->GetUserId().Get());
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
	if( ManagerState == EFriendsAndManagerState::RequestingFriendsList)
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
}

bool FFriendsAndChatManager::ProcessFriendsList()
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

void FFriendsAndChatManager::RefreshList()
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

TSharedPtr< FFriendStuct > FFriendsAndChatManager::FindUser(const FUniqueNetId& InUserID)
{
	for ( const auto& Friend : FriendsList)
	{
		if (Friend->GetUniqueID().Get() == InUserID)
		{
			return Friend;
		}
	}

	return nullptr;
}

void FFriendsAndChatManager::SendFriendInviteNotification()
{
	for( const auto& FriendRequest : PendingIncomingInvitesList)
	{
		if(FriendsListActionNotificationDelegate.IsBound())
		{
			FFormatNamedArguments Args;
			Args.Add(TEXT("Username"), FText::FromString(FriendRequest->GetName()));
			const FText FriendRequestMessage = FText::Format(LOCTEXT("FFriendsAndChatManager_AddedYou", "Friend request from {Username}"), Args);

			TSharedPtr< FFriendsAndChatMessage > NotificationMessage = MakeShareable(new FFriendsAndChatMessage(FriendRequestMessage.ToString(), FriendRequest->GetUniqueID()));
			NotificationMessage->SetButtonCallback( FOnClicked::CreateSP(this, &FFriendsAndChatManager::HandleMessageAccepted, NotificationMessage, EFriendsResponseType::Response_Accept));
			NotificationMessage->SetMessageType(EFriendsRequestType::FriendInvite);
			FriendsListActionNotificationDelegate.Broadcast(NotificationMessage.ToSharedRef());
		}
	}

	PendingIncomingInvitesList.Empty();
	FriendsListNotificationDelegate.Broadcast(true);
}

void FFriendsAndChatManager::SendInviteAcceptedNotification(const TSharedPtr< FFriendStuct > Friend)
{
	if(FriendsListActionNotificationDelegate.IsBound())
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("Username"), FText::FromString(Friend->GetName()));
		const FText FriendRequestMessage = FText::Format(LOCTEXT("FFriendsAndChatManager_Accepted", "{Username} accepted your request"), Args);

		TSharedPtr< FFriendsAndChatMessage > NotificationMessage = MakeShareable(new FFriendsAndChatMessage(FriendRequestMessage.ToString()));
		NotificationMessage->SetMessageType(EFriendsRequestType::FriendAccepted);
		OnFriendsActionNotification().Broadcast(NotificationMessage.ToSharedRef());
	}
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
	else
	{
		const FString DiplayMessage = DisplayName +  TEXT(" not found");
		AddFriendsToast(FText::FromString(DiplayMessage));
	}

	FriendByNameRequests.Remove( DisplayName );
	if ( FriendByNameRequests.Num() == 0 )
	{
		if ( PendingOutgoingFriendRequests.Num() > 0 )
		{
			for ( int32 Index = 0; Index < PendingOutgoingFriendRequests.Num(); Index++ )
			{
				FriendsInterface->SendInvite( 0, PendingOutgoingFriendRequests[Index].Get(), EFriendsLists::ToString( EFriendsLists::Default ) );
				AddFriendsToast(FText::FromString("Request Sent"));
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
		RequestListRefresh();
		SetState(EFriendsAndManagerState::Idle);
	}
}

void FFriendsAndChatManager::OnPresenceReceived( const FUniqueNetId& UserId, const TSharedRef<FOnlineUserPresence>& Presence)
{
	for (const auto& Friend : FriendsList)
	{
		if( Friend->GetUniqueID().Get() == UserId)
		{
			// TODO: May need to do something here
		}
	}
}

void FFriendsAndChatManager::OnPresenceUpdated(const FUniqueNetId& UserId, const bool bWasSuccessful)
{
	// TODO: May need to do something here
}

void FFriendsAndChatManager::OnFriendsListChanged()
{
	// TODO: May need to do something here
}

void FFriendsAndChatManager::OnFriendInviteReceived(const FUniqueNetId& UserId, const FUniqueNetId& FriendId)
{
	RequestListRefresh();
}

void FFriendsAndChatManager::OnFriendRemoved(const FUniqueNetId& UserId, const FUniqueNetId& FriendId)
{
	RequestListRefresh();
}

void FFriendsAndChatManager::OnInviteRejected(const FUniqueNetId& UserId, const FUniqueNetId& FriendId)
{
	RequestListRefresh();
}

void FFriendsAndChatManager::OnInviteAccepted(const FUniqueNetId& UserId, const FUniqueNetId& FriendId)
{
	TSharedPtr< FFriendStuct > Friend = FindUser(FriendId);
	if(Friend.IsValid())
	{
		Friend->SetPendingAccept();
		SendInviteAcceptedNotification(Friend);
	}
	RefreshList();
	RequestListRefresh();
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
		RequestListRefresh();
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

void FFriendsAndChatManager::AddFriendsToast(const FText Message)
{
	if( FriendsNotificationBox.IsValid())
	{
		FNotificationInfo Info(Message);
		Info.ExpireDuration = 2.0f;
		Info.bUseLargeFont = false;
		FriendsNotificationBox->AddNotification(Info);
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
