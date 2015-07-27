// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "SFriendsContainer.h"
#include "SFriendUserHeader.h"
#include "SFriendsStatusCombo.h"
#include "SChatWindow.h"
#include "SChatChrome.h"
#include "SChatChromeFullSettings.h"
#include "SNotificationList.h"
#include "SWindowTitleBar.h"
#include "SFriendUserHeader.h"

#include "ChatChromeViewModel.h"
#include "ChatChromeTabViewModel.h"
#include "FriendsViewModel.h"
#include "FriendViewModel.h"
#include "FriendsStatusViewModel.h"
#include "FriendsUserViewModel.h"
#include "ChatViewModel.h"
#include "FriendRecentPlayerItems.h"
#include "FriendGameInviteItem.h"
#include "FriendPartyInviteItem.h"
#include "ClanRepository.h"
#include "IFriendList.h"
#include "FriendsNavigationService.h"
#include "ChatNotificationService.h"
#include "FriendsChatMarkupService.h"
#include "ChatDisplayService.h"
#include "ChatSettingsService.h"
#include "FriendsAndChatAnalytics.h"

#include "OSSScheduler.h"
#include "FriendsService.h"
#include "GameAndPartyService.h"

#define LOCTEXT_NAMESPACE "FriendsAndChatManager"


/* FFriendsAndChatManager structors
 *****************************************************************************/

FFriendsAndChatManager::FFriendsAndChatManager()
	{}

FFriendsAndChatManager::~FFriendsAndChatManager( )
{
}

void FFriendsAndChatManager::Initialize()
{
	Analytics = FFriendsAndChatAnalyticsFactory::Create();
	OSSScheduler = FOSSSchedulerFactory::Create(Analytics);
	
	NavigationService = FFriendsNavigationServiceFactory::Create();
	NotificationService = FChatNotificationServiceFactory::Create();

	FriendsService = FFriendsServiceFactory::Create(OSSScheduler.ToSharedRef(), NotificationService.ToSharedRef());
	MessageService = FMessageServiceFactory::Create(OSSScheduler.ToSharedRef(), FriendsService.ToSharedRef());
	GameAndPartyService = FGameAndPartyServiceFactory::Create(OSSScheduler.ToSharedRef(), FriendsService.ToSharedRef(), NotificationService.ToSharedRef(), false);
	
	
	
	FriendViewModelFactory = FFriendViewModelFactoryFactory::Create(NavigationService.ToSharedRef(), FriendsService.ToSharedRef(), GameAndPartyService.ToSharedRef());
	FriendsListFactory = FFriendListFactoryFactory::Create(FriendViewModelFactory.ToSharedRef(), FriendsService.ToSharedRef(), GameAndPartyService.ToSharedRef());
	TSharedRef<IChatCommunicationService> CommunicationService = StaticCastSharedRef<IChatCommunicationService>(MessageService.ToSharedRef());
	MarkupServiceFactory = FFriendsChatMarkupServiceFactoryFactory::Create(CommunicationService, NavigationService.ToSharedRef(), FriendsListFactory.ToSharedRef());
}

/* IFriendsAndChatManager interface
 *****************************************************************************/
void FFriendsAndChatManager::Logout()
{
	if (OSSScheduler->IsLoggedIn())
	{
		GameAndPartyService->Logout();

		// Logout Message Service
		ChatRoomstoJoin.Empty();
		MessageService->OnChatPublicRoomJoined().RemoveAll(this);
		MessageService->Logout();

		// Logout Friends Service
		FriendsService->OnAddToast().RemoveAll(this);
		FriendsService->Logout();

		// Logout OSS
		OSSScheduler->Logout();
	}
}

void FFriendsAndChatManager::Login(IOnlineSubsystem* InOnlineSub, bool bInIsGame, int32 LocalUserID)
{
	// OSS login
	OSSScheduler->Login(InOnlineSub, bInIsGame, LocalUserID);

	// Friends Service Login
	FriendsService->Login();
	FriendsService->OnAddToast().AddSP(this, &FFriendsAndChatManager::AddFriendsToast);

	// Message Service Login
	MessageService->Login();
	MessageService->OnChatPublicRoomJoined().AddSP(this, &FFriendsAndChatManager::OnChatPublicRoomJoined);
	for (auto RoomName : ChatRoomstoJoin)
	{
		MessageService->JoinPublicRoom(RoomName);
	}

	// Game Party Service
	GameAndPartyService->Login();
}

bool FFriendsAndChatManager::IsLoggedIn()
{
	return OSSScheduler->IsLoggedIn();
}

void FFriendsAndChatManager::SetOnline()
{
	OSSScheduler->SetOnline();
}

void FFriendsAndChatManager::SetAway()
{
	OSSScheduler->SetAway();
}

EOnlinePresenceState::Type FFriendsAndChatManager::GetOnlineStatus()
{
	return OSSScheduler->GetOnlineStatus();
}

void FFriendsAndChatManager::AddApplicationViewModel(const FString ClientID, TSharedPtr<IFriendsApplicationViewModel> InApplicationViewModel)
{
	GameAndPartyService->AddApplicationViewModel(ClientID, InApplicationViewModel);
}

void FFriendsAndChatManager::ClearApplicationViewModels()
{
	GameAndPartyService->ClearApplicationViewModels();
}

void FFriendsAndChatManager::AddRecentPlayerNamespace(const FString& Namespace)
{
	FriendsService->AddRecentPlayerNamespace(Namespace);
}

void FFriendsAndChatManager::SetAnalyticsProvider(const TSharedPtr<IAnalyticsProvider>& AnalyticsProvider)
{
	Analytics->SetProvider(AnalyticsProvider);
}

void FFriendsAndChatManager::InsertNetworkChatMessage(const FString& InMessage)
{
	MessageService->InsertNetworkMessage(InMessage);
}

void FFriendsAndChatManager::JoinPublicChatRoom(const FString& RoomName)
{
	if (!RoomName.IsEmpty() && MessageService->IsInGlobalChat() == false)
	{
		ChatRoomstoJoin.AddUnique(RoomName);
		MessageService->JoinPublicRoom(RoomName);
	}
}

void FFriendsAndChatManager::OnChatPublicRoomJoined(const FString& ChatRoomID)
{
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
			 SNew(SFriendsContainer, FFriendsViewModelFactory::Create(FriendsService.ToSharedRef(), GameAndPartyService.ToSharedRef(), MessageService.ToSharedRef(), ClanRepository.ToSharedRef(), FriendsListFactory.ToSharedRef(), NavigationService.ToSharedRef()))
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
	NotificationService->OnNotificationsAvailable().Broadcast(false);
	return FriendListWidget;
}

TSharedPtr< SWidget > FFriendsAndChatManager::GenerateStatusWidget( const FFriendsAndChatStyle* InStyle, bool ShowStatusOptions )
{
	if(ShowStatusOptions)
	{
		check(ClanRepository.IsValid());

		TSharedRef<FFriendsViewModel> FriendsViewModel = FFriendsViewModelFactory::Create(FriendsService.ToSharedRef(), GameAndPartyService.ToSharedRef(), MessageService.ToSharedRef(), ClanRepository.ToSharedRef(), FriendsListFactory.ToSharedRef(), NavigationService.ToSharedRef());

		TSharedPtr<SWidget> HeaderWidget = SNew(SBox)
		.WidthOverride(Style.FriendsListStyle.FriendsListWidth)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SFriendsStatusCombo, FriendsViewModel->GetStatusViewModel(), FriendsViewModel->GetUserViewModel())
				.FriendStyle(InStyle)
			]
		];
		return HeaderWidget;
	}

	return SNew(SFriendUserHeader, FFriendsUserViewModelFactory::Create(FriendsService.ToSharedRef()))
		.FriendStyle( InStyle )
		.FontStyle(&InStyle->FriendsNormalFontStyle);
}

TSharedPtr< SWidget > FFriendsAndChatManager::GenerateChatWidget(const FFriendsAndChatStyle* InStyle, TAttribute<FText> ActivationHintDelegate, EChatMessageType::Type MessageType, TSharedPtr<IFriendItem> FriendItem, TSharedPtr< SWindow > WindowPtr)
{
	TSharedRef<IChatDisplayService> ChatDisplayService = GenerateChatDisplayService();
	Style = *InStyle;

	TSharedPtr<FChatViewModel> ViewModel = FChatViewModelFactory::Create(FriendViewModelFactory.ToSharedRef(), MessageService.ToSharedRef(), NavigationService.ToSharedRef(), MarkupServiceFactory->Create(), ChatDisplayService, FriendsService.ToSharedRef(), GameAndPartyService.ToSharedRef(), EChatViewModelType::Windowed);
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

void FFriendsAndChatManager::AddFriendsToast(const FText Message)
{
	if (FriendsNotificationBox.IsValid())
	{
		FNotificationInfo Info(Message);
		Info.ExpireDuration = 2.0f;
		Info.bUseLargeFont = false;
		FriendsNotificationBox->AddNotification(Info);
	}
}

TSharedPtr<FFriendsNavigationService> FFriendsAndChatManager::GetNavigationService()
{
	return NavigationService;
}

TSharedPtr<IChatNotificationService> FFriendsAndChatManager::GetNotificationService()
{
	return NotificationService;
}

TSharedPtr<IChatCommunicationService> FFriendsAndChatManager::GetCommunicationService()
{
	return MessageService;
}

TSharedPtr<IGameAndPartyService> FFriendsAndChatManager::GetGameAndPartyService()
{
	return GameAndPartyService;
}

TSharedRef< IChatDisplayService > FFriendsAndChatManager::GenerateChatDisplayService(bool FadeChatList, bool FadeChatEntry, float ListFadeTime, float EntryFadeTime)
{
	return FChatDisplayServiceFactory::Create(MessageService.ToSharedRef(), FadeChatList, FadeChatEntry, ListFadeTime, EntryFadeTime);
}

TSharedRef< IChatSettingsService > FFriendsAndChatManager::GenerateChatSettingsService()
{
	return FChatSettingsServiceFactory::Create();
}

TSharedPtr< SWidget > FFriendsAndChatManager::GenerateChromeWidget(const struct FFriendsAndChatStyle* InStyle, TSharedRef<IChatDisplayService> ChatDisplayService, TSharedRef<IChatSettingsService> InChatSettingsService)
{
	Style = *InStyle;
	TSharedPtr<FChatChromeViewModel> ChromeViewModel = FChatChromeViewModelFactory::Create(NavigationService.ToSharedRef(), ChatDisplayService, InChatSettingsService);

	TSharedRef<FChatViewModel> CustomChatViewModel = FChatViewModelFactory::Create(FriendViewModelFactory.ToSharedRef(), MessageService.ToSharedRef(), NavigationService.ToSharedRef(), MarkupServiceFactory->Create(), ChatDisplayService, FriendsService.ToSharedRef(), GameAndPartyService.ToSharedRef(), EChatViewModelType::Base);
	CustomChatViewModel->SetChannelFlags(EChatMessageType::Global | EChatMessageType::Whisper | EChatMessageType::Game);
	CustomChatViewModel->SetOutgoingMessageChannel(EChatMessageType::Global);

	TSharedRef<FChatViewModel> GlobalChatViewModel = FChatViewModelFactory::Create(FriendViewModelFactory.ToSharedRef(), MessageService.ToSharedRef(), NavigationService.ToSharedRef(), MarkupServiceFactory->Create(), ChatDisplayService, FriendsService.ToSharedRef(), GameAndPartyService.ToSharedRef(), EChatViewModelType::Base);
	GlobalChatViewModel->SetChannelFlags(EChatMessageType::Global);
	GlobalChatViewModel->SetOutgoingMessageChannel(EChatMessageType::Global);

	TSharedRef<FChatViewModel> WhisperChatViewModel = FChatViewModelFactory::Create(FriendViewModelFactory.ToSharedRef(), MessageService.ToSharedRef(), NavigationService.ToSharedRef(), MarkupServiceFactory->Create(), ChatDisplayService, FriendsService.ToSharedRef(), GameAndPartyService.ToSharedRef(), EChatViewModelType::Base);
	WhisperChatViewModel->SetChannelFlags(EChatMessageType::Whisper);
	WhisperChatViewModel->SetOutgoingMessageChannel(EChatMessageType::Whisper);

	ChromeViewModel->AddTab(FChatChromeTabViewModelFactory::Create(CustomChatViewModel));
	ChromeViewModel->AddTab(FChatChromeTabViewModelFactory::Create(GlobalChatViewModel));
	ChromeViewModel->AddTab(FChatChromeTabViewModelFactory::Create(WhisperChatViewModel));

	TSharedPtr<SChatChrome> ChatChrome = SNew(SChatChrome, ChromeViewModel.ToSharedRef()).FriendStyle(InStyle);
	
	return ChatChrome;
}

#undef LOCTEXT_NAMESPACE