// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MessageBus.cpp: Implements the FMessageBus class.
=============================================================================*/

#include "MessagingPrivatePCH.h"


/* FMessageBus structors
 *****************************************************************************/

FMessageBus::FMessageBus( const IAuthorizeMessageRecipientsPtr& InRecipientAuthorizer )
	: RecipientAuthorizer(InRecipientAuthorizer)
{
	Router = new FMessageRouter();
	RouterThread = FRunnableThread::Create(Router, TEXT("FMessageBus.Router"), false, false, 128 * 1024, TPri_Normal);

	check(Router != nullptr);
}


FMessageBus::~FMessageBus( )
{
	Shutdown();

	delete Router;
}


/* IMessageBus interface
 *****************************************************************************/

void FMessageBus::Shutdown( )
{
	if (RouterThread != nullptr)
	{
		ShutdownDelegate.Broadcast();

		RouterThread->Kill(true);
		delete RouterThread;
		RouterThread = nullptr;
	}
}
