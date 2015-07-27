// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Result of find friend attempt
 */
namespace EFindFriendResult
{
	enum Type
	{
		Success,
		NotFound,
		AlreadyFriends,
		FriendsPending,
		AddingSelfFail
	};

	static const TCHAR* ToString(const EFindFriendResult::Type& Type)
	{
		switch (Type)
		{
		case Success:
			return TEXT("Success");
		case NotFound:
			return TEXT("NotFound");
		case AlreadyFriends:
			return TEXT("AlreadyFriends");
		case FriendsPending:
			return TEXT("FriendsPending");
		case AddingSelfFail:
			return TEXT("AddingSelfFail");
		default:
			return TEXT("");
		};
	}
};

/**
 * Implement the Friend and Chat manager
 */
class FFriendsAndChatManager
	: public IFriendsAndChatManager
	, public TSharedFromThis<FFriendsAndChatManager>
{
public:

	/** Default constructor. */
	FFriendsAndChatManager( );

	/** Destructor. */
	~FFriendsAndChatManager( );

	void Initialize();

public:

	// IFriendsAndChatManager
	virtual void Logout() override;
	virtual void Login(IOnlineSubsystem* InOnlineSub, bool bInIsGame = false, int32 LocalUserID = 0) override;
	virtual bool IsLoggedIn() override;
	virtual void SetOnline() override;
	virtual void SetAway() override;
	virtual EOnlinePresenceState::Type GetOnlineStatus() override;
	virtual void AddApplicationViewModel(const FString ClientID, TSharedPtr<IFriendsApplicationViewModel> ApplicationViewModel) override;
	virtual void AddRecentPlayerNamespace(const FString& Namespace) override;
	virtual void ClearApplicationViewModels() override;
	virtual TSharedRef< IChatDisplayService > GenerateChatDisplayService(bool FadeChatList = false, bool FadeChatEntry = false, float ListFadeTime = -1.f, float EntryFadeTime = -1.f) override;
	virtual TSharedRef< IChatSettingsService > GenerateChatSettingsService() override;
	virtual TSharedPtr< SWidget > GenerateChromeWidget(const struct FFriendsAndChatStyle* InStyle, TSharedRef<IChatDisplayService> ChatViewModel, TSharedRef<IChatSettingsService> ChatSettingsService) override;
	virtual void SetAnalyticsProvider(const TSharedPtr<IAnalyticsProvider>& AnalyticsProvider) override;
	virtual TSharedPtr< SWidget > GenerateFriendsListWidget( const FFriendsAndChatStyle* InStyle ) override;
	virtual TSharedPtr< SWidget > GenerateStatusWidget(const FFriendsAndChatStyle* InStyle, bool ShowStatusOptions) override;
	virtual TSharedPtr< SWidget > GenerateChatWidget(const FFriendsAndChatStyle* InStyle, TAttribute<FText> ActivationHintDelegate, EChatMessageType::Type MessageType, TSharedPtr<IFriendItem> FriendItem, TSharedPtr< SWindow > WindowPtr) override;
	virtual TSharedPtr<SWidget> GenerateFriendUserHeaderWidget(TSharedPtr<IFriendItem> FriendItem) override;
	virtual TSharedPtr<class FFriendsNavigationService> GetNavigationService() override;
	virtual TSharedPtr<class IChatNotificationService> GetNotificationService() override;
	virtual TSharedPtr<class IChatCommunicationService> GetCommunicationService() override;
	virtual TSharedPtr<class IGameAndPartyService> GetGameAndPartyService() override;
	virtual void InsertNetworkChatMessage(const FString& InMessage) override;
	virtual void JoinPublicChatRoom(const FString& RoomName) override;
	virtual void OnChatPublicRoomJoined(const FString& ChatRoomID) override;

	// External events
	virtual FAllowFriendsJoinGame& AllowFriendsJoinGame() override
	{
		return AllowFriendsJoinGameDelegate;
	}

private:

	/** Called when singleton is released. */
	void ShutdownManager();

	/**
	 * A ticker used to perform updates on the main thread.
	 *
	 * @param Delta The tick delta.
	 * @return true to continue ticking.
	 */
	bool Tick( float Delta );

	/** Add a friend toast. */
	void AddFriendsToast(const FText Message);

	/**
	 * Get the best spawn position for the next chat window
	 *
	 * @return Spawn position
	 */
	FVector2D GetWindowSpawnPosition() const;

private:

	// Holds the list messages sent out to be responded to
	TArray<TSharedPtr< FFriendsAndChatMessage > > NotificationMessages;

	/* Delegates
	*****************************************************************************/
	FAllowFriendsJoinGame AllowFriendsJoinGameDelegate;

	/* Services
	*****************************************************************************/
	// Manages private/public chat messages 
	TSharedPtr<class FMessageService> MessageService;
	// Manages navigation services
	TSharedPtr<class FFriendsNavigationService> NavigationService;
	// Manages notification services
	TSharedPtr<class FChatNotificationService> NotificationService;
	// Holds the friends view model facotry
	TSharedPtr<class IFriendViewModelFactory> FriendViewModelFactory;
	// Holds the friends list provider
	TSharedPtr<class IFriendListFactory> FriendsListFactory;
	// Holds the Markup service provider
	TSharedPtr<class IFriendsChatMarkupServiceFactory> MarkupServiceFactory;
	// Holds the Game and Party service
	TSharedPtr<class FGameAndPartyService> GameAndPartyService;
	// Holds the friends service
	TSharedPtr<class FFriendsService> FriendsService;
	// Holds the OSSScheduler 
	TSharedPtr<class FOSSScheduler> OSSScheduler;



	/* Chat stuff
	*****************************************************************************/

	// Keeps track of global chat rooms that have been requested to join
	TArray<FString> ChatRoomstoJoin;
	// Holds the clan repository
	TSharedPtr<class IClanRepository> ClanRepository;

	/* Manger state
	*****************************************************************************/

	// Holds the Friends List widget
	TSharedPtr< SWidget > FriendListWidget;
	
	// Holds the style used to create the Friends List widget
	FFriendsAndChatStyle Style;
	// Holds the toast notification
	TSharedPtr<SNotificationList> FriendsNotificationBox;

private:

	TSharedPtr<class FFriendsAndChatAnalytics> Analytics;
};
