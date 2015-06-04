// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "PortalRpcPrivatePCH.h"
#include "IMessageRpcServer.h"
#include "ModuleManager.h"


/* FPortalRpcProvider structors
 *****************************************************************************/

FPortalRpcProvider::FPortalRpcProvider()
{
	MessageEndpoint = FMessageEndpoint::Builder("FRpcManager")
		.Handling<FPortalRpcLocateServer>(this, &FPortalRpcProvider::HandleMessage);
}


/* FPortalRpcProvider callbacks
 *****************************************************************************/

void FPortalRpcProvider::HandleMessage(const FPortalRpcLocateServer& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context)
{
	if (!LookupDelegate.IsBound())
	{
		return;
	}

	const FString ProductKey = Message.ProductId.ToString() + Message.ProductVersion;
	TSharedPtr<IMessageRpcServer> Server = Servers.FindRef(ProductKey);

	if (!Server.IsValid())
	{
		Server = LookupDelegate.Execute(ProductKey);
	}

	if (Server.IsValid())
	{
		MessageEndpoint->Send(new FPortalRpcServer(Server->GetAddress().ToString()), Context->GetSender());
	}
}
