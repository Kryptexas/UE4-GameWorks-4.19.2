// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MessagingPrivatePCH.h"


/* FMessageBus structors
 *****************************************************************************/

FMessageBus::FMessageBus( const IAuthorizeMessageRecipientsPtr& InRecipientAuthorizer )
	: RecipientAuthorizer(InRecipientAuthorizer)
{
	Router = new FMessageRouter();
	RouterThread = FRunnableThread::Create(Router, TEXT("FMessageBus.Router"), 128 * 1024, TPri_Normal, FPlatformAffinity::GetPoolThreadMask());

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
