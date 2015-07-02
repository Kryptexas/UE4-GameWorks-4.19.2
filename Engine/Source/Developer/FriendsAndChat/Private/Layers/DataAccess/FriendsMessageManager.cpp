// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "OnlineChatInterface.h"
#include "ChatItemViewModel.h"
#include "IChatCommunicationService.h"
#include "FriendsAndChatAnalytics.h"

#define LOCTEXT_NAMESPACE "FriendsMessageManager"
// Message expiry time for different message types
static const int32 GlobalMessageLifetime = 5 * 60;  // 5 min
static const int32 GameMessageLifetime = 5 * 60;  // 5 min
static const int32 PartyMessageLifetime = 5 * 60;  // 5 min
static const int32 WhisperMessageLifetime = 5 * 60;  // 5 min
static const int32 MessageStore = 200;
static const int32 GlobalMaxStore = 100;
static const int32 WhisperMaxStore = 100;
static const int32 GameMaxStore = 100;
static const int32 PartyMaxStore = 100;

class FFriendsMessageManagerImpl
	: public FFriendsMessageManager
{
public:

	virtual void LogIn(IOnlineSubsystem* InOnlineSub, int32 LocalUserNum) override
	{
		// Clear existing data
		LogOut();

		OnlineSub = InOnlineSub;

		Initialize();
		if (OnlineSub != nullptr &&
			OnlineSub->GetIdentityInterface().IsValid())
		{
			LoggedInUser = OnlineSub->GetIdentityInterface()->GetUniquePlayerId(LocalUserNum);
		}

		for (auto RoomName : RoomJoins)
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

	virtual const TArray<TSharedRef<FFriendChatMessage> >& GetMessages() const override
	{
		return ReceivedMessages;
	}

	virtual bool SendRoomMessage(const FString& RoomName, const FString& MsgBody) override
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
						return ChatInterface->SendRoomChat(*LoggedInUser, FChatRoomId(RoomName), MsgBody);
					}
				}
				else
				{
					// send to all joined rooms
					bool bAbleToSend = false;
					TArray<FChatRoomId> JoinedRooms;
					OnlineSub->GetChatInterface()->GetJoinedRooms(*LoggedInUser, JoinedRooms);
					for (auto RoomId : JoinedRooms)
					{
						if (ChatInterface->SendRoomChat(*LoggedInUser, RoomId, MsgBody))
						{
							bAbleToSend = true;
						}
					}
					return bAbleToSend;
				}

			}
		}
		return false;
	}

	virtual bool SendPrivateMessage(TSharedPtr<const FUniqueNetId> UserID, const FText MessageText) override
	{
		if (OnlineSub != nullptr &&
			LoggedInUser.IsValid())
		{
			IOnlineChatPtr ChatInterface = OnlineSub->GetChatInterface();
			if(ChatInterface.IsValid())
			{
				TSharedPtr< FFriendChatMessage > ChatItem = MakeShareable(new FFriendChatMessage());
				ChatItem->FromName = FText::FromString(OnlineSub->GetIdentityInterface()->GetPlayerNickname(*LoggedInUser.Get()));
				TSharedPtr<IFriendItem> FoundFriend = FriendsAndChatManager.Pin()->FindUser(*UserID.Get());
				ChatItem->ToName = FText::FromString(*FoundFriend->GetName());
				ChatItem->Message = MessageText;
				ChatItem->MessageType = EChatMessageType::Whisper;
				ChatItem->MessageTime = FDateTime::Now();
				ChatItem->ExpireTime = FDateTime::Now() + FTimespan::FromSeconds(WhisperMessageLifetime);
				ChatItem->bIsFromSelf = true;
				ChatItem->SenderId = LoggedInUser;
				ChatItem->RecipientId = UserID;
				AddMessage(ChatItem.ToSharedRef());
				
				FriendsAndChatManager.Pin()->GetAnalytics()->RecordPrivateChat(UserID->ToString());

				return ChatInterface->SendPrivateChat(*LoggedInUser, *UserID.Get(), MessageText.ToString());
			}
		}
		return false;
	}

	virtual void InsertNetworkMessage(const FString& MsgBody) override
	{
		TSharedPtr< FFriendChatMessage > ChatItem = MakeShareable(new FFriendChatMessage());
		ChatItem->FromName = FText::FromString("Game");
		ChatItem->Message = FText::FromString(MsgBody);
		ChatItem->MessageType = EChatMessageType::Game;
		ChatItem->MessageTime = FDateTime::Now();
		ChatItem->ExpireTime = FDateTime::Now() + FTimespan::FromSeconds(GameMessageLifetime);
		ChatItem->bIsFromSelf = false;
		GameMessagesCount++;
		AddMessage(ChatItem.ToSharedRef());
	}

	virtual void JoinPublicRoom(const FString& RoomName) override
	{
		if (LoggedInUser.IsValid())
		{
			if (OnlineSub != nullptr &&
				OnlineSub->GetChatInterface().IsValid())
			{
				// join the room to start receiving messages from it
				FString NickName = OnlineSub->GetIdentityInterface()->GetPlayerNickname(*LoggedInUser);
				OnlineSub->GetChatInterface()->JoinPublicRoom(*LoggedInUser, FChatRoomId(RoomName), NickName);
			}
		}
		RoomJoins.AddUnique(RoomName);
	}

	DECLARE_DERIVED_EVENT(FFriendsMessageManagerImpl, IChatCommunicationService::FOnChatMessageReceivedEvent, FOnChatMessageReceivedEvent)
	virtual FOnChatMessageReceivedEvent& OnChatMessageRecieved() override
	{
		return MessageReceivedEvent;
	}
	DECLARE_DERIVED_EVENT(FFriendsMessageManagerImpl, FFriendsMessageManager::FOnChatPublicRoomJoinedEvent, FOnChatPublicRoomJoinedEvent)
	virtual FOnChatPublicRoomJoinedEvent& OnChatPublicRoomJoined() override
	{
		return PublicRoomJoinedEvent;
	}
	DECLARE_DERIVED_EVENT(FFriendsMessageManagerImpl, FFriendsMessageManager::FOnChatPublicRoomExitedEvent, FOnChatPublicRoomExitedEvent)
	virtual FOnChatPublicRoomExitedEvent& OnChatPublicRoomExited() override
	{
		return PublicRoomExitedEvent;
	}

	~FFriendsMessageManagerImpl()
	{
	}

private:

	FFriendsMessageManagerImpl(const TSharedRef<FFriendsAndChatManager>& InFriendsAndChatManager)
		: FriendsAndChatManager(InFriendsAndChatManager)
		, OnlineSub(nullptr)
		, bEnableEnterExitMessages(false)
	{
	}

	void Initialize()
	{
		GlobalMessagesCount = 0;
		WhisperMessagesCount = 0;
		GameMessagesCount = 0;
		PartyMessagesCount = 0;
		ReceivedMessages.Empty();

		if (OnlineSub != nullptr)
		{
			IOnlineChatPtr ChatInterface = OnlineSub->GetChatInterface();
			if (ChatInterface.IsValid())
			{
				OnChatRoomJoinPublicDelegate         = FOnChatRoomJoinPublicDelegate        ::CreateSP(this, &FFriendsMessageManagerImpl::OnChatRoomJoinPublic);
				OnChatRoomExitDelegate               = FOnChatRoomExitDelegate              ::CreateSP(this, &FFriendsMessageManagerImpl::OnChatRoomExit);
				OnChatRoomMemberJoinDelegate         = FOnChatRoomMemberJoinDelegate        ::CreateSP(this, &FFriendsMessageManagerImpl::OnChatRoomMemberJoin);
				OnChatRoomMemberExitDelegate         = FOnChatRoomMemberExitDelegate        ::CreateSP(this, &FFriendsMessageManagerImpl::OnChatRoomMemberExit);
				OnChatRoomMemberUpdateDelegate       = FOnChatRoomMemberUpdateDelegate      ::CreateSP(this, &FFriendsMessageManagerImpl::OnChatRoomMemberUpdate);
				OnChatRoomMessageReceivedDelegate    = FOnChatRoomMessageReceivedDelegate   ::CreateSP(this, &FFriendsMessageManagerImpl::OnChatRoomMessageReceived);
				OnChatPrivateMessageReceivedDelegate = FOnChatPrivateMessageReceivedDelegate::CreateSP(this, &FFriendsMessageManagerImpl::OnChatPrivateMessageReceived);

				OnChatRoomJoinPublicDelegateHandle         = ChatInterface->AddOnChatRoomJoinPublicDelegate_Handle        (OnChatRoomJoinPublicDelegate);
				OnChatRoomExitDelegateHandle               = ChatInterface->AddOnChatRoomExitDelegate_Handle              (OnChatRoomExitDelegate);
				OnChatRoomMemberJoinDelegateHandle         = ChatInterface->AddOnChatRoomMemberJoinDelegate_Handle        (OnChatRoomMemberJoinDelegate);
				OnChatRoomMemberExitDelegateHandle         = ChatInterface->AddOnChatRoomMemberExitDelegate_Handle        (OnChatRoomMemberExitDelegate);
				OnChatRoomMemberUpdateDelegateHandle       = ChatInterface->AddOnChatRoomMemberUpdateDelegate_Handle      (OnChatRoomMemberUpdateDelegate);
				OnChatRoomMessageReceivedDelegateHandle    = ChatInterface->AddOnChatRoomMessageReceivedDelegate_Handle   (OnChatRoomMessageReceivedDelegate);
				OnChatPrivateMessageReceivedDelegateHandle = ChatInterface->AddOnChatPrivateMessageReceivedDelegate_Handle(OnChatPrivateMessageReceivedDelegate);
			}
			IOnlinePresencePtr PresenceInterface = OnlineSub->GetPresenceInterface();
			if (PresenceInterface.IsValid())
			{
				OnPresenceReceivedDelegate = FOnPresenceReceivedDelegate::CreateSP(this, &FFriendsMessageManagerImpl::OnPresenceReceived);
				OnPresenceReceivedDelegateHandle = PresenceInterface->AddOnPresenceReceivedDelegate_Handle(OnPresenceReceivedDelegate);
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
				ChatInterface->ClearOnChatRoomJoinPublicDelegate_Handle        (OnChatRoomJoinPublicDelegateHandle);
				ChatInterface->ClearOnChatRoomExitDelegate_Handle              (OnChatRoomExitDelegateHandle);
				ChatInterface->ClearOnChatRoomMemberJoinDelegate_Handle        (OnChatRoomMemberJoinDelegateHandle);
				ChatInterface->ClearOnChatRoomMemberExitDelegate_Handle        (OnChatRoomMemberExitDelegateHandle);
				ChatInterface->ClearOnChatRoomMemberUpdateDelegate_Handle      (OnChatRoomMemberUpdateDelegateHandle);
				ChatInterface->ClearOnChatRoomMessageReceivedDelegate_Handle   (OnChatRoomMessageReceivedDelegateHandle);
				ChatInterface->ClearOnChatPrivateMessageReceivedDelegate_Handle(OnChatPrivateMessageReceivedDelegateHandle);
			}
			IOnlinePresencePtr PresenceInterface = OnlineSub->GetPresenceInterface();
			if (PresenceInterface.IsValid())
			{
				PresenceInterface->ClearOnPresenceReceivedDelegate_Handle(OnPresenceReceivedDelegateHandle);
			}
		}
		OnlineSub = nullptr;
	}

	void OnChatRoomJoinPublic(const FUniqueNetId& UserId, const FChatRoomId& ChatRoomID, bool bWasSuccessful, const FString& Error)
	{
		if (bWasSuccessful)
		{
			OnChatPublicRoomJoined().Broadcast(ChatRoomID);
		}
	}

	void OnChatRoomExit(const FUniqueNetId& UserId, const FChatRoomId& ChatRoomID, bool bWasSuccessful, const FString& Error)
	{
		if (bWasSuccessful)
		{
			OnChatPublicRoomExited().Broadcast(ChatRoomID);
		}		
	}

	void OnChatRoomMemberJoin(const FUniqueNetId& UserId, const FChatRoomId& ChatRoomID, const FUniqueNetId& MemberId)
	{
		if (bEnableEnterExitMessages &&
			LoggedInUser.IsValid() &&
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
			ChatItem->MessageTime = FDateTime::Now();
			ChatItem->ExpireTime = FDateTime::Now() + GlobalMessageLifetime;
			ChatItem->bIsFromSelf = false;
			GlobalMessagesCount++;
			AddMessage(ChatItem.ToSharedRef());
		}
	}

	void OnChatRoomMemberExit(const FUniqueNetId& UserId, const FChatRoomId& ChatRoomID, const FUniqueNetId& MemberId)
	{
		if (bEnableEnterExitMessages &&
			LoggedInUser.IsValid() &&
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
			ChatItem->MessageTime = FDateTime::Now();
			ChatItem->ExpireTime = FDateTime::Now() + GlobalMessageLifetime;
			ChatItem->bIsFromSelf = false;
			GlobalMessagesCount++;
			AddMessage(ChatItem.ToSharedRef());
		}
	}

	void OnChatRoomMemberUpdate(const FUniqueNetId& UserId, const FChatRoomId& ChatRoomID, const FUniqueNetId& MemberId)
	{
	}

	void OnChatRoomMessageReceived(const FUniqueNetId& UserId, const FChatRoomId& ChatRoomID, const TSharedRef<FChatMessage>& ChatMessage)
	{
		TSharedPtr< FFriendChatMessage > ChatItem = MakeShareable(new FFriendChatMessage());

		{
			// Determine roomtype for the message.  
			FString GlobalChatRoomId;
			TSharedPtr<const FOnlinePartyId> PartyChatRoomId = FriendsAndChatManager.Pin()->GetPartyChatRoomId();
			if (FriendsAndChatManager.Pin()->GetGlobalChatRoomId(GlobalChatRoomId) && ChatRoomID == GlobalChatRoomId)
			{
				ChatItem->MessageType = EChatMessageType::Global;
			}
			else if (PartyChatRoomId.IsValid() && ChatRoomID == (*PartyChatRoomId).ToString())
			{
				ChatItem->MessageType = EChatMessageType::Party;
			}
			else
			{
				UE_LOG(LogOnline, Warning, TEXT("Received message for chatroom that didn't match global or party chatroom criteria %s"), *ChatRoomID);
			}
		}
		ChatItem->FromName = FText::FromString(*ChatMessage->GetNickname());
		ChatItem->Message = FText::FromString(*ChatMessage->GetBody());
		ChatItem->MessageTime = FDateTime::Now();
		ChatItem->ExpireTime = FDateTime::Now() + FTimespan::FromSeconds(GlobalMessageLifetime);
		ChatItem->bIsFromSelf = *ChatMessage->GetUserId() == *LoggedInUser;
		ChatItem->SenderId = ChatMessage->GetUserId();
		ChatItem->RecipientId = nullptr;
		ChatItem->MessageRef = ChatMessage;
		GlobalMessagesCount++;
		AddMessage(ChatItem.ToSharedRef());
	}

	void OnChatPrivateMessageReceived(const FUniqueNetId& UserId, const TSharedRef<FChatMessage>& ChatMessage)
	{
		TSharedPtr< FFriendChatMessage > ChatItem = MakeShareable(new FFriendChatMessage());
		TSharedPtr<IFriendItem> FoundFriend = FriendsAndChatManager.Pin()->FindUser(ChatMessage->GetUserId());
		// Ignore messages from unknown people
		if(FoundFriend.IsValid())
		{
			ChatItem->FromName = FText::FromString(*FoundFriend->GetName());
			ChatItem->SenderId = FoundFriend->GetUniqueID();
			ChatItem->bIsFromSelf = false;
			ChatItem->RecipientId = LoggedInUser;
			ChatItem->Message = FText::FromString(*ChatMessage->GetBody());
			ChatItem->MessageType = EChatMessageType::Whisper;
			ChatItem->MessageTime = FDateTime::Now();
			ChatItem->ExpireTime = FDateTime::Now() + FTimespan::FromSeconds(WhisperMessageLifetime);
			ChatItem->MessageRef = ChatMessage;
			WhisperMessagesCount++;
			AddMessage(ChatItem.ToSharedRef());

			// Inform listers that we have received a chat message
			FriendsAndChatManager.Pin()->SendChatMessageReceivedEvent(ChatItem->MessageType, FoundFriend);
		}
	}

	void OnPresenceReceived(const FUniqueNetId& UserId, const TSharedRef<FOnlineUserPresence>& Presence)
	{
		if (LoggedInUser.IsValid() &&
			*LoggedInUser == UserId)
		{
			if (Presence->bIsOnline)
			{
				for (auto RoomName : RoomJoins)
				{
					// @todo What is this for?  Rooms are rejoined on xmpploginchanged and onlogin, why spam joins on self-presence received too?
					JoinPublicRoom(RoomName);
				}
			}
		}
	}

	void AddMessage(TSharedRef< FFriendChatMessage > NewMessage)
	{
		//TSharedRef<FChatItemViewModel> NewMessage = FChatItemViewModelFactory::Create(ChatItem);
		if(ReceivedMessages.Add(NewMessage) > MessageStore)
		{
			bool bGlobalTimeFound = false;
			bool bGameTimeFound = false;
			bool bPartyTimeFound = false;
			bool bWhisperFound = false;
			FDateTime CurrentTime = FDateTime::Now();
			for(int32 Index = 0; Index < ReceivedMessages.Num(); Index++)
			{
				TSharedRef<FFriendChatMessage> Message = ReceivedMessages[Index];
				if (Message->ExpireTime < CurrentTime)
				{
					RemoveMessage(Message);
					Index--;
				}
				else
				{
					switch (Message->MessageType)
					{
						case EChatMessageType::Global :
						{
							if(GlobalMessagesCount > GlobalMaxStore)
							{
								RemoveMessage(Message);
								Index--;
							}
							else
							{
								bGlobalTimeFound = true;
							}
						}
						break;
						case EChatMessageType::Game :
						{
							if(GameMessagesCount > GameMaxStore)
							{
								RemoveMessage(Message);
								Index--;
							}
							else
							{
								bGameTimeFound = true;
							}
						}
						break;
						case EChatMessageType::Party:
						{
							if (PartyMessagesCount > PartyMaxStore)
							{
								RemoveMessage(Message);
								Index--;
							}
							else
							{
								bPartyTimeFound = true;
							}
						}
						break;
						case EChatMessageType::Whisper :
						{
							if(WhisperMessagesCount > WhisperMaxStore)
							{
								RemoveMessage(Message);
								Index--;
							}
							else
							{
								bWhisperFound = true;
							}
						}
						break;
					}
				}
				if (ReceivedMessages.Num() < MessageStore || (bPartyTimeFound && bGameTimeFound && bGlobalTimeFound && bWhisperFound))
				{
					break;
				}
			}
		}
		OnChatMessageRecieved().Broadcast(NewMessage);
	}

	void RemoveMessage(TSharedRef<FFriendChatMessage> Message)
	{
		switch(Message->MessageType)
		{
			case EChatMessageType::Global : GlobalMessagesCount--; break;
			case EChatMessageType::Game: GameMessagesCount--; break;
			case EChatMessageType::Party: PartyMessagesCount--; break;
			case EChatMessageType::Whisper : WhisperMessagesCount--; break;
		}
		ReceivedMessages.Remove(Message);
	}

private:

	TWeakPtr<FFriendsAndChatManager> FriendsAndChatManager;
	// Incoming delegates
	FOnChatRoomJoinPublicDelegate OnChatRoomJoinPublicDelegate;
	FOnChatRoomExitDelegate OnChatRoomExitDelegate;
	FOnChatRoomMemberJoinDelegate OnChatRoomMemberJoinDelegate;
	FOnChatRoomMemberExitDelegate OnChatRoomMemberExitDelegate;
	FOnChatRoomMemberUpdateDelegate OnChatRoomMemberUpdateDelegate;
	FOnChatRoomMessageReceivedDelegate OnChatRoomMessageReceivedDelegate;
	FOnChatPrivateMessageReceivedDelegate OnChatPrivateMessageReceivedDelegate;
	FOnPresenceReceivedDelegate OnPresenceReceivedDelegate;

	// Handles to the above registered delegates
	FDelegateHandle OnChatRoomJoinPublicDelegateHandle;
	FDelegateHandle OnChatRoomExitDelegateHandle;
	FDelegateHandle OnChatRoomMemberJoinDelegateHandle;
	FDelegateHandle OnChatRoomMemberExitDelegateHandle;
	FDelegateHandle OnChatRoomMemberUpdateDelegateHandle;
	FDelegateHandle OnChatRoomMessageReceivedDelegateHandle;
	FDelegateHandle OnChatPrivateMessageReceivedDelegateHandle;
	FDelegateHandle OnPresenceReceivedDelegateHandle;

	// Outgoing events
	FOnChatMessageReceivedEvent MessageReceivedEvent;
	FOnChatPublicRoomJoinedEvent PublicRoomJoinedEvent;
	FOnChatPublicRoomExitedEvent PublicRoomExitedEvent;

	IOnlineSubsystem* OnlineSub;
	TSharedPtr<const FUniqueNetId> LoggedInUser;
	TArray<FString> RoomJoins;

	TArray<TSharedRef<FFriendChatMessage> > ReceivedMessages;

	int32 GlobalMessagesCount;
	int32 WhisperMessagesCount;
	int32 GameMessagesCount;
	int32 PartyMessagesCount;

	bool bEnableEnterExitMessages;

private:
	friend FFriendsMessageManagerFactory;
};

TSharedRef< FFriendsMessageManager > FFriendsMessageManagerFactory::Create(const TSharedRef<FFriendsAndChatManager>& FriendsAndChatManager)
{
	TSharedRef< FFriendsMessageManagerImpl > MessageManager(new FFriendsMessageManagerImpl(FriendsAndChatManager));
	return MessageManager;
}

#undef LOCTEXT_NAMESPACE