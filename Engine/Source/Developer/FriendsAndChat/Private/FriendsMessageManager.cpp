// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "OnlineChatInterface.h"

#define LOCTEXT_NAMESPACE "FriendsMessageManager"

class FFriendsMessageManagerImpl
	: public FFriendsMessageManager
{
public:

	virtual void LogIn() override
	{
		Initialize();
	}

	virtual void LogOut() override
	{
		UnInitialize();
	}

	virtual void SendMessage(const FUniqueNetId& RecipientId, const FString& MsgBody) override
	{
		if (OnlineSubMcp != nullptr &&
		OnlineSubMcp->GetIdentityInterface().IsValid())
		{
			IOnlineIdentityPtr OnlineIdentity = OnlineSubMcp->GetIdentityInterface();
			if(OnlineIdentity->GetUniquePlayerId(0).IsValid())
			{
				if (!RoomName.IsEmpty())
				{
					TSharedPtr<FChatRoomInfo> RoomInfo = ChatInterface->GetRoomInfo(*LoggedInUser, FChatRoomId(RoomName));
					if (RoomInfo.IsValid() &&
						RoomInfo->IsJoined())
					{
						ChatInterface->SendRoomChat(*LoggedInUser, FChatRoomId(RoomName), MsgBody);
					}
				}
				else
				{
					// send to all joined rooms
					TArray<FChatRoomId> JoinedRooms;
					OnlineSub->GetChatInterface()->GetJoinedRooms(*LoggedInUser, JoinedRooms);
					for (auto RoomId : JoinedRooms)
					{
						ChatInterface->SendRoomChat(*LoggedInUser, RoomId, MsgBody);
					}
				}
				
			}
		}
	}

	DECLARE_DERIVED_EVENT(FFriendsMessageManagerImpl, FFriendsMessageManager::FOnChatMessageReceivedEvent, FOnChatMessageReceivedEvent)
	virtual FOnChatMessageReceivedEvent& OnChatMessageRecieved() override
	{
		return MessageReceivedDelegate;
	}


	~FFriendsMessageManagerImpl()
	{
	}

private:
	FFriendsMessageManagerImpl( )
		: OnlineSubMcp(nullptr)
	{
	}

	void Initialize()
	{
		// Clear existing data
		LogOut();

		OnlineSubMcp = static_cast< FOnlineSubsystemMcp* >( IOnlineSubsystem::Get( TEXT( "MCP" ) ) );

		if (OnlineSubMcp != nullptr)
		{
			IOnlineChatPtr ChatInterface = OnlineSubMcp->GetChatInterface();
			if( ChatInterface.IsValid())
			{
				OnChatRoomJoinPublicDelegate = FOnChatRoomJoinPublicDelegate::CreateSP( this, &FFriendsMessageManagerImpl::OnChatRoomJoinPublic);
				OnChatRoomExitDelegate = FOnChatRoomExitDelegate::CreateSP( this, &FFriendsMessageManagerImpl::OnChatRoomExit);
				OnChatRoomMemberJoinDelegate = FOnChatRoomMemberJoinDelegate::CreateSP( this, &FFriendsMessageManagerImpl::OnChatRoomMemberJoin);
				OnChatRoomMemberExitDelegate = FOnChatRoomMemberExitDelegate::CreateSP( this, &FFriendsMessageManagerImpl::OnChatRoomMemberExit);
				OnChatRoomMemberUpdateDelegate = FOnChatRoomMemberUpdateDelegate::CreateSP( this, &FFriendsMessageManagerImpl::OnChatRoomMemberUpdate);
				OnChatRoomMessageReceivedDelegate = FOnChatRoomMessageReceivedDelegate::CreateSP( this, &FFriendsMessageManagerImpl::OnChatRoomMessageReceived);
				OnChatPrivateMessageReceivedDelegate = FOnChatPrivateMessageReceivedDelegate::CreateSP( this, &FFriendsMessageManagerImpl::OnChatPrivateMessageReceived);

				ChatInterface->AddOnChatRoomJoinPublicDelegate(OnChatRoomJoinPublicDelegate);
				ChatInterface->AddOnChatRoomExitDelegate(OnChatRoomExitDelegate);
				ChatInterface->AddOnChatRoomMemberJoinDelegate(OnChatRoomMemberJoinDelegate);
				ChatInterface->AddOnChatRoomMemberExitDelegate(OnChatRoomMemberExitDelegate);
				ChatInterface->AddOnChatRoomMemberUpdateDelegate(OnChatRoomMemberUpdateDelegate);
				ChatInterface->AddOnChatRoomMessageReceivedDelegate(OnChatRoomMessageReceivedDelegate);
				ChatInterface->AddOnChatPrivateMessageReceivedDelegate(OnChatPrivateMessageReceivedDelegate);
			}
		}
	}

	void UnInitialize()
	{
		if (OnlineSubMcp != nullptr)
		{
			IOnlineChatPtr ChatInterface = OnlineSubMcp->GetChatInterface();
			if( ChatInterface.IsValid())
			{
				ChatInterface->ClearOnChatRoomJoinPublicDelegate(OnChatRoomJoinPublicDelegate);
				ChatInterface->ClearOnChatRoomExitDelegate(OnChatRoomExitDelegate);
				ChatInterface->ClearOnChatRoomMemberJoinDelegate(OnChatRoomMemberJoinDelegate);
				ChatInterface->ClearOnChatRoomMemberExitDelegate(OnChatRoomMemberExitDelegate);
				ChatInterface->ClearOnChatRoomMemberUpdateDelegate(OnChatRoomMemberUpdateDelegate);
				ChatInterface->ClearOnChatRoomMessageReceivedDelegate(OnChatRoomMessageReceivedDelegate);
				ChatInterface->ClearOnChatPrivateMessageReceivedDelegate(OnChatPrivateMessageReceivedDelegate);
			}
		}
		OnlineSubMcp = nullptr;
	}

	void OnChatRoomJoinPublic(const FUniqueNetId& FriendID, const FChatRoomId& ChatRoomID, bool, const FString& Message)
	{		
	}

	void OnChatRoomExit(const FUniqueNetId& FriendID, const FChatRoomId& ChatRoomID, bool, const FString& Message)
	{
	}

	void OnChatRoomMemberJoin(const FUniqueNetId& FriendID, const FChatRoomId& ChatRoomID, const FUniqueNetId& OtherID)
	{
		if (LoggedInUser.IsValid() &&
			*LoggedInUser != MemberId)
		{
			TSharedPtr< FFriendChatMessage > ChatItem = MakeShareable(new FFriendChatMessage());
			TSharedPtr<FChatRoomMember> ChatMember = OnlineSub->GetChatInterface()->GetMember(UserId, ChatRoomID, MemberId);
			if (ChatMember.IsValid())
			{
				ChatItem->FromName = FText::FromString(*ChatMember->GetNickname());
			}
			ChatItem->Message = FText::FromString(TEXT("entered room"));
			ChatItem->MessageType = EChatMessageType::Global;
			ChatItem->MessageTimeText = FText::AsTime(FDateTime::Now());
			ChatItem->bIsFromSelf = false;
			OnChatMessageRecieved().Broadcast(ChatItem.ToSharedRef());
		}
	}

	void OnChatRoomMemberExit(const FUniqueNetId& FriendID, const FChatRoomId& ChatRoomID, const FUniqueNetId& OtherID)
	{
		if (LoggedInUser.IsValid() &&
			*LoggedInUser != MemberId)
		{
			TSharedPtr< FFriendChatMessage > ChatItem = MakeShareable(new FFriendChatMessage());
			TSharedPtr<FChatRoomMember> ChatMember = OnlineSub->GetChatInterface()->GetMember(UserId, ChatRoomID, MemberId);
			if (ChatMember.IsValid())
			{
				ChatItem->FromName = FText::FromString(*ChatMember->GetNickname());
			}
			ChatItem->Message = FText::FromString(TEXT("left room"));
			ChatItem->MessageType = EChatMessageType::Global;
			ChatItem->MessageTimeText = FText::AsTime(FDateTime::Now());
			ChatItem->bIsFromSelf = false;
			OnChatMessageRecieved().Broadcast(ChatItem.ToSharedRef());
		}
	}

	void OnChatRoomMemberUpdate(const FUniqueNetId& FriendID, const FChatRoomId& ChatRoomID, const FUniqueNetId& OtherID)
	{
	}

	void OnChatRoomMessageReceived(const FUniqueNetId& FriendID, const FChatRoomId& ChatRoomID, const FChatMessage& ChatMessage)
	{
		TSharedPtr< FFriendChatMessage > ChatItem = MakeShareable(new FFriendChatMessage());

		TSharedPtr<FChatRoomMember> ChatMember = OnlineSub->GetChatInterface()->GetMember(UserId, ChatRoomID, ChatMessage.GetUserId());
		if (ChatMember.IsValid())
		{
			ChatItem->FromName = FText::FromString(*ChatMember->GetNickname());
		}
		else
		{
			ChatItem->FromName = FText::FromString("Unknown");
		}
		ChatItem->Message = FText::FromString(*ChatMessage.GetBody());
		ChatItem->MessageType = EChatMessageType::Global;
		ChatItem->MessageTimeText = FText::AsTime(ChatMessage.GetTimestamp());
		ChatItem->bIsFromSelf = UserId == *LoggedInUser;
		OnChatMessageRecieved().Broadcast(ChatItem.ToSharedRef());

		TSharedPtr< FFriendChatMessage > ChatItem = MakeShareable(new FFriendChatMessage());

		ChatItem->FromName = FText::FromString(*ChatMessage.GetNickname());
		ChatItem->Message = FText::FromString(*ChatMessage.GetBody());
		ChatItem->MessageType = EChatMessageType::Global;
		ChatItem->MessageTimeText = FText::AsTime(ChatMessage.GetTimestamp());
		ChatItem->bIsFromSelf = ChatMessage.GetUserId() == *LoggedInUser;
		OnChatMessageRecieved().Broadcast(ChatItem.ToSharedRef());

	}

	void OnChatPrivateMessageReceived(const FUniqueNetId& FriendID, const FChatMessage& ChatMessage)
	{
		TSharedPtr< FFriendChatMessage > ChatItem = MakeShareable(new FFriendChatMessage());

		TSharedPtr<FFriendStuct> FoundFriend = FFriendsAndChatManager::Get()->FindUser(ChatMessage.GetUserId());
		if(FoundFriend.IsValid())
		{
			ChatItem->FriendID = FText::FromString(*FoundFriend->GetName());
		}
		else
		{
			ChatItem->FriendID = FText::FromString("Unknown");
		}
		ChatItem->Message = FText::FromString(*ChatMessage.GetBody());
		ChatItem->MessageType = EChatMessageType::Whisper;
		ChatItem->MessageTimeText = FText::AsTime(ChatMessage.GetTimestamp());
		ChatItem->bIsFromSelf = false;
		OnChatMessageRecieved().Broadcast(ChatItem.ToSharedRef());
	}

private:

	// Incomming delegates
	FOnChatRoomJoinPublicDelegate OnChatRoomJoinPublicDelegate;
	FOnChatRoomExitDelegate OnChatRoomExitDelegate;
	FOnChatRoomMemberJoinDelegate OnChatRoomMemberJoinDelegate;
	FOnChatRoomMemberExitDelegate OnChatRoomMemberExitDelegate;
	FOnChatRoomMemberUpdateDelegate OnChatRoomMemberUpdateDelegate;
	FOnChatRoomMessageReceivedDelegate OnChatRoomMessageReceivedDelegate;
	FOnChatPrivateMessageReceivedDelegate OnChatPrivateMessageReceivedDelegate;

	// Outgoing events
	FOnChatMessageReceivedEvent MessageReceivedDelegate;

	FOnlineSubsystemMcp* OnlineSubMcp;

private:
	friend FFriendsMessageManagerFactory;
};

TSharedRef< FFriendsMessageManager > FFriendsMessageManagerFactory::Create()
{
	TSharedRef< FFriendsMessageManagerImpl > MessageManager(new FFriendsMessageManagerImpl());
	return MessageManager;
}

#undef LOCTEXT_NAMESPACE
