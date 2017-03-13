// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "XmppStrophe/XmppStropheThread.h"
#include "XmppStrophe/XmppConnectionStrophe.h"
#include "XmppStrophe/StropheContext.h"
#include "XmppStrophe/StropheConnection.h"
#include "XmppStrophe/StropheStanza.h"

#include "HAL/RunnableThread.h"
#include "Misc/Guid.h"

#if WITH_XMPP_STROPHE

FXmppStropheThread::FXmppStropheThread(FXmppConnectionStrophe& InConnectionManager, const FXmppUserJid& InUser, const FString& InAuth, const FXmppServer& InServerConfiguration)
	: ConnectionManager(InConnectionManager)
	, StropheConnection(ConnectionManager.StropheContext)
	, ServerConfiguration(InServerConfiguration)
{
	// Setup connection
	StropheConnection.SetUserId(InUser);
	StropheConnection.SetPassword(InAuth);
	StropheConnection.SetKeepAlive(ServerConfiguration.PingTimeout, ServerConfiguration.PingInterval);

	ConnectRequest = true;

	static int32 ThreadInstanceIdx = 0;
	constexpr const int32 StackSize = 64 * 1024;
	ThreadPtr.Reset(FRunnableThread::Create(this, *FString::Printf(TEXT("XmppConnectionThread_%d"), ThreadInstanceIdx++), StackSize, TPri_Normal));
}

FXmppStropheThread::~FXmppStropheThread()
{
	if (ThreadPtr.IsValid())
	{
		// Stop ticking our thread before we exit (and kill the underlying thread)
		ThreadPtr->Kill(true);
	}
}

bool FXmppStropheThread::SendStanza(FStropheStanza&& Stanza)
{
	return StanzaSendQueue.Enqueue(MakeUnique<FStropheStanza>(MoveTemp(Stanza)));
}

bool FXmppStropheThread::Init()
{
	StropheConnection.RegisterStropheHandler(ConnectionManager);
	return true;
}

uint32 FXmppStropheThread::Run()
{
	while (!ExitRequested)
	{
		if (ConnectRequest)
		{
			ConnectRequest = false;

			ConnectionManager.QueueNewLoginStatus(EXmppLoginStatus::ProcessingLogin);
			if (!StropheConnection.Connect(ServerConfiguration.ServerAddr, ServerConfiguration.ServerPort, ConnectionManager))
			{
				ConnectionManager.QueueNewLoginStatus(EXmppLoginStatus::LoggedOut);
			}
		}
		else if (DisconnectRequest)
		{
			DisconnectRequest = false;

			ConnectionManager.QueueNewLoginStatus(EXmppLoginStatus::ProcessingLogout);
			StropheConnection.Disconnect();
			ConnectionManager.QueueNewLoginStatus(EXmppLoginStatus::LoggedOut);
		}

		SendQueuedStanza();

		StropheConnection.XmppThreadTick();
	}

	return 0;
}

void FXmppStropheThread::Stop()
{
	ExitRequested = true;
}

void FXmppStropheThread::Exit()
{
	if (StropheConnection.GetConnectionState() == FStropheConnectionState::Connected)
	{
		StropheConnection.Disconnect();
	}
	StropheConnection.RemoveStropheHandler();
}

void FXmppStropheThread::SendQueuedStanza()
{
	// Send all our queued stanzas
	while (!StanzaSendQueue.IsEmpty())
	{
		TUniquePtr<FStropheStanza> StanzaPtr;
		if (StanzaSendQueue.Dequeue(StanzaPtr))
		{
			// Add a Correlation Id if we haven't set one already
			static const FString CorrelationId(TEXT("corr-id"));
			if (!StanzaPtr->HasAttribute(CorrelationId))
			{
				StanzaPtr->SetAttribute(CorrelationId, FGuid::NewGuid().ToString());
			}

			StropheConnection.SendStanza(*StanzaPtr);
		}
	}
}

#endif
