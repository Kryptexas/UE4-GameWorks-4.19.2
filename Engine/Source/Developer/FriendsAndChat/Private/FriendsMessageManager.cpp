// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	FriendsMessageManager.cpp: Implements the FFriendsMessageManager class.
=============================================================================*/

#include "FriendsAndChatPrivatePCH.h"

#define LOCTEXT_NAMESPACE "FriendsMessageManager"

/* FFriendsMessageManager structors
 *****************************************************************************/

FFriendsMessageManager::FFriendsMessageManager( )
	: bClearInvites( false )
	, PingMcpInterval( 15.0f ) 
{
	PingMcpCountdown = 0;
}

FFriendsMessageManager::~FFriendsMessageManager( )
{}

/* IFriendsMessageManager interface
 *****************************************************************************/

void FFriendsMessageManager::StartupManager()
{
	FParse::Value( FCommandLine::Get(), TEXT( "LaunchGameMessage=" ), GameLaunchMessageID );
}

void FFriendsMessageManager::Init( FOnFriendsNotification& NotificationDelegate, bool bInCanJointGame )
{
	// Clear existing data
	Logout();
	FriendsListNotificationDelegate = &NotificationDelegate;
	bCanJoinGame = bInCanJointGame;
	bPollForMessages = false;

	OnlineSubMcp = static_cast< FOnlineSubsystemMcp* >( IOnlineSubsystem::Get( TEXT( "MCP" ) ) );

	if (OnlineSubMcp != NULL &&
		OnlineSubMcp->GetIdentityInterface().IsValid() &&
		OnlineSubMcp->GetMessageInterface().IsValid())
	{
		// Add our delegate for the async call
		OnEnumerateMessagesCompleteDelegate = FOnEnumerateMessagesCompleteDelegate::CreateSP(this, &FFriendsMessageManager::OnEnumerateMessagesComplete);
		OnReadMessageCompleteDelegate = FOnReadMessageCompleteDelegate::CreateSP(this, &FFriendsMessageManager::OnReadMessageComplete);
		OnSendMessageCompleteDelegate = FOnSendMessageCompleteDelegate::CreateSP(this, &FFriendsMessageManager::OnSendMessageComplete);
		OnDeleteMessageCompleteDelegate = FOnDeleteMessageCompleteDelegate::CreateSP(this, &FFriendsMessageManager::OnDeleteMessageComplete);

		OnlineSubMcp->GetMessageInterface()->AddOnEnumerateMessagesCompleteDelegate(0, OnEnumerateMessagesCompleteDelegate);
		OnlineSubMcp->GetMessageInterface()->AddOnReadMessageCompleteDelegate(0, OnReadMessageCompleteDelegate);
		OnlineSubMcp->GetMessageInterface()->AddOnSendMessageCompleteDelegate(0, OnSendMessageCompleteDelegate);
		OnlineSubMcp->GetMessageInterface()->AddOnDeleteMessageCompleteDelegate(0, OnDeleteMessageCompleteDelegate);

		IOnlineIdentityPtr OnlineIdentity = OnlineSubMcp->GetIdentityInterface();

		TSharedPtr<FUniqueNetId> UserId = OnlineIdentity->GetUniquePlayerId(0);
		if ( UserId.IsValid() )
		{
			DisplayName = OnlineIdentity->GetUserAccount( *UserId )->GetDisplayName();
		}

		if ( UpdateMessagesTickerDelegate.IsBound() == false )
		{
			UpdateMessagesTickerDelegate = FTickerDelegate::CreateSP( this, &FFriendsMessageManager::Tick );
		}

		ManagerState = EFriendsMessageManagerState::Idle;

		FTicker::GetCoreTicker().AddTicker( UpdateMessagesTickerDelegate );
	}
}

void FFriendsMessageManager::SetMessagePolling( bool bStart )
{
	bPollForMessages = bStart;
}

bool FFriendsMessageManager::Tick( float Delta )
{
	PingMcpCountdown -= Delta;

	if ( bPollForMessages && ManagerState == EFriendsMessageManagerState::Idle )
	{
		// Countdown and ping if necessary
		if ( MessagesToRead.Num() > 0 )
		{
			SetState( EFriendsMessageManagerState::ReadingMessages );
		}
		else if ( MessagesToDelete.Num() > 0 )
		{
			SetState( EFriendsMessageManagerState::DeletingMessages );
		}
		else if ( GameInvitesToSend.Num() > 0 )
		{
			SetState( EFriendsMessageManagerState::SendingGameInvite );
		}
		else if ( ChatMessagesToSend.Num() > 0 )
		{
			SetState( EFriendsMessageManagerState::SendingMessages );
		}
		else if ( GameJoingRequestsToSend.Num() > 0 )
		{
			SetState( EFriendsMessageManagerState::SendingJoinGameRequest );
		}
		else if ( PingMcpCountdown < 0.f )
		{
			if ( UnhandledNetID.IsValid() )
			{
				ResendMessage();
			}
			SetState( EFriendsMessageManagerState::EnumeratingMessages );
			PingMcpCountdown = PingMcpInterval;
		}
	}
	return true;
}

void FFriendsMessageManager::RequestEnumerateMessages()
{
	OnlineSubMcp->GetMessageInterface()->EnumerateMessages( 0 );
}

void FFriendsMessageManager::InviteFriendToGame( TSharedRef<FUniqueNetId> FriendID )
{
	GameInvitesToSend.AddUnique( FriendID );
}

void FFriendsMessageManager::RequestJoinAGame( TSharedRef<FUniqueNetId> FriendID )
{
	GameJoingRequestsToSend.AddUnique( FriendID );
}

void FFriendsMessageManager::SendGameInviteRequest()
{
	TSharedPtr<FUniqueNetId> UserId = OnlineSubMcp->GetIdentityInterface()->GetUniquePlayerId(0);
	if ( GameInvitesToSend[ 0 ]->IsValid()  )
	{
		FOnlineMessagePayload InvitePayload;
		TArray<TSharedRef<FUniqueNetId> > Recipients;
		Recipients.Add( GameInvitesToSend[0] );
 		InvitePayload.SetAttribute( TEXT("SenderID"), FVariantData( DisplayName ) );
		InvitePayload.SetAttribute( TEXT("GameInvite"), FVariantData( UserId.Get()->ToString() ) );
		OnlineSubMcp->GetMessageInterface()->SendMessage( 0, Recipients, TEXT("GameInvite"), InvitePayload );
	}

	GameInvitesToSend.RemoveAt( 0 );
}

void FFriendsMessageManager::SendGameJoinRequest()
{
	TSharedPtr<FUniqueNetId> UserId = OnlineSubMcp->GetIdentityInterface()->GetUniquePlayerId( 0 );
	if ( GameJoingRequestsToSend[ 0 ]->IsValid() )
	{
		FOnlineMessagePayload TestPayload;
		TArray<TSharedRef<FUniqueNetId> > Recipients;
		Recipients.Add( GameJoingRequestsToSend[0] );

 		TestPayload.SetAttribute(TEXT("GameInvite"), FVariantData(TEXT("Join your game")));
 		TestPayload.SetAttribute(TEXT("SenderID"), FVariantData( DisplayName ) ); 
		OnlineSubMcp->GetMessageInterface()->SendMessage(0, Recipients, TEXT("GameJoin"), TestPayload);
	}

	GameJoingRequestsToSend.RemoveAt( 0 );
}

void FFriendsMessageManager::SendChatMessageRequest()
{
	TSharedPtr<FUniqueNetId> UserId = OnlineSubMcp->GetIdentityInterface()->GetUniquePlayerId(0);

	if ( ChatMessagesToSend.Num() > 0 )
	{
		FString DisplayMessage = DisplayName;
		DisplayMessage += TEXT ( " says:\n" );
		DisplayMessage += ChatMessagesToSend[0].Message.ToString();

		ChatMessages.Add( MakeShareable ( new FFriendsAndChatMessage( DisplayMessage ) ) );

		FOnlineMessagePayload TestPayload;
		TArray<TSharedRef<FUniqueNetId> > Recipients;
		Recipients.Add( ChatMessagesToSend[0].FriendID );
 		TestPayload.SetAttribute(TEXT("STRINGValue"), FVariantData( ChatMessagesToSend[0].Message.ToString() ) ); 
 		TestPayload.SetAttribute(TEXT("SenderID"), FVariantData( DisplayName ) ); 
		OnlineSubMcp->GetMessageInterface()->SendMessage(0, Recipients, TEXT("TestType"), TestPayload);
	}
	ChatMessagesToSend.RemoveAt( 0 );
}

void FFriendsMessageManager::SendMessage( TSharedPtr< FFriendStuct > FriendID, const FText& Message )
{
	ChatMessagesToSend.Add( FOutGoingChatMessage( FriendID->GetOnlineUser()->GetUserId(), Message ) );
}

void FFriendsMessageManager::ClearGameInvites()
{
	bClearInvites = true;
}

void FFriendsMessageManager::SetUnhandledNotification( TSharedRef< FUniqueNetId > NetID )
{
	UnhandledNetID = NetID;
}

EFriendsMessageManagerState::Type FFriendsMessageManager::GetState()
{
	return ManagerState;
}

TArray< TSharedPtr< FFriendsAndChatMessage > > FFriendsMessageManager::GetChatMessages()
{
	return ChatMessages;
}

FOnMessagesUpdated& FFriendsMessageManager::OnChatListUpdated()
{
	return OChatListUpdatedDelegate;
}

void FFriendsMessageManager::SetState( EFriendsMessageManagerState::Type NewState )
{
	ManagerState = NewState;

	switch ( NewState )
	{
	case EFriendsMessageManagerState::EnumeratingMessages:
		{
			RequestEnumerateMessages();
		}
		break;
	case EFriendsMessageManagerState::DeletingMessages:
		{
			if ( MessagesToDelete.Num() > 0 )
			{
				OnlineSubMcp->GetMessageInterface()->DeleteMessage( 0, *MessagesToDelete[0] );
			}
		}
		break;
	case EFriendsMessageManagerState::ReadingMessages:
		{
			if ( MessagesToRead.Num() > 0 )
			{
				OnlineSubMcp->GetMessageInterface()->ReadMessage( 0, *MessagesToRead[0] );
			}
		}
		break;
	case EFriendsMessageManagerState::SendingGameInvite:
		{
			if ( GameInvitesToSend.Num() > 0 )
			{
				SendGameInviteRequest();
			}
		}
		break;
	case EFriendsMessageManagerState::SendingJoinGameRequest:
		{
			if ( GameJoingRequestsToSend.Num() > 0 )
			{
				SendGameJoinRequest();
			}
		}
		break;
	case EFriendsMessageManagerState::SendingMessages:
		{
			if ( ChatMessagesToSend.Num() > 0 )
			{
				SendChatMessageRequest();
			}
		}
		break;
	}
}

void FFriendsMessageManager::Logout()
{
	FOnlineSubsystemMcp* OnlineSubMcp = static_cast< FOnlineSubsystemMcp* >( IOnlineSubsystem::Get( TEXT( "MCP" ) ) );
	if ( OnlineSubMcp != NULL )
	{
		OnlineSubMcp->GetMessageInterface()->ClearOnEnumerateMessagesCompleteDelegate(0, OnEnumerateMessagesCompleteDelegate);
		OnlineSubMcp->GetMessageInterface()->ClearOnReadMessageCompleteDelegate(0, OnReadMessageCompleteDelegate);
		OnlineSubMcp->GetMessageInterface()->ClearOnSendMessageCompleteDelegate(0, OnSendMessageCompleteDelegate);
		OnlineSubMcp->GetMessageInterface()->ClearOnDeleteMessageCompleteDelegate(0, OnDeleteMessageCompleteDelegate);
	}
	FTicker::GetCoreTicker().RemoveTicker( UpdateMessagesTickerDelegate );
}

void FFriendsMessageManager::OnEnumerateMessagesComplete(int32 LocalPlayer, bool bWasSuccessful, const FString& ErrorStr)
{
	if ( bWasSuccessful )
	{
		TArray< TSharedRef<FOnlineMessageHeader> > MessageHeaders;
		if ( OnlineSubMcp->GetMessageInterface()->GetMessageHeaders( LocalPlayer, MessageHeaders ) )
		{
			bool bFoundLatentMessage = false;

			// Log each message header data out
			for ( int32 Index = 0; Index < MessageHeaders.Num(); Index++ )
			{
				const FOnlineMessageHeader& Header = *MessageHeaders[ Index ];

				if( !bClearInvites && ( bCanJoinGame == true || LatentMessage.IsEmpty() ) )
				{
					// Add to list of messages to download
					MessagesToRead.AddUnique( Header.MessageId );
				}

				if ( bClearInvites || LatentMessage != Header.MessageId->ToString() )
				{
					// Add to list of messages to delete
					MessagesToDelete.AddUnique( &Header.MessageId.Get() );
				}
				else
				{
					bFoundLatentMessage = true;
				}
			}

			if ( bClearInvites || bFoundLatentMessage == false )
			{
				bClearInvites = false;
				LatentMessage = TEXT("");
			}
		}	
		else
		{
			UE_LOG(LogOnline, Log, TEXT( "GetMessageHeaders(%d) failed" ), LocalPlayer );
		}
	}

	SetState( EFriendsMessageManagerState::Idle );
}

void FFriendsMessageManager::OnReadMessageComplete(int32 LocalPlayer, bool bWasSuccessful, const FUniqueMessageId& MessageId, const FString& ErrorStr)
{
	if ( bWasSuccessful )
	{
		TSharedPtr<FOnlineMessage> TempMessage = OnlineSubMcp->GetMessageInterface()->GetMessage( 0, MessageId );
		FOnlineMessagePayload Payload = TempMessage->Payload;
		FVariantData TestValue;
		Payload.GetAttribute(TEXT("STRINGValue"), TestValue);
		FVariantData SenderName;
 		Payload.GetAttribute(TEXT("SenderID"), SenderName );

		FString DisplayMessage = SenderName.ToString();

		FVariantData GameInvite;
 		if ( Payload.GetAttribute(TEXT("GameInvite"), GameInvite ) )
		{
			TSharedPtr< FUniqueNetId > NetID = FFriendsAndChatManager::Get()->FindUserID( DisplayMessage);
			if ( NetID.IsValid() )
			{
				DisplayMessage += TEXT ( " invited you to play" );
				TSharedPtr< FFriendsAndChatMessage > NotificationMessage = MakeShareable( new FFriendsAndChatMessage( DisplayMessage, NetID.ToSharedRef() ) );
				NotificationMessage->SetMessageType( EFriendsRequestType::JoinGame );
				NotificationMessage->SetRequesterName( SenderName.ToString() );
				NotificationMessage->SetSelfHandle( true );

				// auto accept - for game launch
				if ( bCanJoinGame && GameLaunchMessageID != TEXT("") && GameLaunchMessageID == MessageId.ToString() )
				{
					NotificationMessage->SetAutoAccept();
				}
				else if ( !bCanJoinGame )
				{
					// Send message ID
					NotificationMessage->SetLaunchGameID( MessageId.ToString() );
				}

				FriendsListNotificationDelegate->Broadcast( NotificationMessage.ToSharedRef() );

				// Chache, and don't delete game invites if the app cannot directly join a game
				if ( bCanJoinGame == false && LatentMessage.IsEmpty() )
				{
					for ( int32 Index = 0; Index < MessagesToDelete.Num(); Index++ )
					{
						if ( *MessagesToDelete[Index] == MessageId )
						{
							LatentMessage = MessageId.ToString();
							MessagesToDelete.RemoveAt( Index );
							break;
						}
					}
				}
			}
		}
		else
		{
			DisplayMessage += TEXT ( " says:\n" );
			DisplayMessage += TestValue.ToString();
			TSharedPtr< FFriendsAndChatMessage > ChatMessage = MakeShareable( new FFriendsAndChatMessage( DisplayMessage ) );
			ChatMessage->SetSelfHandle( false );
			ChatMessages.Add( ChatMessage );
			OChatListUpdatedDelegate.Broadcast();
		}

		MessagesToRead.RemoveAt(0);
	}

	SetState( EFriendsMessageManagerState::Idle );
}

void FFriendsMessageManager::OnSendMessageComplete(int32 LocalPlayer, bool bWasSuccessful, const FString& ErrorStr)
{
	if ( bWasSuccessful )
	{
		OChatListUpdatedDelegate.Broadcast();
	}

	SetState( EFriendsMessageManagerState::Idle );
}

void FFriendsMessageManager::OnDeleteMessageComplete(int32 LocalPlayer, bool bWasSuccessful, const FUniqueMessageId& MessageId, const FString& ErrorStr)
{
	// done with this part of the test if no more messages to delete
	MessagesToDelete.RemoveAt( 0 );
	SetState( EFriendsMessageManagerState::Idle );
}

FReply FFriendsMessageManager::HandleMessageAccepted( TSharedPtr< FFriendsAndChatMessage > InNotificationMessage )
{
	NotficationMessages.Remove( InNotificationMessage );
	return FReply::Handled();
}

FReply FFriendsMessageManager::HandleMessageDeclined( TSharedPtr< FFriendsAndChatMessage > InNotificationMessage )
{
	NotficationMessages.Remove( InNotificationMessage );
	return FReply::Handled();
}

void FFriendsMessageManager::ResendMessage()
{
	if ( UnhandledNetID.IsValid() )
	{
		TSharedPtr< FFriendsAndChatMessage > NotificationMessage = MakeShareable( new FFriendsAndChatMessage( TEXT(""), UnhandledNetID.ToSharedRef() ) );
		NotificationMessage->SetMessageType( EFriendsRequestType::JoinGame );
		NotificationMessage->SetSelfHandle( true );
		NotificationMessage->SetAutoAccept();
		FriendsListNotificationDelegate->Broadcast( NotificationMessage.ToSharedRef() );
	}
	UnhandledNetID.Reset();
}

/* FFriendsMessageManager system singletons
*****************************************************************************/
TSharedPtr< FFriendsMessageManager > FFriendsMessageManager::SingletonInstance = NULL;

TSharedRef< FFriendsMessageManager > FFriendsMessageManager::Get()
{
	if ( !SingletonInstance.IsValid() )
	{
		SingletonInstance = MakeShareable( new FFriendsMessageManager() );
		SingletonInstance->StartupManager();
	}
	return SingletonInstance.ToSharedRef();
}

void FFriendsMessageManager::Shutdown()
{
	SingletonInstance.Reset();
}

#undef LOCTEXT_NAMESPACE
