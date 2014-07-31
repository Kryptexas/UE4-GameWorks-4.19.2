// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "OnlineConnectionInterface.h"
#include "OnlineSubsystemTypes.h"
#include "AndroidRuntimeSettings.h"
#include <jni.h>

extern "C" void Java_com_epicgames_ue4_GameActivity_nativeCompletedConnection(JNIEnv* LocalJNIEnv, jobject LocalThiz, jint errorCode);

/**
 *	IOnlineConnection - Interface class for connection management
 */
class FOnlineConnectionGooglePlay : public IOnlineConnection
{
private:

	/** Reference to the main GameCenter subsystem */
	class FOnlineSubsystemGooglePlay* AndroidSubsystem;
	
	/** hide the default constructor, we need a reference to our OSS */
	FOnlineConnectionGooglePlay() {}

	
	/**
	 * Native function called from Java when we get the connection result callback.
	 *
	 * @param LocalJNIEnv the current JNI environment
	 * @param LocalThiz instance of the Java GameActivity class
	 * @param errorCode The reported error code from the connection attempt
	 */
	friend void Java_com_epicgames_ue4_GameActivity_nativeCompletedConnection(JNIEnv* LocalJNIEnv, jobject LocalThiz, jint errorCode);

	struct FPendingConnection
	{
		FOnlineConnectionGooglePlay * ConnectionInterface;
		FOnConnectionCompleteDelegate Delegate;
		bool IsConnectionPending;
	};


	/** Instance of the connection context data */
	static FPendingConnection PendingConnectRequest;
public:

	/**
	 * Constructor
	 *
	 * @param InSubsystem - A reference to the owning subsystem
	 */
	FOnlineConnectionGooglePlay( class FOnlineSubsystemGooglePlay* InSubsystem );

	virtual void Connect(const FOnConnectionCompleteDelegate & Delegate = FOnConnectionCompleteDelegate()) override;
};

typedef TSharedPtr<FOnlineConnectionGooglePlay, ESPMode::ThreadSafe> FOnlineConnectionGooglePlayPtr;
