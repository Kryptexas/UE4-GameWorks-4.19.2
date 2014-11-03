// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "OnlineSubsystemPackage.h"
#include "OnlineNotificationTransportInterface.h"

/**
* unique identifier for notification transports
*/
typedef FString FNotificationTransportId;

/**
* Details of an entitlement
*/
struct FOnlineNotificationTransport
{
	/** Unique notification transport id associated with this transport */
	FNotificationTransportId Id;

	//TODO
	//delegate for receiving
	//delegate for sending

	/**
	* Equality operator
	*/
	bool operator==(const FOnlineNotificationTransport& Other) const
	{
		return Other.Id == Id;
	}

	bool SendNotification(const FOnlineNotification& Notification)
	{
		return true;
	}
};

/** What happened in the handler function */
enum class EOnlineNotificationResult
{
	None,		// No handling occurred
	Handled,	// Notification was handled
};

/**
* Macro to parse a notification struct into a passed in UStruct.
* Example:
*
* FNotificationData Data;
*
* if (READ_NOTIFICATION_JSON(Data,Notification)) {}
*
* You'll need to include JsonUtilities.h to use it
*/
#define READ_NOTIFICATION_JSON(Struct, Notification) FJsonObjectConverter::JsonObjectToUStruct(Notification.Payload->AsObject().ToSharedRef(), Struct.StaticStruct(), &Struct, 0, 0)

/**
* Delegate type for handling a notification
*
* The first parameter is a notification structure
* Return result code to indicate if notification has been handled
*/
DECLARE_DELEGATE_RetVal_OneParam(EOnlineNotificationResult, FHandleOnlineNotificationSignature, const FOnlineNotification&);


/** Struct to keep track of bindings */
struct FOnlineNotificationBinding
{
	/** Delegate to call when this binding is activated */
	FHandleOnlineNotificationSignature NotificationDelegate;

	bool operator==(const FOnlineNotificationBinding& Other) const
	{
		return NotificationDelegate == Other.NotificationDelegate;
	}

	FOnlineNotificationBinding()
	{}

	FOnlineNotificationBinding(const FHandleOnlineNotificationSignature& InNotificationDelegate)
		: NotificationDelegate(InNotificationDelegate)
	{}
};

/** This class is a static manager used to track notification transports and map the delivered notifications to subscribed notification handlers */
class ONLINESUBSYSTEM_API FOnlineNotificationHandler
{

protected:

	typedef TMap< FString, TArray<FOnlineNotificationBinding> > NotificationTypeBindingsMap;

	/** Map from type of notification to the delegate to call */
	NotificationTypeBindingsMap SystemBindingMap;

	/** Map from player and type of notification to the delegate to call */
	TMap< FString, NotificationTypeBindingsMap > PlayerBindingMap;

	/** Map from a transport type to the transport object */
	TMap< FString, FOnlineNotificationTransport > TransportMap;

public:

	FOnlineNotificationHandler()
	{
	}

	static FOnlineNotificationHandler Singleton;

	// SYSTEM NOTIFICATION BINDINGS

	/** Add a notification binding for a type */
	void AddSystemNotificationBinding(FString NotificationType, const FOnlineNotificationBinding& NewBinding);

	/** Remove the notification handler for a type */
	void RemoveSystemNotificationBinding(FString NotificationType, const FOnlineNotificationBinding& NewBinding);

	/** Resets all system notification handlers */
	void ResetSystemNotificationBindings();

	// PLAYER NOTIFICATION BINDINGS

	/** Add a notification binding for a type */
	void AddPlayerNotificationBinding(const FUniqueNetId& PlayerId, FString NotificationType, const FOnlineNotificationBinding& NewBinding);

	/** Remove the player notification handler for a type */
	void RemovePlayerNotificationBinding(const FUniqueNetId& PlayerId, FString NotificationType, const FOnlineNotificationBinding& NewBinding);

	/** Resets a player's notification handlers */
	void ResetPlayerNotificationBindings(const FUniqueNetId& PlayerId);

	/** Resets all player notification handlers */
	void ResetAllPlayerNotificationBindings();

	// RECEIVING NOTIFICATIONS

	/** Deliver a notification to the appropriate handler for that player/msg type.  Called by NotificationTransport implementations. */
	void DeliverNotification(const FOnlineNotification& Notification);

	// NOTIFICATION TRANSPORTS / SENDING

	/** Add a notification transport */
	void AddNotificationTransport(FString TransportType, const FOnlineNotificationTransport& Transport);

	/** Remove a notification transport */
	void RemoveNotificationTransport(FString TransportType);

	/** Send a notification using a specific transport */
	bool SendNotification(FString TransportType, const FOnlineNotification& Notification);

	/** Resets all transports */
	void ResetNotificationTransports();
};
