// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "SFriendsContainer.h"
#include "SFriendUserHeader.h"
#include "SFriendsStatusCombo.h"
#include "SChatWindow.h"
#include "SChatChrome.h"
#include "SChatChromeFullSettings.h"
#include "ChatChromeViewModel.h"
#include "ChatChromeTabViewModel.h"
#include "FriendsViewModel.h"
#include "FriendViewModel.h"
#include "FriendsStatusViewModel.h"
#include "FriendsUserViewModel.h"
#include "ChatViewModel.h"
#include "SNotificationList.h"
#include "SWindowTitleBar.h"
#include "FriendRecentPlayerItems.h"
#include "FriendGameInviteItem.h"
#include "FriendPartyInviteItem.h"
#include "SFriendUserHeader.h"
#include "ClanRepository.h"
#include "IFriendList.h"
#include "FriendsNavigationService.h"
#include "FriendsChatMarkupService.h"
#include "ChatDisplayService.h"
#include "ChatSettingsService.h"
#include "FriendsAndChatAnalytics.h"

#define LOCTEXT_NAMESPACE "FriendsAndChatManager"

const float CHAT_ANALYTICS_INTERVAL = 5 * 60.0f;  // 5 min

/* FFriendsAndChatManager structors
 *****************************************************************************/

FFriendsAndChatManager::FFriendsAndChatManager()
 	: OnlineSub(nullptr)
	, bJoinedGlobalChat(false)
	, ManagerState ( EFriendsAndManagerState::OffLine )
	, bIsInited( false )
	, bRequiresListRefresh(false)
	, bRequiresRecentPlayersRefresh(false)
	, LocalControllerIndex(0)
	, FlushChatAnalyticsCountdown(CHAT_ANALYTICS_INTERVAL)
	{}

FFriendsAndChatManager::~FFriendsAndChatManager( )
{
}

void FFriendsAndChatManager::Initialize()
{
	MessageManager = FFriendsMessageManagerFactory::Create(SharedThis(this));
	NavigationService = FFriendsNavigationServiceFactory::Create();
	FriendViewModelFactory = FFriendViewModelFactoryFactory::Create(NavigationService.ToSharedRef(), SharedThis(this));
	FriendsListFactory = FFriendListFactoryFactory::Create(FriendViewModelFactory.ToSharedRef(), SharedThis(this));
	TSharedRef<IChatCommunicationService> CommunicationService = StaticCastSharedRef<IChatCommunicationService>(MessageManager.ToSharedRef());
	MarkupServiceFactory = FFriendsChatMarkupServiceFactoryFactory::Create(CommunicationService, NavigationService.ToSharedRef(), FriendsListFactory.ToSharedRef());
	Analytics = FFriendsAndChatAnalyticsFactory::Create();
}

/* IFriendsAndChatManager interface
 *****************************************************************************/

void FFriendsAndChatManager::Login(IOnlineSubsystem* InOnlineSub, bool bInIsGame, int32 LocalUserID)
{
	// Clear existing data
	Logout();

	bIsInited = false;
	bIsInGame = bInIsGame;
	
	if (InOnlineSub)
	{
		OnlineSub = InOnlineSub;
	}
	else
	{
		OnlineSub = IOnlineSubsystem::Get(TEXT("MCP"));
	}

	if (OnlineSub != nullptr &&
		OnlineSub->GetUserInterface().IsValid() &&
		OnlineSub->GetIdentityInterface().IsValid())
	{
		LocalControllerIndex = 0;
		OnlineIdentity = OnlineSub->GetIdentityInterface();
		
		// ToDo - Inject these. Also, create FriendsListViewModelFactory here: Nick Davies 30th March 2015
		ClanRepository = FClanRepositoryFactory::Create();

		if(OnlineIdentity->GetUniquePlayerId(LocalControllerIndex).IsValid())
		{
			IOnlineUserPtr UserInterface = OnlineSub->GetUserInterface();
			check(UserInterface.IsValid());

			FriendsInterface = OnlineSub->GetFriendsInterface();
			check( FriendsInterface.IsValid() )

			// Create delegates for list refreshes
			OnQueryRecentPlayersCompleteDelegate = FOnQueryRecentPlayersCompleteDelegate           ::CreateRaw(this, &FFriendsAndChatManager::OnQueryRecentPlayersComplete);
			OnFriendsListChangedDelegate         = FOnFriendsChangeDelegate                        ::CreateSP (this, &FFriendsAndChatManager::OnFriendsListChanged);
			OnDeleteFriendCompleteDelegate       = FOnDeleteFriendCompleteDelegate                 ::CreateSP (this, &FFriendsAndChatManager::OnDeleteFriendComplete);
			OnQueryUserIdMappingCompleteDelegate = IOnlineUser::FOnQueryUserMappingComplete        ::CreateSP(this, &FFriendsAndChatManager::OnQueryUserIdMappingComplete);
			OnQueryUserInfoCompleteDelegate      = FOnQueryUserInfoCompleteDelegate                ::CreateSP (this, &FFriendsAndChatManager::OnQueryUserInfoComplete);
			OnPresenceReceivedCompleteDelegate   = FOnPresenceReceivedDelegate                     ::CreateSP (this, &FFriendsAndChatManager::OnPresenceReceived);
			OnPresenceUpdatedCompleteDelegate    = IOnlinePresence::FOnPresenceTaskCompleteDelegate::CreateSP (this, &FFriendsAndChatManager::OnPresenceUpdated);
			OnFriendInviteReceivedDelegate       = FOnInviteReceivedDelegate                       ::CreateSP (this, &FFriendsAndChatManager::OnFriendInviteReceived);
			OnFriendRemovedDelegate              = FOnFriendRemovedDelegate                        ::CreateSP (this, &FFriendsAndChatManager::OnFriendRemoved);
			OnFriendInviteRejected               = FOnInviteRejectedDelegate                       ::CreateSP (this, &FFriendsAndChatManager::OnInviteRejected);
			OnFriendInviteAccepted               = FOnInviteAcceptedDelegate                       ::CreateSP (this, &FFriendsAndChatManager::OnInviteAccepted);
			OnGameInviteReceivedDelegate         = FOnSessionInviteReceivedDelegate                ::CreateSP (this, &FFriendsAndChatManager::OnGameInviteReceived);
			OnPartyInviteReceivedDelegate        = FOnPartyInviteReceivedDelegate                  ::CreateSP(this, &FFriendsAndChatManager::OnPartyInviteReceived);
			OnDestroySessionCompleteDelegate     = FOnDestroySessionCompleteDelegate               ::CreateSP (this, &FFriendsAndChatManager::OnGameDestroyed);
			OnPartyMemberJoinedDelegate          = FOnPartyMemberJoinedDelegate                    ::CreateSP (this, &FFriendsAndChatManager::OnPartyMemberJoined);
			OnPartyMemberExitedDelegate          = FOnPartyMemberExitedDelegate                    ::CreateSP (this, &FFriendsAndChatManager::OnPartyMemberExited);

			OnQueryRecentPlayersCompleteDelegateHandle	= FriendsInterface->AddOnQueryRecentPlayersCompleteDelegate_Handle(   OnQueryRecentPlayersCompleteDelegate);
			OnFriendsListChangedDelegateHandle			= FriendsInterface->AddOnFriendsChangeDelegate_Handle(LocalControllerIndex, OnFriendsListChangedDelegate);
			OnFriendInviteReceivedDelegateHandle		= FriendsInterface->AddOnInviteReceivedDelegate_Handle            (   OnFriendInviteReceivedDelegate);
			OnFriendRemovedDelegateHandle				= FriendsInterface->AddOnFriendRemovedDelegate_Handle             (   OnFriendRemovedDelegate);
			OnFriendInviteRejectedHandle				= FriendsInterface->AddOnInviteRejectedDelegate_Handle            (   OnFriendInviteRejected);
			OnFriendInviteAcceptedHandle				= FriendsInterface->AddOnInviteAcceptedDelegate_Handle            (   OnFriendInviteAccepted);
			OnDeleteFriendCompleteDelegateHandle		= FriendsInterface->AddOnDeleteFriendCompleteDelegate_Handle(LocalControllerIndex, OnDeleteFriendCompleteDelegate);

			OnQueryUserInfoCompleteDelegateHandle = UserInterface->AddOnQueryUserInfoCompleteDelegate_Handle(LocalControllerIndex, OnQueryUserInfoCompleteDelegate);

			OnPresenceReceivedCompleteDelegateHandle = OnlineSub->GetPresenceInterface()->AddOnPresenceReceivedDelegate_Handle      (OnPresenceReceivedCompleteDelegate);
			OnGameInviteReceivedDelegateHandle       = OnlineSub->GetSessionInterface ()->AddOnSessionInviteReceivedDelegate_Handle (OnGameInviteReceivedDelegate);
			OnPartyInviteReceivedDelegateHandle = OnlineSub->GetPartyInterface()->AddOnPartyInviteReceivedDelegate_Handle(OnPartyInviteReceivedDelegate);
			OnDestroySessionCompleteDelegateHandle   = OnlineSub->GetSessionInterface ()->AddOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegate);

			OnPartyMemberJoinedDelegateHandle = OnlineSub->GetPartyInterface()->AddOnPartyMemberJoinedDelegate_Handle(OnPartyMemberJoinedDelegate);
			OnPartyMemberExitedDelegateHandle = OnlineSub->GetPartyInterface()->AddOnPartyMemberExitedDelegate_Handle(OnPartyMemberExitedDelegate);

			ManagerState = EFriendsAndManagerState::Idle;

			FriendsList.Empty();
			PendingFriendsList.Empty();
			OldUserPresenceMap.Empty();

			if ( UpdateFriendsTickerDelegate.IsBound() == false )
			{
				UpdateFriendsTickerDelegate = FTickerDelegate::CreateSP( this, &FFriendsAndChatManager::Tick );
			}

			UpdateFriendsTickerDelegateHandle = FTicker::GetCoreTicker().AddTicker( UpdateFriendsTickerDelegate );

			SetState(EFriendsAndManagerState::RequestFriendsListRefresh);
			RequestRecentPlayersListRefresh();

			MessageManager->LogIn(OnlineSub, LocalUserID);
			MessageManager->OnChatPublicRoomJoined().AddSP(this, &FFriendsAndChatManager::OnChatPublicRoomJoined);
			for (auto RoomName : ChatRoomstoJoin)
			{
				MessageManager->JoinPublicRoom(RoomName);
			}

			SetUserIsOnline(EOnlinePresenceState::Online);
		}
		else
		{
			SetState(EFriendsAndManagerState::OffLine);
		}
	}
}

void FFriendsAndChatManager::Logout()
{
	// flush before removing the analytics provider
	Analytics->FireEvent_FlushChatStats(); 

	if (OnlineSub != nullptr)
	{
		if (OnlineSub->GetFriendsInterface().IsValid())
		{
			OnlineSub->GetFriendsInterface()->ClearOnQueryRecentPlayersCompleteDelegate_Handle(   OnQueryRecentPlayersCompleteDelegateHandle);
			OnlineSub->GetFriendsInterface()->ClearOnFriendsChangeDelegate_Handle             (LocalControllerIndex, OnFriendsListChangedDelegateHandle);
			OnlineSub->GetFriendsInterface()->ClearOnInviteReceivedDelegate_Handle            (   OnFriendInviteReceivedDelegateHandle);
			OnlineSub->GetFriendsInterface()->ClearOnFriendRemovedDelegate_Handle             (   OnFriendRemovedDelegateHandle);
			OnlineSub->GetFriendsInterface()->ClearOnInviteRejectedDelegate_Handle            (   OnFriendInviteRejectedHandle);
			OnlineSub->GetFriendsInterface()->ClearOnInviteAcceptedDelegate_Handle            (   OnFriendInviteAcceptedHandle);
			OnlineSub->GetFriendsInterface()->ClearOnDeleteFriendCompleteDelegate_Handle	  (LocalControllerIndex, OnDeleteFriendCompleteDelegateHandle);
		}
		if (OnlineSub->GetPresenceInterface().IsValid())
		{
			OnlineSub->GetPresenceInterface()->ClearOnPresenceReceivedDelegate_Handle(OnPresenceReceivedCompleteDelegateHandle);
		}
		if (OnlineSub->GetSessionInterface().IsValid())
		{
			OnlineSub->GetSessionInterface()->ClearOnSessionInviteReceivedDelegate_Handle (OnGameInviteReceivedDelegateHandle);
			OnlineSub->GetSessionInterface()->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegateHandle);
		}
		if (OnlineSub->GetPartyInterface().IsValid())
		{
			OnlineSub->GetPartyInterface()->ClearOnPartyInviteReceivedDelegate_Handle(OnPartyInviteReceivedDelegateHandle);
		}
		if (OnlineSub->GetUserInterface().IsValid())
		{
			OnlineSub->GetUserInterface()->ClearOnQueryUserInfoCompleteDelegate_Handle(LocalControllerIndex, OnQueryUserInfoCompleteDelegateHandle);
		}
		if (OnlineSub->GetPartyInterface().IsValid())
		{
			OnlineSub->GetPartyInterface()->ClearOnPartyMemberJoinedDelegate_Handle(OnPartyMemberJoinedDelegateHandle);
			OnlineSub->GetPartyInterface()->ClearOnPartyMemberExitedDelegate_Handle(OnPartyMemberExitedDelegateHandle);
		}
	}

	FriendsList.Empty();
	RecentPlayersList.Empty();
	PendingFriendsList.Empty();
	FriendByNameRequests.Empty();
	FilteredFriendsList.Empty();
	PendingOutgoingDeleteFriendRequests.Empty();
	PendingOutgoingAcceptFriendRequests.Empty();
	PendingIncomingInvitesList.Empty();
	PendingGameInvitesList.Empty();
	NotifiedRequest.Empty();
	ChatRoomstoJoin.Empty();

	MessageManager->LogOut();

	// Prevent shared pointers lingering after OSS has shutdown
	OnlineIdentity = nullptr;
	FriendsInterface = nullptr;
	OnlineSub = nullptr;

	if ( UpdateFriendsTickerDelegate.IsBound() )
	{
		FTicker::GetCoreTicker().RemoveTicker( UpdateFriendsTickerDelegateHandle );
	}

	MessageManager->OnChatPublicRoomJoined().RemoveAll(this);

	SetState(EFriendsAndManagerState::OffLine);
}

bool FFriendsAndChatManager::IsLoggedIn()
{
	return ManagerState != EFriendsAndManagerState::OffLine;
}

void FFriendsAndChatManager::SetOnline()
{
	SetUserIsOnline(EOnlinePresenceState::Online);
}

void FFriendsAndChatManager::SetAway()
{
	SetUserIsOnline(EOnlinePresenceState::Away);
}

void FFriendsAndChatManager::AddApplicationViewModel(const FString ClientID, TSharedPtr<IFriendsApplicationViewModel> InApplicationViewModel)
{
	ApplicationViewModels.Add(ClientID, InApplicationViewModel);
}

void FFriendsAndChatManager::AddRecentPlayerNamespace(const FString& Namespace)
{
	RecentPlayersNamespaces.AddUnique(Namespace);
}

void FFriendsAndChatManager::ClearApplicationViewModels()
{
	ApplicationViewModels.Empty();
}

void FFriendsAndChatManager::SetAnalyticsProvider(const TSharedPtr<IAnalyticsProvider>& AnalyticsProvider)
{
	Analytics->SetProvider(AnalyticsProvider);
}

void FFriendsAndChatManager::InsertNetworkChatMessage(const FString& InMessage)
{
	MessageManager->InsertNetworkMessage(InMessage);
}

void FFriendsAndChatManager::JoinPublicChatRoom(const FString& RoomName)
{
	if (!RoomName.IsEmpty() && bJoinedGlobalChat == false)
	{
		ChatRoomstoJoin.AddUnique(RoomName);
		MessageManager->JoinPublicRoom(RoomName);
	}
}

bool FFriendsAndChatManager::GetGlobalChatRoomId(FString& OutGlobalRoomId) const
{
	if (ChatRoomstoJoin.Num() > 0)
	{
		OutGlobalRoomId = ChatRoomstoJoin[0];
		return true;
	}
	return false;
}

TSharedPtr<const FOnlinePartyId> FFriendsAndChatManager::GetPartyChatRoomId() const
{
	TSharedPtr<const FUniqueNetId> UserId = OnlineIdentity->GetUniquePlayerId(LocalControllerIndex);
	TSharedPtr<const FOnlinePartyId> PartyChatRoomId;
	TArray< TSharedRef<const FOnlinePartyId> > OutPartyIds;
	if (OnlineSub != nullptr &&
		UserId.IsValid())
	{
		IOnlinePartyPtr PartyInterface = OnlineSub->GetPartyInterface();
		if (PartyInterface.IsValid()
			&& PartyInterface->GetJoinedParties(*UserId, OutPartyIds) == true
			&& OutPartyIds.Num() > 0)
		{
			PartyChatRoomId = OutPartyIds[0]; // @todo EN need to identify the primary game party consistently when multiple parties exist
		}
	}
	return PartyChatRoomId;
}

void FFriendsAndChatManager::OnChatPublicRoomJoined(const FString& ChatRoomID)
{
	if (ChatRoomstoJoin.Contains(ChatRoomID))
	{
		bJoinedGlobalChat = true;
	}
}

bool FFriendsAndChatManager::IsInGlobalChat() const
{
	return bJoinedGlobalChat;
}

TSharedPtr< SWidget > FFriendsAndChatManager::GenerateFriendsListWidget( const FFriendsAndChatStyle* InStyle )
{
	if ( !FriendListWidget.IsValid() )
	{
		if (!ClanRepository.IsValid())
		{
			ClanRepository = FClanRepositoryFactory::Create();
		}
		check(ClanRepository.IsValid())

		Style = *InStyle;
		FFriendsAndChatModuleStyle::Initialize(Style);
		SAssignNew(FriendListWidget, SOverlay)
		+SOverlay::Slot()
		[
			 SNew(SFriendsContainer, FFriendsViewModelFactory::Create(SharedThis(this), ClanRepository.ToSharedRef(), FriendsListFactory.ToSharedRef(), NavigationService.ToSharedRef()))
			.FriendStyle(&Style)
			.FontStyle(&Style.FriendsNormalFontStyle)
			.Method(EPopupMethod::UseCurrentWindow)
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

TSharedPtr< SWidget > FFriendsAndChatManager::GenerateStatusWidget( const FFriendsAndChatStyle* InStyle, bool ShowStatusOptions )
{
	if(ShowStatusOptions)
	{
		check(ClanRepository.IsValid());

		TSharedRef<FFriendsViewModel> FriendsViewModel = FFriendsViewModelFactory::Create(SharedThis(this), ClanRepository.ToSharedRef(), FriendsListFactory.ToSharedRef(), NavigationService.ToSharedRef());

		TSharedPtr<SWidget> HeaderWidget = SNew(SBox)
		.WidthOverride(Style.FriendsListStyle.FriendsListWidth)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SFriendsStatusCombo, FriendsViewModel->GetStatusViewModel(), FriendsViewModel->GetUserViewModel())
				.FriendStyle(&Style)
			]
		];
		return HeaderWidget;
	}

	return SNew(SFriendUserHeader, FFriendsUserViewModelFactory::Create(SharedThis(this)))
		.FriendStyle( InStyle )
		.FontStyle(&InStyle->FriendsNormalFontStyle);
}

TSharedPtr< SWidget > FFriendsAndChatManager::GenerateChatWidget(const FFriendsAndChatStyle* InStyle, TAttribute<FText> ActivationHintDelegate, EChatMessageType::Type MessageType, TSharedPtr<IFriendItem> FriendItem, TSharedPtr< SWindow > WindowPtr)
{
	TSharedRef<IChatDisplayService> ChatDisplayService = GenerateChatDisplayService();
	Style = *InStyle;

	TSharedPtr<FChatViewModel> ViewModel = FChatViewModelFactory::Create(MessageManager.ToSharedRef(), NavigationService.ToSharedRef(), MarkupServiceFactory->Create(), ChatDisplayService, SharedThis(this), EChatViewModelType::Windowed);
	ViewModel->SetChannelFlags(MessageType);
	ViewModel->SetOutgoingMessageChannel(MessageType);
	ViewModel->SetCaptureFocus(true);

	if (FriendItem.IsValid())
	{
		TSharedPtr<FSelectedFriend> NewFriend;
		NewFriend = MakeShareable(new FSelectedFriend());
		NewFriend->DisplayName = FText::FromString(FriendItem->GetName());
		NewFriend->UserID = FriendItem->GetUniqueID();
		ViewModel->SetWhisperFriend(NewFriend);
	}

	ViewModel->RefreshMessages();
	ViewModel->SetIsActive(true);

	TSharedPtr<SChatWindow> ChatWidget = SNew(SChatWindow, ViewModel.ToSharedRef())
		.FriendStyle(InStyle)
		.Method(EPopupMethod::UseCurrentWindow)
		.ActivationHintText(ActivationHintDelegate);


	if (WindowPtr.IsValid())
	{
		WindowPtr->GetOnWindowActivatedEvent().AddSP(ChatWidget.Get(), &SChatWindow::HandleWindowActivated);
		WindowPtr->GetOnWindowDeactivatedEvent().AddSP(ChatWidget.Get(), &SChatWindow::HandleWindowDeactivated);
	}

	return ChatWidget;
}

TSharedPtr<SWidget> FFriendsAndChatManager::GenerateFriendUserHeaderWidget(TSharedPtr<IFriendItem> FriendItem)
{
	if (FriendItem.IsValid())
	{
		TSharedPtr<FSelectedFriend> NewFriend;
		NewFriend = MakeShareable(new FSelectedFriend());
		NewFriend->DisplayName = FText::FromString(FriendItem->GetName());
		NewFriend->UserID = FriendItem->GetUniqueID();
		return SNew(SFriendUserHeader, FriendItem.ToSharedRef()).FriendStyle(&Style).FontStyle(&Style.FriendsNormalFontStyle).ShowUserName(true).Visibility(EVisibility::HitTestInvisible);
	}
	return nullptr;
}

TSharedPtr<FFriendsNavigationService> FFriendsAndChatManager::GetNavigationService()
{
	return NavigationService;
}

TSharedRef< IChatDisplayService > FFriendsAndChatManager::GenerateChatDisplayService(bool FadeChatList, bool FadeChatEntry, float ListFadeTime, float EntryFadeTime)
{
	return FChatDisplayServiceFactory::Create(SharedThis(this), FadeChatList, FadeChatEntry, ListFadeTime, EntryFadeTime);
}

TSharedRef< IChatSettingsService > FFriendsAndChatManager::GenerateChatSettingsService()
{
	return FChatSettingsServiceFactory::Create();
}

TSharedPtr< SWidget > FFriendsAndChatManager::GenerateChromeWidget(const struct FFriendsAndChatStyle* InStyle, TSharedRef<IChatDisplayService> ChatDisplayService, TSharedRef<IChatSettingsService> InChatSettingsService)
{
	Style = *InStyle;
	TSharedPtr<FChatChromeViewModel> ChromeViewModel = FChatChromeViewModelFactory::Create(NavigationService.ToSharedRef(), ChatDisplayService, InChatSettingsService);

	TSharedRef<FChatViewModel> CustomChatViewModel = FChatViewModelFactory::Create(MessageManager.ToSharedRef(), NavigationService.ToSharedRef(), MarkupServiceFactory->Create(), ChatDisplayService, SharedThis(this), EChatViewModelType::Base);
	CustomChatViewModel->SetChannelFlags(EChatMessageType::Global | EChatMessageType::Whisper | EChatMessageType::Game);
	CustomChatViewModel->SetOutgoingMessageChannel(EChatMessageType::Global);

	TSharedRef<FChatViewModel> GlobalChatViewModel = FChatViewModelFactory::Create(MessageManager.ToSharedRef(), NavigationService.ToSharedRef(), MarkupServiceFactory->Create(), ChatDisplayService, SharedThis(this), EChatViewModelType::Base);
	GlobalChatViewModel->SetChannelFlags(EChatMessageType::Global);
	GlobalChatViewModel->SetOutgoingMessageChannel(EChatMessageType::Global);

	TSharedRef<FChatViewModel> WhisperChatViewModel = FChatViewModelFactory::Create(MessageManager.ToSharedRef(), NavigationService.ToSharedRef(), MarkupServiceFactory->Create(), ChatDisplayService, SharedThis(this), EChatViewModelType::Base);
	WhisperChatViewModel->SetChannelFlags(EChatMessageType::Whisper);
	WhisperChatViewModel->SetOutgoingMessageChannel(EChatMessageType::Whisper);

	ChromeViewModel->AddTab(FChatChromeTabViewModelFactory::Create(CustomChatViewModel));
	ChromeViewModel->AddTab(FChatChromeTabViewModelFactory::Create(GlobalChatViewModel));
	ChromeViewModel->AddTab(FChatChromeTabViewModelFactory::Create(WhisperChatViewModel));

	TSharedPtr<SChatChrome> ChatChrome = SNew(SChatChrome, ChromeViewModel.ToSharedRef()).FriendStyle(InStyle);
	
	return ChatChrome;
}

void FFriendsAndChatManager::OpenGlobalChat()
{
	if (IsInGlobalChat())
	{
		if (OnlineIdentity.IsValid() &&
			OnlineIdentity->GetUniquePlayerId(LocalControllerIndex).IsValid())
		{
			Analytics->FireEvent_RecordToggleChat(*OnlineIdentity->GetUniquePlayerId(LocalControllerIndex), TEXT("Global"), true);
			NavigationService->ChangeViewChannel(EChatMessageType::Global);
		}
	}
}

// Actions
void FFriendsAndChatManager::SetUserIsOnline(EOnlinePresenceState::Type OnlineState)
{
	if (OnlineSub != nullptr &&
		OnlineIdentity.IsValid())
	{
		TSharedPtr<const FUniqueNetId> UserId = OnlineIdentity->GetUniquePlayerId(0);
		if (UserId.IsValid())
		{
			TSharedPtr<FOnlineUserPresence> CurrentPresence;
			OnlineSub->GetPresenceInterface()->GetCachedPresence(*UserId, CurrentPresence);
			FOnlineUserPresenceStatus NewStatus;
			if (CurrentPresence.IsValid())
			{
				NewStatus = CurrentPresence->Status;
			}
			NewStatus.State = OnlineState;
			OnlineSub->GetPresenceInterface()->SetPresence(*UserId, NewStatus, OnPresenceUpdatedCompleteDelegate);
		}
	}
}

void FFriendsAndChatManager::AcceptFriend( TSharedPtr< IFriendItem > FriendItem )
{
	PendingOutgoingAcceptFriendRequests.Add(FriendItem->GetOnlineUser()->GetUserId()->ToString());
	FriendItem->SetPendingAccept();
	OnFriendsNotification().Broadcast(false);
	if (OnlineIdentity.IsValid() &&
		OnlineIdentity->GetUniquePlayerId(LocalControllerIndex).IsValid())
	{
		Analytics->FireEvent_FriendRequestAccepted(*OnlineIdentity->GetUniquePlayerId(LocalControllerIndex), *FriendItem);
	}
}


void FFriendsAndChatManager::RejectFriend(TSharedPtr< IFriendItem > FriendItem)
{
	NotifiedRequest.Remove( FriendItem->GetOnlineUser()->GetUserId() );
	PendingOutgoingDeleteFriendRequests.Add(FriendItem->GetOnlineUser()->GetUserId().Get().ToString());
	FriendsList.Remove( FriendItem );
	OnFriendsNotification().Broadcast(false);

	if (OnlineIdentity.IsValid() &&
		OnlineIdentity->GetUniquePlayerId(LocalControllerIndex).IsValid())
	{
		Analytics->FireEvent_FriendRemoved(*OnlineIdentity->GetUniquePlayerId(LocalControllerIndex), *FriendItem, EFriendActionType::ToAnalyticString(EFriendActionType::RejectFriendRequest));
	}
}

void FFriendsAndChatManager::DeleteFriend(TSharedPtr< IFriendItem > FriendItem, EFriendActionType::Type Reason)
{
	TSharedRef<const FUniqueNetId> UserID = FriendItem->GetOnlineFriend()->GetUserId();
	NotifiedRequest.Remove( UserID );
	PendingOutgoingDeleteFriendRequests.Add(UserID.Get().ToString());
	FriendsList.Remove( FriendItem );
	FriendItem->SetPendingDelete();
	RefreshList();
	// Clear notifications
	OnFriendsNotification().Broadcast(false);

	if (OnlineIdentity.IsValid() &&
		OnlineIdentity->GetUniquePlayerId(LocalControllerIndex).IsValid())
	{
		Analytics->FireEvent_FriendRemoved(*OnlineIdentity->GetUniquePlayerId(LocalControllerIndex), *FriendItem, EFriendActionType::ToAnalyticString(Reason));
	}
}

void FFriendsAndChatManager::RequestFriend( const FText& FriendName )
{
	if ( !FriendName.IsEmpty() )
	{
		FriendByNameRequests.AddUnique(*FriendName.ToString());
	}
}

// Process action responses

FReply FFriendsAndChatManager::HandleMessageAccepted( TSharedPtr< FFriendsAndChatMessage > ChatMessage, EFriendsResponseType::Type ResponseType )
{
	switch ( ResponseType )
	{
	case EFriendsResponseType::Response_Accept:
		{
			PendingOutgoingAcceptFriendRequests.Add(ChatMessage->GetUniqueID()->ToString());
			TSharedPtr< IFriendItem > User = FindUser(ChatMessage->GetUniqueID().Get());
			if ( User.IsValid() )
			{
				AcceptFriend(User);
			}
		}
		break;
	case EFriendsResponseType::Response_Reject:
		{
			NotifiedRequest.Remove( ChatMessage->GetUniqueID() );
			TSharedPtr< IFriendItem > User = FindUser( ChatMessage->GetUniqueID().Get());
			if ( User.IsValid() )
			{
				RejectFriend(User);
			}
		}
		break;
	case EFriendsResponseType::Response_Ignore:
		{
			NotifiedRequest.Remove(ChatMessage->GetUniqueID());
			TSharedPtr< IFriendItem > User = FindUser(ChatMessage->GetUniqueID().Get());
			if (User.IsValid())
			{
				DeleteFriend(User, EFriendActionType::IgnoreFriendRequest);
			}
		}
	break;
	}

	NotificationMessages.Remove( ChatMessage );

	return FReply::Handled();
}

FReply FFriendsAndChatManager::HandleGameInviteResponse( TSharedPtr< FFriendsAndChatMessage > MessageNotification, EFriendsResponseType::Type ResponseType )
{
	TSharedPtr< IFriendItem > User = FindUser(MessageNotification->GetUniqueID().Get());
	if(User.IsValid())
	{
		if(ResponseType == EFriendsResponseType::Response_Accept)
		{
			AcceptGameInvite(User);
		}
		else
		{
			RejectGameInvite(User);
		}
	}

	NotificationMessages.Remove(MessageNotification);
	return FReply::Handled();
}

// Getters

int32 FFriendsAndChatManager::GetFilteredFriendsList( TArray< TSharedPtr< IFriendItem > >& OutFriendsList )
{
	OutFriendsList = FilteredFriendsList;
	return OutFriendsList.Num();
}

TArray< TSharedPtr< IFriendItem > >& FFriendsAndChatManager::GetRecentPlayerList()
{
	return RecentPlayersList;
}

int32 FFriendsAndChatManager::GetFilteredOutgoingFriendsList( TArray< TSharedPtr< IFriendItem > >& OutFriendsList )
{
	OutFriendsList = FilteredOutgoingList;
	return OutFriendsList.Num();
}

int32 FFriendsAndChatManager::GetFilteredGameInviteList(TArray< TSharedPtr< IFriendItem > >& OutFriendsList)
{
	for (auto It = PendingGameInvitesList.CreateConstIterator(); It; ++It)
	{
		OutFriendsList.Add(It.Value());
	}
	return OutFriendsList.Num();
}

TSharedPtr<const FUniqueNetId> FFriendsAndChatManager::GetGameSessionId() const
{
	if (OnlineSub != nullptr &&
		OnlineIdentity.IsValid() &&
		OnlineSub->GetSessionInterface().IsValid())
	{
		const FNamedOnlineSession* GameSession = OnlineSub->GetSessionInterface()->GetNamedSession(GameSessionName);
		if (GameSession != nullptr)
		{
			TSharedPtr<FOnlineSessionInfo> UserSessionInfo = GameSession->SessionInfo;
			if (UserSessionInfo.IsValid())
			{
				return OnlineIdentity->CreateUniquePlayerId(UserSessionInfo->GetSessionId().ToString());
			}
		}
	}
	return nullptr;
}

TSharedPtr<const FUniqueNetId> FFriendsAndChatManager::GetGameSessionId(FString SessionID) const
{
	return OnlineIdentity->CreateUniquePlayerId(SessionID);
}

bool FFriendsAndChatManager::IsInGameSession() const
{	
	if (OnlineSub != nullptr &&
		OnlineIdentity.IsValid() &&
		OnlineSub->GetSessionInterface().IsValid() &&
		OnlineSub->GetSessionInterface()->GetNamedSession(GameSessionName) != nullptr)
	{
		return true;
	}
	return false;
}

bool FFriendsAndChatManager::IsInActiveParty() const
{
	if (OnlineIdentity.IsValid())
	{
		TSharedPtr<const FUniqueNetId> UserId = OnlineIdentity->GetUniquePlayerId(LocalControllerIndex);
		TSharedPtr<const FOnlinePartyId> ActivePartyId = GetPartyChatRoomId();
		if (ActivePartyId.IsValid() && UserId.IsValid())
		{
			if (OnlineSub != nullptr)
			{
				IOnlinePartyPtr PartyInterface = OnlineSub->GetPartyInterface();
				if (PartyInterface.IsValid())
				{
					TArray< TSharedRef<FOnlinePartyMember> > OutPartyMembers;
					if (PartyInterface->GetPartyMembers(*UserId, *ActivePartyId, OutPartyMembers)
						&& OutPartyMembers.Num() > 1)
					{
						// Fortnite puts you in a party immediately, so we have to check size here to see if YOU think you're in a party & have anyone to talk to
						// @todo What about if people leave and come back - should MUC persist w/ chat history?
						//       Need UI flow to determine right way to do this & edge cases.  Can a party of 1 exist, should it allow party chat, etc.
						return true;
					}
				}
			}
		}
	}
	return false;
}

bool FFriendsAndChatManager::IsFriendInSameSession(const TSharedPtr< const IFriendItem >& FriendItem) const
{
	TSharedPtr<const FUniqueNetId> MySessionId = GetGameSessionId();
	bool bMySessionValid = MySessionId.IsValid() && MySessionId->IsValid();

	TSharedPtr<const FUniqueNetId> FriendSessionId = FriendItem->GetGameSessionId();
	bool bFriendSessionValid = FriendSessionId.IsValid() && FriendSessionId->IsValid();

	return (bMySessionValid && bFriendSessionValid && (*FriendSessionId == *MySessionId));
}

bool FFriendsAndChatManager::IsFriendInSameParty(const TSharedPtr< const IFriendItem >& FriendItem) const
{
	if (OnlineSub != nullptr &&
		OnlineIdentity.IsValid() &&
		OnlineSub->GetPartyInterface().IsValid())
	{
		TSharedPtr<const FUniqueNetId> UserId = OnlineIdentity->GetUniquePlayerId(LocalControllerIndex);
		if (UserId.IsValid())
		{
			TSharedPtr<IOnlinePartyJoinInfo> FriendPartyJoinInfo = GetPartyJoinInfo(FriendItem);
			if (FriendPartyJoinInfo.IsValid())
			{
				TArray<TSharedRef<const FOnlinePartyId>> JoinedParties;
				if (OnlineSub->GetPartyInterface()->GetJoinedParties(*UserId, JoinedParties))
				{
					for (auto PartyId : JoinedParties)
					{
						if (*PartyId == *FriendPartyJoinInfo->GetPartyId())
						{
							return true;
						}
					}
				}
			}
		}
	}

	return false;
}

TSharedPtr<IOnlinePartyJoinInfo> FFriendsAndChatManager::GetPartyJoinInfo(const TSharedPtr<const IFriendItem>& FriendItem) const
{
	TSharedPtr<IOnlinePartyJoinInfo> Result;

	if (OnlineSub != nullptr &&
		OnlineIdentity.IsValid() &&
		OnlineSub->GetPartyInterface().IsValid())
	{
		TSharedPtr<const FUniqueNetId> UserId = OnlineIdentity->GetUniquePlayerId(LocalControllerIndex);
		if (UserId.IsValid())
		{
			Result = OnlineSub->GetPartyInterface()->GetAdvertisedParty(*UserId, *FriendItem->GetUniqueID());
		}
	}

	return Result;
}

bool FFriendsAndChatManager::IsInJoinableParty() const
{
	bool bIsJoinable = false;

	if (OnlineSub != nullptr &&
		OnlineIdentity.IsValid() &&
		OnlineSub->GetPartyInterface().IsValid())
	{
		TSharedPtr<const FUniqueNetId> UserId = OnlineIdentity->GetUniquePlayerId(LocalControllerIndex);
		if (UserId.IsValid())
		{
			if (UserId.IsValid())
			{
				TSharedPtr<IOnlinePartyJoinInfo> PartyJoinInfo = OnlineSub->GetPartyInterface()->GetAdvertisedParty(*UserId, *UserId);
				bIsJoinable = PartyJoinInfo.IsValid() && !PartyJoinInfo->IsInvalidForJoin();
			}
		}
	}

	return bIsJoinable;
}

bool FFriendsAndChatManager::IsInJoinableGameSession() const
{
	bool bIsJoinable = false;

	if (OnlineSub != nullptr)
	{
		IOnlineSessionPtr SessionInt = OnlineSub->GetSessionInterface();
		if (SessionInt.IsValid())
		{
			FNamedOnlineSession* Session = SessionInt->GetNamedSession(GameSessionName);
			if (Session)
			{
				bool bPublicJoinable = false;
				bool bFriendJoinable = false;
				bool bInviteOnly = false;
				bool bAllowInvites = false;
				if (Session->GetJoinability(bPublicJoinable, bFriendJoinable, bInviteOnly, bAllowInvites))
				{
					// User's game is joinable in some way if any of this is true (context needs to be handled outside this function)
					bIsJoinable = bPublicJoinable || bFriendJoinable || bInviteOnly;
				}
			}
		}
	}

	return bIsJoinable;
}

bool FFriendsAndChatManager::JoinGameAllowed(FString ClientID)
{
	bool bJoinGameAllowed = true;

	if (bIsInGame)
	{
		bJoinGameAllowed = true;
	}
	else
	{
		TSharedPtr<IFriendsApplicationViewModel>* FriendsApplicationViewModel = ApplicationViewModels.Find(ClientID);
		if (FriendsApplicationViewModel != nullptr &&
			(*FriendsApplicationViewModel).IsValid())
		{
			bJoinGameAllowed = (*FriendsApplicationViewModel)->IsAppJoinable();
		}
	}	
		
	return bJoinGameAllowed;
}

const bool FFriendsAndChatManager::IsInLauncher() const
{
	// ToDo NDavies - Find a better way to identify if we are in game
	return !AllowFriendsJoinGameDelegate.IsBound();
}

EOnlinePresenceState::Type FFriendsAndChatManager::GetOnlineStatus()
{
	if (OnlineSub != nullptr &&
		OnlineIdentity.IsValid())
	{
		TSharedPtr<FOnlineUserPresence> Presence;
		TSharedPtr<const FUniqueNetId> UserId = OnlineIdentity->GetUniquePlayerId(0);
		if(UserId.IsValid())
		{
			OnlineSub->GetPresenceInterface()->GetCachedPresence(*UserId, Presence);
			if(Presence.IsValid())
			{
				return Presence->Status.State;
			}
		}
	}
	return EOnlinePresenceState::Offline;
}

FString FFriendsAndChatManager::GetUserClientId() const
{
	FString Result;
	if (OnlineSub != nullptr &&
		OnlineIdentity.IsValid())
	{
		TSharedPtr<FOnlineUserPresence> Presence;
		TSharedPtr<const FUniqueNetId> UserId = OnlineIdentity->GetUniquePlayerId(0);
		if (UserId.IsValid())
		{
			OnlineSub->GetPresenceInterface()->GetCachedPresence(*UserId, Presence);
			if (Presence.IsValid())
			{
				const FVariantData* ClientId = Presence->Status.Properties.Find(DefaultClientIdKey);
				if (ClientId != nullptr && ClientId->GetType() == EOnlineKeyValuePairDataType::String)
				{
					ClientId->GetValue(Result);
				}
			}
		}
	}
	return Result;
}

FString FFriendsAndChatManager::GetUserNickname() const
{
	FString Result;
	if (OnlineSub != nullptr &&
		OnlineIdentity.IsValid())
	{
		TSharedPtr<FOnlineUserPresence> Presence;
		TSharedPtr<const FUniqueNetId> UserId = OnlineIdentity->GetUniquePlayerId(0);
		if (UserId.IsValid())
		{
			Result = OnlineIdentity->GetPlayerNickname(*UserId);
		}
	}
	return Result;
}

// List processing

void FFriendsAndChatManager::RequestListRefresh()
{
	bRequiresListRefresh = true;
}

void FFriendsAndChatManager::RequestRecentPlayersListRefresh()
{
	bRequiresRecentPlayersRefresh = true;
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
		else if (bRequiresRecentPlayersRefresh)
		{
			bRequiresRecentPlayersRefresh = false;

			// reset list of namespaces to query
			PendingRecentPlayersQueries = RecentPlayersNamespaces;
			if (PendingRecentPlayersQueries.Num() > 0)
			{
				SetState(EFriendsAndManagerState::RequestRecentPlayersListRefresh);
			}
		}
		else if (ReceivedGameInvites.Num() > 0 || ReceivedPartyInvites.Num() > 0)
		{
			SetState(EFriendsAndManagerState::RequestGameInviteRefresh);
		}

		ProcessCommandLineInvites();
	}

	FlushChatAnalyticsCountdown -= Delta;
	if (FlushChatAnalyticsCountdown <= 0)
	{
		Analytics->FireEvent_FlushChatStats();
		// Reset countdown for new update
		FlushChatAnalyticsCountdown = CHAT_ANALYTICS_INTERVAL;
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
			FOnReadFriendsListComplete Delegate = FOnReadFriendsListComplete::CreateSP(this, &FFriendsAndChatManager::OnReadFriendsListComplete);
			if (FriendsInterface.IsValid() && FriendsInterface->ReadFriendsList(LocalControllerIndex, EFriendsLists::ToString(EFriendsLists::Default), Delegate))
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
	case EFriendsAndManagerState::RequestRecentPlayersListRefresh:
		{
			FString RecentPlayersNamespace = PendingRecentPlayersQueries.Num() > 0 ? PendingRecentPlayersQueries.Pop() : FString();

			bool bRetry = false;
			if (!RecentPlayersNamespace.IsEmpty())
			{
				TSharedPtr<const FUniqueNetId> UserId = OnlineIdentity.IsValid() ? OnlineIdentity->GetUniquePlayerId(LocalControllerIndex) : nullptr;
				if (UserId.IsValid())
				{
					if (FriendsInterface->QueryRecentPlayers(*UserId, RecentPlayersNamespace))
					{
						SetState(EFriendsAndManagerState::RequestingRecentPlayersIDs);
					}
					else
					{
						bRetry = true;
					}
				}
			}

			if (bRetry)
			{
				SetState(EFriendsAndManagerState::Idle);
				RequestRecentPlayersListRefresh();
			}
		}
		break;
	case EFriendsAndManagerState::RequestingFriendsList:
	case EFriendsAndManagerState::RequestingRecentPlayersIDs:
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
			if (FriendsInterface.IsValid())
			{
				TSharedPtr<const FUniqueNetId> PlayerId = OnlineIdentity->CreateUniquePlayerId(PendingOutgoingDeleteFriendRequests[0]);
				FriendsInterface->DeleteFriend(0, *PlayerId, EFriendsLists::ToString(EFriendsLists::Default));
			}
		}
		break;
	case EFriendsAndManagerState::AcceptingFriendRequest:
		{
			if (FriendsInterface.IsValid())
			{
				TSharedPtr<const FUniqueNetId> PlayerId = OnlineIdentity->CreateUniquePlayerId(PendingOutgoingAcceptFriendRequests[0]);
				FOnAcceptInviteComplete Delegate = FOnAcceptInviteComplete::CreateSP(this, &FFriendsAndChatManager::OnAcceptInviteComplete);
				FriendsInterface->AcceptInvite(0, *PlayerId, EFriendsLists::ToString(EFriendsLists::Default), Delegate);
			}
		}
		break;
	case EFriendsAndManagerState::RequestGameInviteRefresh:
		{	
			// process invites and remove entries that are completed
			ProcessReceivedGameInvites();
			if (!RequestGameInviteUserInfo())
			{
				SetState(EFriendsAndManagerState::Idle);
			}			
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

void FFriendsAndChatManager::OnQueryRecentPlayersComplete(const FUniqueNetId& UserId, const FString& Namespace, bool bWasSuccessful, const FString& ErrorStr)
{
	bool bFoundAllIds = true;

	// keep querying until done with all namespaces
	FString RecentPlayersNamespace = PendingRecentPlayersQueries.Num() > 0 ? PendingRecentPlayersQueries.Pop() : FString();
	if (!RecentPlayersNamespace.IsEmpty())
	{
		FriendsInterface->QueryRecentPlayers(UserId, RecentPlayersNamespace);
	}
	else
	{
		RecentPlayersList.Empty();
		TArray< TSharedRef<FOnlineRecentPlayer> > Players;
		if (FriendsInterface->GetRecentPlayers(UserId, FString(), Players))
		{
			for (const auto& RecentPlayer : Players)
			{
				if (RecentPlayer->GetDisplayName().IsEmpty())
				{
					QueryUserIds.Add(RecentPlayer->GetUserId());
				}
				else
				{
					RecentPlayersList.Add(MakeShareable(new FFriendRecentPlayerItem(RecentPlayer)));
				}
			}

			if (QueryUserIds.Num())
			{
				check(OnlineSub != nullptr && OnlineSub->GetUserInterface().IsValid());
				IOnlineUserPtr UserInterface = OnlineSub->GetUserInterface();
				UserInterface->QueryUserInfo(LocalControllerIndex, QueryUserIds);
				bFoundAllIds = false;
			}
			else
			{
				OnFriendsListUpdated().Broadcast();
			}
		}

		if (bFoundAllIds)
		{
			SetState(EFriendsAndManagerState::Idle);
		}
	}
}

void FFriendsAndChatManager::PreProcessList(const FString& ListName)
{
	TArray< TSharedRef<FOnlineFriend> > Friends;
	bool bReadyToChangeState = true;

	if (FriendsInterface.IsValid() && FriendsInterface->GetFriendsList(LocalControllerIndex, ListName, Friends))
	{
		if (Friends.Num() > 0)
		{
			for ( const auto& Friend : Friends)
			{
				TSharedPtr< IFriendItem > ExistingFriend = FindUser(Friend->GetUserId().Get());
				if ( ExistingFriend.IsValid() )
				{
					if (Friend->GetInviteStatus() != ExistingFriend->GetOnlineFriend()->GetInviteStatus() || (ExistingFriend->IsPendingAccepted() && Friend->GetInviteStatus() == EInviteStatus::Accepted))
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

		check(OnlineSub != nullptr && OnlineSub->GetUserInterface().IsValid());
		IOnlineUserPtr UserInterface = OnlineSub->GetUserInterface();

		if ( QueryUserIds.Num() > 0 )
		{
			UserInterface->QueryUserInfo(LocalControllerIndex, QueryUserIds);
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

void FFriendsAndChatManager::OnQueryUserInfoComplete( int32 LocalPlayer, bool bWasSuccessful, const TArray< TSharedRef<const FUniqueNetId> >& UserIds, const FString& ErrorStr )
{
	if( ManagerState == EFriendsAndManagerState::RequestingFriendsList)
	{
		check(OnlineSub != nullptr && OnlineSub->GetUserInterface().IsValid());
		IOnlineUserPtr UserInterface = OnlineSub->GetUserInterface();

		for ( int32 UserIdx=0; UserIdx < UserIds.Num(); UserIdx++ )
		{
			TSharedPtr<FOnlineFriend> OnlineFriend = FriendsInterface->GetFriend( 0, *UserIds[UserIdx], EFriendsLists::ToString( EFriendsLists::Default ) );
			TSharedPtr<FOnlineUser> OnlineUser = OnlineSub->GetUserInterface()->GetUserInfo( LocalPlayer, *UserIds[UserIdx] );
			if (OnlineFriend.IsValid() && OnlineUser.IsValid())
			{
				TSharedPtr<IFriendItem> Existing;
				for (auto FriendEntry : PendingFriendsList)
				{
					if (*FriendEntry->GetUniqueID() == *UserIds[UserIdx])
					{
						Existing = FriendEntry;
						break;
					}
				}
				if (Existing.IsValid())
				{
					Existing->SetOnlineUser(OnlineUser);
					Existing->SetOnlineFriend(OnlineFriend);
				}
				else
				{
					TSharedPtr< FFriendItem > FriendItem = MakeShareable(new FFriendItem(OnlineFriend, OnlineUser, EFriendsDisplayLists::DefaultDisplay, SharedThis(this)));
					PendingFriendsList.Add(FriendItem);
					FriendItem->SetChachedOnline(FriendItem->IsOnline());
				}
			}
			else
			{
				UE_LOG(LogOnline, Log, TEXT("PlayerId=%s not found"), *UserIds[UserIdx]->ToDebugString());
			}
		}

		QueryUserIds.Empty();

		SetState( EFriendsAndManagerState::ProcessFriendsList );
	}
	else if(ManagerState == EFriendsAndManagerState::RequestingRecentPlayersIDs)
	{
		RecentPlayersList.Empty();
		TSharedPtr<const FUniqueNetId> UserId = OnlineIdentity->GetUniquePlayerId(0);
		if (UserId.IsValid())
		{
			TArray< TSharedRef<FOnlineRecentPlayer> > Players;
			if (FriendsInterface->GetRecentPlayers(*UserId, FString(), Players))
			{
				for (const auto& RecentPlayer : Players)
				{
					TSharedRef<FFriendRecentPlayerItem> RecentPlayerItem = MakeShareable(new FFriendRecentPlayerItem(RecentPlayer));
					TSharedPtr<FOnlineUser> OnlineUser = OnlineSub->GetUserInterface()->GetUserInfo(LocalPlayer, *RecentPlayer->GetUserId());
					// Invalid OnlineUser can happen if user disabled their account, but are still on someone's recent players list.  Skip including those users.
					if (OnlineUser.IsValid())
					{
						RecentPlayerItem->SetOnlineUser(OnlineUser);
						RecentPlayersList.Add(RecentPlayerItem);
					}
					else
					{
						FString InvalidUserId = RecentPlayerItem->GetUniqueID()->ToString();
						UE_LOG(LogOnline, VeryVerbose, TEXT("Hiding recent player that we could not obtain info for (eg. disabled player), id: %s"), *InvalidUserId);
					}
				}
			}
		}
		OnFriendsListUpdated().Broadcast();
		SetState(EFriendsAndManagerState::Idle);
	}
	else if (ManagerState == EFriendsAndManagerState::RequestGameInviteRefresh)
	{
		ProcessReceivedGameInvites();
		ReceivedGameInvites.Empty();
		ReceivedPartyInvites.Empty();
		SetState(EFriendsAndManagerState::Idle);
	}
}

bool FFriendsAndChatManager::ProcessFriendsList()
{
	/** Functor for comparing friends list */
	struct FCompareGroupByName
	{
		FORCEINLINE bool operator()( const TSharedPtr< IFriendItem > A, const TSharedPtr< IFriendItem > B ) const
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
			if (PendingFriendsList[Index]->IsUpdated() || FriendsList[Index]->GetUniqueID() != PendingFriendsList[Index]->GetUniqueID())
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

	OnFriendsListUpdated().Broadcast();
}

void FFriendsAndChatManager::SendFriendRequests()
{
	// Invite Friends
	IOnlineUserPtr UserInterface = OnlineSub->GetUserInterface();
	if (UserInterface.IsValid())
	{
		TSharedPtr<const FUniqueNetId> UserId = OnlineIdentity->GetUniquePlayerId(0);
		if (UserId.IsValid())
		{
				for (int32 Index = 0; Index < FriendByNameRequests.Num(); Index++)
				{
					UserInterface->QueryUserIdMapping(*UserId, FriendByNameRequests[Index], OnQueryUserIdMappingCompleteDelegate);
				}
		}
	}
}

TSharedPtr<const FUniqueNetId > FFriendsAndChatManager::FindUserID( const FString& InUsername )
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

TSharedPtr< IFriendItem > FFriendsAndChatManager::FindUser(const FUniqueNetId& InUserID)
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

TSharedPtr< IFriendItem > FFriendsAndChatManager::FindUser(const TSharedRef<const FUniqueNetId>& InUserID)
{
	return FindUser(InUserID.Get());
}

TSharedPtr< IFriendItem > FFriendsAndChatManager::FindRecentPlayer(const FUniqueNetId& InUserID)
{
	for (const auto& Friend : RecentPlayersList)
	{
		if (Friend->GetUniqueID().Get() == InUserID)
		{
			return Friend;
		}
	}

	return nullptr;
}

TSharedPtr<FFriendViewModel> FFriendsAndChatManager::GetFriendViewModel(const FString InUserID, const FText Username)
{
	TSharedPtr<const FUniqueNetId> NetID = OnlineIdentity->CreateUniquePlayerId(InUserID);
	return GetFriendViewModel(NetID, Username);
}

TSharedPtr<FFriendViewModel> FFriendsAndChatManager::GetFriendViewModel(const TSharedPtr<const FUniqueNetId> InUserID, const FText Username)
{
	TSharedPtr<IFriendItem> FoundFriend = FindUser(InUserID.ToSharedRef());
	if (!FoundFriend.IsValid())
	{
		FoundFriend = FindRecentPlayer(*InUserID.Get());
	}
	if (!FoundFriend.IsValid())
	{
		FoundFriend = MakeShareable(new FFriendRecentPlayerItem(InUserID, Username));
	}
	if(FoundFriend.IsValid())
	{
		return FriendViewModelFactory->Create(FoundFriend.ToSharedRef());
	}
	return nullptr;
}

void FFriendsAndChatManager::SendFriendInviteNotification()
{
	for( const auto& FriendRequest : PendingIncomingInvitesList)
	{
		if(OnFriendsActionNotification().IsBound())
		{
			FFormatNamedArguments Args;
			Args.Add(TEXT("Username"), FText::FromString(FriendRequest->GetName()));
			const FText FriendRequestMessage = FText::Format(LOCTEXT("FFriendsAndChatManager_AddedYou", "Friend request from {Username}"), Args);

			TSharedPtr< FFriendsAndChatMessage > NotificationMessage = MakeShareable(new FFriendsAndChatMessage(FriendRequestMessage.ToString(), FriendRequest->GetUniqueID()));
			NotificationMessage->SetButtonCallback( FOnClicked::CreateSP(this, &FFriendsAndChatManager::HandleMessageAccepted, NotificationMessage, EFriendsResponseType::Response_Accept));
			NotificationMessage->SetButtonCallback( FOnClicked::CreateSP(this, &FFriendsAndChatManager::HandleMessageAccepted, NotificationMessage, EFriendsResponseType::Response_Ignore));
			NotificationMessage->SetButtonDescription(LOCTEXT("FFriendsAndChatManager_Accept", "Accept"));
			NotificationMessage->SetButtonDescription(LOCTEXT("FFriendsAndChatManager_Ignore", "Ignore"));
			NotificationMessage->SetButtonStyle(TEXT("FriendsListEmphasisButton"));
			NotificationMessage->SetButtonStyle(TEXT("FriendsListCriticalButton"));
			NotificationMessage->SetMessageType(EFriendsRequestType::FriendInvite);
			OnFriendsActionNotification().Broadcast(NotificationMessage.ToSharedRef());
		}
	}

	PendingIncomingInvitesList.Empty();
	FriendsListNotificationDelegate.Broadcast(true);
}

void FFriendsAndChatManager::SendInviteAcceptedNotification(const TSharedPtr< IFriendItem > Friend)
{
	if(OnFriendsActionNotification().IsBound())
	{
		const FText FriendRequestMessage = GetInviteNotificationText(Friend);
		TSharedPtr< FFriendsAndChatMessage > NotificationMessage = MakeShareable(new FFriendsAndChatMessage(FriendRequestMessage.ToString()));
		NotificationMessage->SetMessageType(EFriendsRequestType::FriendAccepted);
		OnFriendsActionNotification().Broadcast(NotificationMessage.ToSharedRef());
	}
}

void FFriendsAndChatManager::SendMessageReceivedNotification(const TSharedPtr< IFriendItem > Friend)
{
	if (OnFriendsActionNotification().IsBound())
	{
		const FText FriendReceivedMessage = FText::Format(LOCTEXT("FriendMessageRecievedToast", "Message from {0}"), FText::FromString(Friend->GetName()));
		TSharedPtr< FFriendsAndChatMessage > NotificationMessage = MakeShareable(new FFriendsAndChatMessage(FriendReceivedMessage.ToString()));
		NotificationMessage->SetMessageType(EFriendsRequestType::ChatMessage);
		OnFriendsActionNotification().Broadcast(NotificationMessage.ToSharedRef());
	}
}

const FText FFriendsAndChatManager::GetInviteNotificationText(TSharedPtr< IFriendItem > Friend) const
{
	FFormatNamedArguments Args;
	Args.Add(TEXT("Username"), FText::FromString(Friend->GetName()));

	if(Friend->IsPendingAccepted())
	{
		return FText::Format(LOCTEXT("FriendAddedToast", "{Username} added as a friend"), Args);
	}
	return FText::Format(LOCTEXT("FriendAcceptedToast", "{Username} accepted your request"), Args);
}

void FFriendsAndChatManager::OnQueryUserIdMappingComplete(bool bWasSuccessful, const FUniqueNetId& RequestingUserId, const FString& DisplayName, const FUniqueNetId& IdentifiedUserId, const FString& Error)
{
	check( OnlineSub != nullptr && OnlineSub->GetUserInterface().IsValid() );

	EFindFriendResult::Type FindFriendResult = EFindFriendResult::NotFound;

	if ( bWasSuccessful && IdentifiedUserId.IsValid() )
	{
		TSharedPtr<IFriendItem> ExistingFriend = FindUser(IdentifiedUserId);
		if (ExistingFriend.IsValid())
		{
			if (ExistingFriend->GetInviteStatus() == EInviteStatus::Accepted)
			{
				AddFriendsToast(FText::FromString("Already friends"));

				FindFriendResult = EFindFriendResult::AlreadyFriends;
			}
			else
			{
				AddFriendsToast(FText::FromString("Friend already requested"));

				FindFriendResult = EFindFriendResult::FriendsPending;
			}
		}
		else if (IdentifiedUserId == RequestingUserId)
		{
			AddFriendsToast(FText::FromString("Can't friend yourself"));

			FindFriendResult = EFindFriendResult::AddingSelfFail;
		}
		else
		{
			TSharedPtr<const FUniqueNetId> FriendId = OnlineIdentity->CreateUniquePlayerId(IdentifiedUserId.ToString());
			PendingOutgoingFriendRequests.Add(FriendId.ToSharedRef());
			FriendByNameInvites.AddUnique(DisplayName);

			FindFriendResult = EFindFriendResult::Success;
		}
	}
	else
	{
		const FString DiplayMessage = DisplayName +  TEXT(" not found");
		AddFriendsToast(FText::FromString(DiplayMessage));
	}

	bool bRecentPlayer = false;
	if (FindFriendResult == EFindFriendResult::Success)
	{
		bRecentPlayer = FindRecentPlayer(IdentifiedUserId).IsValid();
	}

	if (OnlineIdentity.IsValid() &&
		OnlineIdentity->GetUniquePlayerId(LocalControllerIndex).IsValid())
	{
		Analytics->FireEvent_AddFriend(*OnlineIdentity->GetUniquePlayerId(LocalControllerIndex), 
								  DisplayName, 
								  IdentifiedUserId, 
								  FindFriendResult, 
								  bRecentPlayer);
	}

	FriendByNameRequests.Remove( DisplayName );
	if ( FriendByNameRequests.Num() == 0 )
	{
		if ( PendingOutgoingFriendRequests.Num() > 0 )
		{
			for ( int32 Index = 0; Index < PendingOutgoingFriendRequests.Num(); Index++ )
			{
				FOnSendInviteComplete Delegate = FOnSendInviteComplete::CreateSP(this, &FFriendsAndChatManager::OnSendInviteComplete);
				FriendsInterface->SendInvite(LocalControllerIndex, PendingOutgoingFriendRequests[Index].Get(), EFriendsLists::ToString(EFriendsLists::Default), Delegate);
				AddFriendsToast(LOCTEXT("FFriendsAndChatManager_FriendRequestSent", "Request Sent"));
			}
		}
		else
		{
			SetState(EFriendsAndManagerState::Idle);
		}
	}
}


void FFriendsAndChatManager::OnSendInviteComplete( int32 LocalPlayer, bool bWasSuccessful, const FUniqueNetId& FriendId, const FString& ListName, const FString& ErrorStr )
{
	PendingOutgoingFriendRequests.RemoveAt( 0 );

	if ( PendingOutgoingFriendRequests.Num() == 0 )
	{
		RequestListRefresh();
		SetState(EFriendsAndManagerState::Idle);
	}
}

void FFriendsAndChatManager::OnPresenceReceived(const FUniqueNetId& UserId, const TSharedRef<FOnlineUserPresence>& NewPresence)
{
	// Compare to previous presence for this friend, display a toast if a friend has came online or joined a game
	TSharedPtr<const FUniqueNetId> SelfId = OnlineIdentity->GetUniquePlayerId(0);
	bool bIsSelf = (SelfId.IsValid() && UserId == *SelfId);
	bool bFriendNotificationsBound = OnFriendsActionNotification().IsBound();
	// Don't show notifications if we're building the friends list presences for the first time.
	// Guess at this using the size of the saved presence list. OK to show the last 1 or 2, but avoid spamming dozens of notifications at startup
	int32 OnlineFriendCount = 0;
	for (auto Friend : FriendsList)
	{
		if (Friend->IsOnline())
		{
			OnlineFriendCount++;
		}
	}

	// Check for out of date invites
	if (OnlineSub != nullptr && OnlineIdentity.IsValid())
	{
		TSharedPtr<const FUniqueNetId> PlayerUserId = OnlineIdentity->GetUniquePlayerId(0);
		if (PlayerUserId.IsValid())
		{
			TSharedPtr<FOnlineUserPresence> CurrentPresence;
			TArray<FString> InvitesToRemove;
			OnlineSub->GetPresenceInterface()->GetCachedPresence(*PlayerUserId, CurrentPresence);
			for (auto It = PendingGameInvitesList.CreateConstIterator(); It; ++It)
			{
				FString CurrentSessionID = CurrentPresence->SessionId.IsValid() ? CurrentPresence->SessionId->ToString() : TEXT("");
				FString InviteSessionID = It.Value()->GetGameSessionId().IsValid() ? It.Value()->GetGameSessionId()->ToString() : TEXT("");
				if ((CurrentSessionID == InviteSessionID) || (GetOnlineStatus() != EOnlinePresenceState::Offline && !It.Value()->IsOnline()))
				{
					if (!CurrentSessionID.IsEmpty())
					{
						// Already in the same session so remove
						InvitesToRemove.Add(It.Key());
					}
				}
			}
			for (auto It = InvitesToRemove.CreateConstIterator(); It; ++It)
			{
				TSharedPtr<IFriendItem>* Existing = PendingGameInvitesList.Find(*It);
				if (Existing != nullptr)
				{
					(*Existing)->SetPendingDelete();
					PendingGameInvitesList.Remove(*It);
				}
				OnGameInvitesUpdated().Broadcast();
			}
		}
	}

	TSharedPtr<IFriendItem> PresenceFriend = FindUser(UserId);
	if(PresenceFriend.IsValid())
	{
		if(PresenceFriend->IsCachedOnline() != PresenceFriend->IsOnline())
		{
			RefreshList();
		}
		PresenceFriend->SetChachedOnline(PresenceFriend->IsOnline());
	}

// @todo Antony.Carter Disabled for GDC as its a bit spammy
#if 0
	// When a new friend comes online, we should see eg. OnlineFriedCount = 3, OldUserPresence = 2.  Only assume we're starting up if there's a difference of 2 or more
	bool bJustStartedUp = (OnlineFriendCount - 1 > OldUserPresenceMap.Num());

	// Skip notifications for various reasons
	if (!bIsSelf)
	{
		UE_LOG(LogOnline, Verbose, TEXT("Checking friend presence change for notification %s"), *UserId.ToString());
		if (!bJustStartedUp && bFoundFriend && bFriendNotificationsBound)
		{
			const FString& FriendName = PresenceFriend->GetName();
			FFormatNamedArguments Args;
			Args.Add(TEXT("FriendName"), FText::FromString(FriendName));

			const FOnlineUserPresence* OldPresencePtr = OldUserPresenceMap.Find(UserId.ToString());
			if (OldPresencePtr == nullptr)
			{
				if ( NewPresence->bIsOnline == true)
				{
					// Had no previous presence, if the new one is online then they just logged on
					const FText PresenceChangeText = FText::Format(LOCTEXT("FriendPresenceChange_Online", "{FriendName} Is Now Online"), Args);
					TSharedPtr< FFriendsAndChatMessage > NotificationMessage = MakeShareable(new FFriendsAndChatMessage(PresenceChangeText.ToString()));
					NotificationMessage->SetMessageType(EFriendsRequestType::PresenceChange);
					OnFriendsActionNotification().Broadcast(NotificationMessage.ToSharedRef());
					UE_LOG(LogOnline, Verbose, TEXT("Notifying friend came online %s %s"), *FriendName, *UserId.ToString());
				}
				else
				{
					// This probably shouldn't be possible but can demote from warning if this turns out to be a valid use case
					UE_LOG(LogOnline, Warning, TEXT("Had no cached presence for user, then received an Offline presence. ??? %s %s"), *FriendName, *UserId.ToString());
				}
			}
			else
			{
				// Have a previous presence, see what changed
				const FOnlineUserPresence& OldPresence = *OldPresencePtr;

				if (NewPresence->bIsPlayingThisGame == true
					&& OldPresence.SessionId != NewPresence->SessionId
					&& NewPresence->SessionId.IsValid()
					&& NewPresence->SessionId->IsValid())
				{
					const FText PresenceChangeText = FText::Format(LOCTEXT("FriendPresenceChange_InGame", "{FriendName} Is Now In A Game"), Args);
					TSharedPtr< FFriendsAndChatMessage > NotificationMessage = MakeShareable(new FFriendsAndChatMessage(PresenceChangeText.ToString()));
					NotificationMessage->SetMessageType(EFriendsRequestType::PresenceChange);
					OnFriendsActionNotification().Broadcast(NotificationMessage.ToSharedRef());
					UE_LOG(LogOnline, Verbose, TEXT("Notifying friend playing this game AND sessionId changed %s %s"), *FriendName, *UserId.ToString());
				}
				else if (OldPresence.bIsPlayingThisGame == false && NewPresence->bIsPlayingThisGame == true)
				{
					// could limit notifications to same game only by removing isPlaying check above
					Args.Add(TEXT("GameName"), FText::FromString(PresenceFriend->GetClientName()));
					const FText PresenceChangeText = FText::Format(LOCTEXT("FriendPresenceChange_NowPlayingThisGame", "{FriendName} Is Now Playing {GameName}"), Args);
					TSharedPtr< FFriendsAndChatMessage > NotificationMessage = MakeShareable(new FFriendsAndChatMessage(PresenceChangeText.ToString()));
					NotificationMessage->SetMessageType(EFriendsRequestType::PresenceChange);
					OnFriendsActionNotification().Broadcast(NotificationMessage.ToSharedRef());
					UE_LOG(LogOnline, Verbose, TEXT("Notifying friend isPlayingThisGame %s %s"), *FriendName, *UserId.ToString());
				}
				else if (OldPresence.bIsPlaying == false && NewPresence->bIsPlaying == true)
				{
					Args.Add(TEXT("GameName"), FText::FromString(PresenceFriend->GetClientName()));
					const FText PresenceChangeText = FText::Format(LOCTEXT("FriendPresenceChange_Nowplaying", "{FriendName} Is Now Playing {GameName}"), Args);
					TSharedPtr< FFriendsAndChatMessage > NotificationMessage = MakeShareable(new FFriendsAndChatMessage(PresenceChangeText.ToString()));
					NotificationMessage->SetMessageType(EFriendsRequestType::PresenceChange);
					OnFriendsActionNotification().Broadcast(NotificationMessage.ToSharedRef());
					UE_LOG(LogOnline, Verbose, TEXT("Notifying friend isPlaying %s %s"), *FriendName, *UserId.ToString());
				}
			}
		}

		// Make a copy of new presence to backup
		if (NewPresence->bIsOnline)
		{
			OldUserPresenceMap.Add(UserId.ToString(), NewPresence.Get());
			UE_LOG(LogOnline, Verbose, TEXT("Added friend presence to oldpresence map %s"), *UserId.ToString());
		}
		else
		{
			// Or remove the presence if they went offline
			OldUserPresenceMap.Remove(UserId.ToString());
			UE_LOG(LogOnline, Verbose, TEXT("Removed offline friend presence from oldpresence map %s"), *UserId.ToString());
		}
	}
#endif
}

void FFriendsAndChatManager::OnPresenceUpdated(const FUniqueNetId& UserId, const bool bWasSuccessful)
{
}

void FFriendsAndChatManager::OnFriendsListChanged()
{
	if(ManagerState == EFriendsAndManagerState::Idle)
	{
		// Request called from outside our expected actions. e.g. Friend canceled their friend request
		RequestListRefresh();
	}
}

void FFriendsAndChatManager::OnFriendInviteReceived(const FUniqueNetId& UserId, const FUniqueNetId& FriendId)
{
	RequestListRefresh();
}

void FFriendsAndChatManager::OnGameInviteReceived(const FUniqueNetId& UserId, const FUniqueNetId& FromId, const FString& ClientId, const FOnlineSessionSearchResult& InviteResult)
{
	if (OnlineSub != NULL &&
		OnlineSub->GetIdentityInterface().IsValid())
	{
		TSharedPtr<const FUniqueNetId> FromIdPtr = OnlineSub->GetIdentityInterface()->CreateUniquePlayerId(FromId.ToString());

		// Check we have entitlement for this game
		TSharedPtr<IFriendsApplicationViewModel>* FriendsApplicationViewModel = ApplicationViewModels.Find(ClientId);
		if (FriendsApplicationViewModel != nullptr &&
			(*FriendsApplicationViewModel).IsValid() &&
			(*FriendsApplicationViewModel)->IsAppEntitlementGranted())
		{
			if (FromIdPtr.IsValid())
			{
				ReceivedGameInvites.AddUnique(FReceivedGameInvite(FromIdPtr.ToSharedRef(), InviteResult, ClientId));
			}
		}
	}
}

void FFriendsAndChatManager::OnPartyInviteReceived(const FUniqueNetId& RecipientId, const FOnlinePartyId& PartyId, const FUniqueNetId& SenderId)
{
	if (OnlineSub != NULL &&
		OnlineSub->GetPartyInterface().IsValid())
	{
		TArray<TSharedRef<IOnlinePartyJoinInfo>> PartyInvites;
		if (OnlineSub->GetPartyInterface()->GetPendingInvites(RecipientId, PartyInvites))
		{
			for (auto PartyInvite : PartyInvites)
			{
				if (*PartyInvite->GetPartyId() == PartyId)
				{
					TSharedPtr<const FUniqueNetId> SenderIdPtr = OnlineSub->GetIdentityInterface()->CreateUniquePlayerId(SenderId.ToString());
					ReceivedPartyInvites.AddUnique(FReceivedPartyInvite(SenderIdPtr.ToSharedRef(), PartyInvite));
					break;
				}
			}
		}
	}
}

void FFriendsAndChatManager::ProcessCommandLineInvites()
{
	if (OnlineSub != NULL && OnlineSub->GetPartyInterface().IsValid())
	{
		TSharedPtr<IOnlinePartyJoinInfo> AutoAcceptJoinInfo = OnlineSub->GetPartyInterface()->ConsumePendingCommandLineInvite();
		if (AutoAcceptJoinInfo.IsValid())
		{
			ReceivedPartyInvites.Add(FReceivedPartyInvite(AutoAcceptJoinInfo->GetLeaderId(), AutoAcceptJoinInfo.ToSharedRef()));

			AutoAcceptPartyId = AutoAcceptJoinInfo->GetPartyId()->ToString();
		}
	}
}

void FFriendsAndChatManager::ProcessReceivedGameInvites()
{
	if (OnlineSub != NULL &&
		OnlineSub->GetUserInterface().IsValid())
	{
		// list of newly added game invites
		TArray<TSharedPtr<IFriendItem>> UpdatedGameInvites;
		// received game invites waiting to be processed
		for (int32 Idx = 0; Idx < ReceivedGameInvites.Num(); Idx++)
		{
			const FReceivedGameInvite& Invite = ReceivedGameInvites[Idx];

			TSharedPtr<const FUniqueNetId> MySessionId = GetGameSessionId();
			bool bMySessionValid = MySessionId.IsValid() && MySessionId->IsValid();

			if (!Invite.InviteResult->Session.SessionInfo.IsValid() ||
				(bMySessionValid && Invite.InviteResult->Session.SessionInfo->GetSessionId().ToString() == MySessionId->ToString()))
			{
				// remove invites if user is already in the game session
				ReceivedGameInvites.RemoveAt(Idx--);
			}
			else
			{
				// add to list of pending invites to accept
				TSharedPtr<FOnlineUser> UserInfo;
				TSharedPtr<IFriendItem> Friend = FindUser(*Invite.FromId);
				TSharedPtr<FOnlineFriend> OnlineFriend;
				if (Friend.IsValid())
				{
					UserInfo = Friend->GetOnlineUser();
					OnlineFriend = Friend->GetOnlineFriend();
				}
				if (!UserInfo.IsValid())
				{
					UserInfo = OnlineSub->GetUserInterface()->GetUserInfo(LocalControllerIndex, *Invite.FromId);
				}
				if (UserInfo.IsValid() && OnlineFriend.IsValid())
				{
					TSharedPtr<FFriendGameInviteItem> GameInvite = MakeShareable(
						new FFriendGameInviteItem(UserInfo.ToSharedRef(), Invite.InviteResult, Invite.ClientId, OnlineFriend.ToSharedRef(), SharedThis(this))
						);
					PendingGameInvitesList.Add(Invite.FromId->ToString(), GameInvite);
					UpdatedGameInvites.Add(GameInvite);
					ReceivedGameInvites.RemoveAt(Idx--);
				}
			}
		}

		// received party invites waiting to be processed
		for (int32 Idx = 0; Idx < ReceivedPartyInvites.Num(); Idx++)
		{
			const FReceivedPartyInvite& Invite = ReceivedPartyInvites[Idx];
			// user info should already be queried by now
			TSharedPtr<FOnlineUser> UserInfo;
			UserInfo = OnlineSub->GetUserInterface()->GetUserInfo(LocalControllerIndex, *Invite.FromId);
			if (UserInfo.IsValid())
			{
				TSharedPtr<FFriendPartyInviteItem> PartyInvite = MakeShareable(
					new FFriendPartyInviteItem(UserInfo.ToSharedRef(), Invite.PartyJoinInfo->GetClientId(), Invite.PartyJoinInfo, SharedThis(this))
					);
				PendingGameInvitesList.Add(Invite.FromId->ToString(), PartyInvite);
				UpdatedGameInvites.Add(PartyInvite);
				ReceivedPartyInvites.RemoveAt(Idx--);

				if (!AutoAcceptPartyId.IsEmpty() && (AutoAcceptPartyId == PartyInvite->GetPartyJoinInfo()->GetPartyId()->ToString()))
				{
					AutoAcceptPartyId.Empty();
					AcceptGameInvite(PartyInvite);
				}
			}
		}

		// notifications for any newly added invtes
		if (UpdatedGameInvites.Num() > 0)
		{
			OnGameInvitesUpdated().Broadcast();
		}
		for (auto NewInvite : UpdatedGameInvites)
		{
			SendGameInviteNotification(NewInvite);
		}
	}
}

bool FFriendsAndChatManager::RequestGameInviteUserInfo()
{
	bool bPending = false;

	// query for user ids that are not already cached from game invites
	IOnlineUserPtr UserInterface = OnlineSub->GetUserInterface();
	IOnlineIdentityPtr IdentityInterface = OnlineSub->GetIdentityInterface();
	if (UserInterface.IsValid() &&
		IdentityInterface.IsValid())
	{
		TArray<TSharedRef<const FUniqueNetId>> GameInviteUserIds;
		for (auto GameInvite : ReceivedGameInvites)
		{
			GameInviteUserIds.Add(GameInvite.FromId);
		}
		for (auto PartyInvite : ReceivedPartyInvites)
		{
			GameInviteUserIds.Add(PartyInvite.FromId);
		}
		if (GameInviteUserIds.Num() > 0)
		{
			UserInterface->QueryUserInfo(LocalControllerIndex, GameInviteUserIds);
			bPending = true;
		}
	}

	return bPending;
}

void FFriendsAndChatManager::SendGameInviteNotification(const TSharedPtr<IFriendItem>& FriendItem)
{
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("Username"), FText::FromString(FriendItem->GetName()));
		const FText FriendRequestMessage = FText::Format(LOCTEXT("FFriendsAndChatManager_GameInvite", "Game invite from {Username}"), Args);
		TSharedPtr< FFriendsAndChatMessage > NotificationMessage = MakeShareable(new FFriendsAndChatMessage(FriendRequestMessage.ToString(), FriendItem->GetUniqueID()));
		NotificationMessage->SetMessageType(EFriendsRequestType::GameInvite);

		NotificationMessage->SetButtonCallback( FOnClicked::CreateSP(this, &FFriendsAndChatManager::HandleGameInviteResponse, NotificationMessage, EFriendsResponseType::Response_Accept));
		NotificationMessage->SetButtonCallback( FOnClicked::CreateSP(this, &FFriendsAndChatManager::HandleGameInviteResponse, NotificationMessage, EFriendsResponseType::Response_Reject));
		NotificationMessage->SetButtonDescription(LOCTEXT("FFriendsAndChatManager_GameInviteJoin", "Join Game"));
		NotificationMessage->SetButtonDescription(LOCTEXT("FFriendsAndChatManager_GameInviteReject", "Reject"));
		NotificationMessage->SetButtonStyle(TEXT("FriendsListEmphasisButton"));
		NotificationMessage->SetButtonStyle(TEXT("FriendsListCriticalButton"));
		NotificationMessage->SetMessageType(EFriendsRequestType::FriendInvite);
		OnFriendsActionNotification().Broadcast(NotificationMessage.ToSharedRef());
	}

	FriendsListNotificationDelegate.Broadcast(true);
}

void FFriendsAndChatManager::SendChatMessageReceivedEvent(EChatMessageType::Type ChatType, TSharedPtr<IFriendItem> FriendItem)
{
	OnChatMessageRecieved().Broadcast(ChatType, FriendItem);
}

void FFriendsAndChatManager::OnGameDestroyed(const FName SessionName, bool bWasSuccessful)
{
	if (SessionName == GameSessionName)
	{
		RequestRecentPlayersListRefresh();
	}
}

void FFriendsAndChatManager::OnPartyMemberJoined(const FUniqueNetId& LocalUserId, const FOnlinePartyId& PartyId, const FUniqueNetId& MemberId)
{
	//ToDo - NickDavies - re-add party 
	// If now in an active party, make party chat available & the default output channel
	//GetChatViewModel()->UpdateInPartyUI();
}

void FFriendsAndChatManager::OnPartyMemberExited(const FUniqueNetId& LocalUserId, const FOnlinePartyId& PartyId, const FUniqueNetId& MemberId, const EMemberExitedReason Reason)
{
	RequestRecentPlayersListRefresh();
	//ToDo - NickDavies - re-add party 
	// If no longer in an active party, make party chat unavailable & no longer the default output channel
	//GetChatViewModel()->UpdateInPartyUI();
}

void FFriendsAndChatManager::RejectGameInvite(const TSharedPtr<IFriendItem>& FriendItem)
{
	TSharedPtr<IFriendItem>* Existing = PendingGameInvitesList.Find(FriendItem->GetUniqueID()->ToString());
	if (Existing != nullptr)
	{
		(*Existing)->SetPendingDelete();
		PendingGameInvitesList.Remove(FriendItem->GetUniqueID()->ToString());
	}
	// update game invite UI
	OnGameInvitesUpdated().Broadcast();

	// reject party invite
	TSharedPtr<IOnlinePartyJoinInfo> PartyInfo = FriendItem->GetPartyJoinInfo();
	if (PartyInfo.IsValid())
	{
		TSharedPtr<const FUniqueNetId> UserId = OnlineIdentity->GetUniquePlayerId(LocalControllerIndex);
		if (UserId.IsValid())
		{
			OnlineSub->GetPartyInterface()->RejectInvitation(*UserId, *PartyInfo->GetPartyId());
		}
	}

	if (OnlineIdentity.IsValid() &&
		OnlineIdentity->GetUniquePlayerId(LocalControllerIndex).IsValid())
	{
		Analytics->FireEvent_RejectGameInvite(*OnlineIdentity->GetUniquePlayerId(LocalControllerIndex), *FriendItem->GetUniqueID());
	}
	OnFriendsNotification().Broadcast(false);
}

void FFriendsAndChatManager::AcceptGameInvite(const TSharedPtr<IFriendItem>& FriendItem)
{
	TSharedPtr<IFriendItem>* Existing = PendingGameInvitesList.Find(FriendItem->GetUniqueID()->ToString());
	if (Existing != nullptr)
	{
		(*Existing)->SetPendingDelete();
		PendingGameInvitesList.Remove(FriendItem->GetUniqueID()->ToString());
	}
	// update game invite UI
	OnGameInvitesUpdated().Broadcast();

	FString AdditionalCommandline;

	TSharedPtr<IOnlinePartyJoinInfo> PartyInfo = FriendItem->GetPartyJoinInfo();
	if (PartyInfo.IsValid())
	{
		// notify for further processing of join party request 
		OnFriendsJoinParty().Broadcast(*FriendItem->GetUniqueID(), PartyInfo.ToSharedRef(), true);

		AdditionalCommandline = OnlineSub->GetPartyInterface()->MakeTokenFromJoinInfo(*PartyInfo);
	}
	else
	{
		// notify for further processing of join game request 
		OnFriendsJoinGame().Broadcast(*FriendItem->GetUniqueID(), *FriendItem->GetGameSessionId());
		
		AdditionalCommandline = 
			TEXT("-invitesession=") + FriendItem->GetGameSessionId()->ToString() + 
			TEXT(" -invitefrom=") + FriendItem->GetUniqueID()->ToString();
	}

	// launch game with invite cmd line parameters
	TSharedPtr<IFriendsApplicationViewModel>* FriendsApplicationViewModel = ApplicationViewModels.Find(FriendItem->GetClientId());
	if (FriendsApplicationViewModel != nullptr &&
		(*FriendsApplicationViewModel).IsValid())
	{
		(*FriendsApplicationViewModel)->LaunchFriendApp(AdditionalCommandline);
	}

	if (OnlineIdentity.IsValid() &&
		OnlineIdentity->GetUniquePlayerId(LocalControllerIndex).IsValid())
	{
		Analytics->FireEvent_AcceptGameInvite(*OnlineIdentity->GetUniquePlayerId(LocalControllerIndex), *FriendItem->GetUniqueID());
	}
	OnFriendsNotification().Broadcast(false);
}

void FFriendsAndChatManager::SendGameInvite(const TSharedPtr<IFriendItem>& FriendItem)
{
	SendGameInvite(*FriendItem->GetUniqueID());
}

bool FFriendsAndChatManager::SendPartyInvite(const FUniqueNetId& ToUser)
{
	bool bResult = false;

	if (OnlineSub != nullptr &&
		OnlineIdentity.IsValid())
	{
		TSharedPtr<const FUniqueNetId> UserId = OnlineIdentity->GetUniquePlayerId(LocalControllerIndex);
		if (UserId.IsValid())
		{
			// party invite if available
			if (OnlineSub->GetPartyInterface().IsValid())
			{
				TSharedPtr<IOnlinePartyJoinInfo> PartyJoinInfo = OnlineSub->GetPartyInterface()->GetAdvertisedParty(*UserId, *UserId);
				if (PartyJoinInfo.IsValid())
				{
					bResult = OnlineSub->GetPartyInterface()->SendInvitation(*UserId, *PartyJoinInfo->GetPartyId(), ToUser);
				}
			}
		}
	}

	return bResult;
}

void FFriendsAndChatManager::SendGameInvite(const FUniqueNetId& ToUser)
{
	if (OnlineSub != nullptr &&
		OnlineIdentity.IsValid())
	{
		TSharedPtr<const FUniqueNetId> UserId = OnlineIdentity->GetUniquePlayerId(0);
		if (UserId.IsValid())
		{
			// party invite if available
			if (!SendPartyInvite(ToUser))
			{
				// just a game session invite
				if (OnlineSub->GetSessionInterface().IsValid() &&
					OnlineSub->GetSessionInterface()->GetNamedSession(GameSessionName) != nullptr)
				{
					OnlineSub->GetSessionInterface()->SendSessionInviteToFriend(*UserId, GameSessionName, ToUser);
				}
			}

			Analytics->FireEvent_SendGameInvite(*OnlineIdentity->GetUniquePlayerId(LocalControllerIndex), ToUser);
		}
	}
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
	TSharedPtr< IFriendItem > Friend = FindUser(FriendId);
	if(Friend.IsValid())
	{
		SendInviteAcceptedNotification(Friend);
		Friend->SetPendingAccept();
	}
	RequestListRefresh();
}

void FFriendsAndChatManager::OnAcceptInviteComplete( int32 LocalPlayer, bool bWasSuccessful, const FUniqueNetId& FriendId, const FString& ListName, const FString& ErrorStr )
{
	PendingOutgoingAcceptFriendRequests.Remove(FriendId.ToString());

	// Do something with an accepted invite
	if ( PendingOutgoingAcceptFriendRequests.Num() > 0 )
	{
		SetState( EFriendsAndManagerState::AcceptingFriendRequest );
	}
	else
	{
		RequestListRefresh();
		SetState( EFriendsAndManagerState::Idle );
	}
}

void FFriendsAndChatManager::OnDeleteFriendComplete( int32 LocalPlayer, bool bWasSuccessful, const FUniqueNetId& DeletedFriendID, const FString& ListName, const FString& ErrorStr )
{
	PendingOutgoingDeleteFriendRequests.Remove(DeletedFriendID.ToString());

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

#undef LOCTEXT_NAMESPACE
