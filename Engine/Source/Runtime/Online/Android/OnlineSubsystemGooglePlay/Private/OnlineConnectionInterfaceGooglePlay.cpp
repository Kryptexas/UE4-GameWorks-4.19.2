// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemGooglePlayPrivatePCH.h"
#include "OnlineAsyncTaskManagerGooglePlay.h"
#include "Online.h"
#include "Android/AndroidJNI.h"

FOnlineConnectionGooglePlay::FPendingConnection FOnlineConnectionGooglePlay::PendingConnectRequest;

// Java interface to deal with callbacks
extern "C" void Java_com_epicgames_ue4_GameActivity_nativeCompletedConnection(JNIEnv* LocalJNIEnv, jobject LocalThiz, jint errorCode)
{
	auto ConnectionInterface = FOnlineConnectionGooglePlay::PendingConnectRequest.ConnectionInterface;

	if (!ConnectionInterface ||
		!ConnectionInterface->AndroidSubsystem ||
		!ConnectionInterface->AndroidSubsystem->GetAsyncTaskManager())
	{
		// We should call the delegate with a false parameter here, but if we don't have
		// the async task manager we're not going to call it on the game thread.
		return;
	}

	ConnectionInterface->AndroidSubsystem->GetAsyncTaskManager()->AddGenericToOutQueue([errorCode]()
	{
		auto& PendingConnection = FOnlineConnectionGooglePlay::PendingConnectRequest;
		bool failed = errorCode == -1; // find out what a 'no fail' error code might be...

		PendingConnection.Delegate.ExecuteIfBound(errorCode, failed);
		PendingConnection.Delegate.Unbind();
		PendingConnection.IsConnectionPending = false;
	});

}

FOnlineConnectionGooglePlay::FOnlineConnectionGooglePlay( FOnlineSubsystemGooglePlay* InSubsystem )
	: AndroidSubsystem(InSubsystem)
{
	PendingConnectRequest.ConnectionInterface = this;
}

void FOnlineConnectionGooglePlay::Connect(const FOnConnectionCompleteDelegate & Delegate)
{
	if (PendingConnectRequest.IsConnectionPending)
	{
		UE_LOG(LogOnline, Warning, TEXT("FOnlineConnectionGooglePlay::Connect: Cannot start a new connection attempt while one is in progress."));
		Delegate.ExecuteIfBound(0,false); // Need an error code here...
		return;
	}

	PendingConnectRequest.Delegate = Delegate;
	PendingConnectRequest.IsConnectionPending = true;

	// Kick off the connect process
	extern void AndroidThunkCpp_GooglePlayConnect();
	FPlatformMisc::LowLevelOutputDebugStringf(TEXT("In FOnlineConnectionGooglePlay::Connect()"));
	AndroidThunkCpp_GooglePlayConnect();
}