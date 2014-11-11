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

		if (OnlineSub != nullptr &&
			OnlineSub->GetIdentityInterface().IsValid())
		{
			LoggedInUser = OnlineSub->GetIdentityInterface()->GetUniquePlayerId(0);
		}

		for (auto RoomName : PendingRoomJoins)
		{
			JoinPublicRoom(RoomName);
		}
	}

	virtual void LogOut() override
	{
		if (OnlineSub != nullptr &&
			OnlineSub->GetChatInterface().IsValid() &&
			LoggedInUser.IsValid())
		{
			// exit out of any rooms that we're in
			TArray<FChatRoomId> JoinedRooms;
			OnlineSub->GetChatInterface()->GetJoinedRooms(*LoggedInUser, JoinedRooms);
			for (auto RoomId : JoinedRooms)
			{
				OnlineSub->GetChatInterface()->ExitRoom(*LoggedInUser, RoomId);
			}
		}
		LoggedInUser.Reset();
		UnInitialize();
	}

	virtual void SendRoomMessage(const FString& RoomName, const FString& MsgBody) override
	{
		if (OnlineSub != nullptr &&
			LoggedInUser.IsValid())
		{
			IOnlineChatPtr ChatInterface = OnlineSub->GetChatInterface();
			if (ChatInterface.IsValid())
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

	virtual void SendPrivateMessage(const FUniqueNetId& RecipientId, const FString& MsgBody) override
	{
		if (OnlineSub != nullptr &&
			LoggedInUser.IsValid())
		{
			IOnlineChatPtr ChatInterface = OnlineSub->GetChatInterface();
			if(ChatInterface.IsValid())
			{
				ChatInterface->SendPrivateChat(*LoggedInUser, RecipientId, MsgBody);
			}
		}
	}

	virtual void SendNetworkMessage(const FString& MsgBody) override
	{
		FFriendsAndChatManager::Get()->SendNetworkMessage(MsgBody);
	}

	virtual void InsertNetworkMessage(const FString& MsgBody) override
	{
		TSharedPtr< FFriendChatMessage > ChatItem = MakeShareable(new FFriendChatMessage());
		ChatItem->FromName = FText::FromString("Game");
		ChatItem->Message = FText::FromString(MsgBody);
		ChatItem->MessageType = EChatMessageType::Network;
		ChatItem->MessageTimeText = FText::AsTime(FDateTime::Now());
		ChatItem->bIsFromSelf = false;
		OnChatMessageRecieved().Broadcast(ChatItem.ToSharedRef());
	}

	virtual void JoinPublicRoom(const FString& RoomName) override
	{
		if (LoggedInUser.IsValid())
		{
			PendingRoomJoins.Remove(RoomName);

			if (OnlineSub != nullptr &&
				OnlineSub->GetChatInterface().IsValid())
			{
				// join the room to start receiving messages from it
				FString NickName = OnlineSub->GetIdentityInterface()->GetPlayerNickname(*LoggedInUser);
				OnlineSub->GetChatInterface()->JoinPublicRoom(*LoggedInUser, FChatRoomId(RoomName), NickName);
			}
		}
		else
		{
			PendingRoomJoins.AddUnique(RoomName);
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
		: OnlineSub(nullptr)
	{
	}

	void Initialize()
	{
		// Clear existing data
		LogOut();

		OnlineSub = IOnlineSubsystem::Get( TEXT( "MCP" ) );

		if (OnlineSub != nullptr)
		{
			IOnlineChatPtr ChatInterface = OnlineSub->GetChatInterface();
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
		if (OnlineSub != nullptr)
		{
			IOnlineChatPtr ChatInterface = OnlineSub->GetChatInterface();
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
		OnlineSub = nullptr;
	}

	void OnChatRoomJoinPublic(const FUniqueNetId& UserId, const FChatRoomId& ChatRoomID, bool bWasSuccessful, const FString& Error)
	{
	}

	void OnChatRoomExit(const FUniqueNetId& UserId, const FChatRoomId& ChatRoomID, bool bWasSuccessful, const FString& Error)
	{
	}

	void OnChatRoomMemberJoin(const FUniqueNetId& UserId, const FChatRoomId& ChatRoomID, const FUniqueNetId& MemberId)
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

	void OnChatRoomMemberExit(const FUniqueNetId& UserId, const FChatRoomId& ChatRoomID, const FUniqueNetId& MemberId)
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

	void OnChatRoomMemberUpdate(const FUniqueNetId& UserId, const FChatRoomId& ChatRoomID, const FUniqueNetId& MemberId)
	{
	}

	void OnChatRoomMessageReceived(const FUniqueNetId& UserId, const FChatRoomId& ChatRoomID, const FChatMessage& ChatMessage)
	{
		TSharedPtr< FFriendChatMessage > ChatItem = MakeShareable(new FFriendChatMessage());

		ChatItem->FromName = FText::FromString(*ChatMessage.GetNickname());
		ChatItem->Message = FText::FromString(*ChatMessage.GetBody());
		ChatItem->MessageType = EChatMessageType::Global;
		ChatItem->MessageTimeText = FText::AsTime(ChatMessage.GetTimestamp());
		ChatItem->bIsFromSelf = ChatMessage.GetUserId() == *LoggedInUser;
		OnChatMessageRecieved().Broadcast(ChatItem.ToSharedRef());

	}

	void OnChatPrivateMessageReceived(const FUniqueNetId& UserId, const FChatMessage& ChatMessage)
	{
		TSharedPtr< FFriendChatMessage > ChatItem = MakeShareable(new FFriendChatMessage());

		TSharedPtr<IFriendItem> FoundFriend = FFriendsAndChatManager::Get()->FindUser(ChatMessage.GetUserId());
		if(FoundFriend.IsValid())
		{
			ChatItem->FromName = FText::FromString(*FoundFriend->GetName());
		}
		else
		{
			ChatItem->FromName = FText::FromString("Unknown");
		}
		ChatItem->Message = FText::FromString(*ChatMessage.GetBody());
		ChatItem->MessageType = EChatMessageType::Whisper;
		ChatItem->MessageTimeText = FText::AsTime(ChatMessage.GetTimestamp());
		ChatItem->bIsFromSelf = false;
		OnChatMessageRecieved().Broadcast(ChatItem.ToSharedRef());
	}

private:

	// Incoming delegates
	FOnChatRoomJoinPublicDelegate OnChatRoomJoinPublicDelegate;
	FOnChatRoomExitDelegate OnChatRoomExitDelegate;
	FOnChatRoomMemberJoinDelegate OnChatRoomMemberJoinDelegate;
	FOnChatRoomMemberExitDelegate OnChatRoomMemberExitDelegate;
	FOnChatRoomMemberUpdateDelegate OnChatRoomMemberUpdateDelegate;
	FOnChatRoomMessageReceivedDelegate OnChatRoomMessageReceivedDelegate;
	FOnChatPrivateMessageReceivedDelegate OnChatPrivateMessageReceivedDelegate;

	// Outgoing events
	FOnChatMessageReceivedEvent MessageReceivedDelegate;

	IOnlineSubsystem* OnlineSub;
	TSharedPtr<FUniqueNetId> LoggedInUser;
	TArray<FString> PendingRoomJoins;

private:
	friend FFriendsMessageManagerFactory;
};

TSharedRef< FFriendsMessageManager > FFriendsMessageManagerFactory::Create()
{
	TSharedRef< FFriendsMessageManagerImpl > MessageManager(new FFriendsMessageManagerImpl());
	return MessageManager;
}

#undef LOCTEXT_NAMESPACE