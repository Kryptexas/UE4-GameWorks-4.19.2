// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "OnlineSubsystemPackage.h"

/** This class should be embedded inside other classes, and is used to bind delegates to OnlineNotifications */

/** What happened in the handler function */
namespace EOnlineNotificationResult
{
	enum Type
	{
		None,		// No handling occurred
		Block,		// Notification was handled and should not be forwarded
		Forward,	// Notification handled, forward to other 
	};
}

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
#define READ_NOTIFICATION_JSON(Struct, Notification) FJsonObjectConverter::JsonObjectStringToUStruct(Notification.PayloadStr, Struct.StaticStruct(), &Struct, 0, 0)

/**
 * Delegate type for handling a notification
 *
 * The first parameter is a notification structure
 * Return result code to indicate if notification has been blocked or should be forwarded
 */
DECLARE_DELEGATE_RetVal_OneParam(EOnlineNotificationResult::Type, FHandleOnlineNotificationSignature, const FOnlineNotification&);


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


class ONLINESUBSYSTEM_API FOnlineNotificationHandler
{

protected:

	/** Map from name of notification to the delegate to call */
	TMap<FName, TArray<FOnlineNotificationBinding> > BindingMap;

	/** Call this binding for all non-blocked notifications */
	FOnlineNotificationBinding DefaultBinding;

	
public:

	FOnlineNotificationHandler()
	{
	}

	/** Add a new binding.  */
	void AddNotificationBinding(FName NotificationName, const FOnlineNotificationBinding& NewBinding);

	/** Set the default binding, which is called after all other bindings */
	void SetDefaultNotificationBinding(const FOnlineNotificationBinding& NewBinding);

	/** Handle a notification, will call all applicable bindings, until a Block result is returned */
	void HandleNotification(const FOnlineNotification& Notification);

	/** Resets all bindings */
	void ResetBindings();
};