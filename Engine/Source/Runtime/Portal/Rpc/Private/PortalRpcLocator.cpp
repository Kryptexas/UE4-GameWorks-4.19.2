// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "PortalRpcPrivatePCH.h"
#include "EngineVersion.h"
#include "PortalRpcLocator.h"


/* FPortalRpcLocator structors
 *****************************************************************************/

FPortalRpcLocator::FPortalRpcLocator()
	: LastServerResponse(FDateTime::MinValue())
{
	MessageEndpoint = FMessageEndpoint::Builder("FPortalRpcLocator")
		.Handling<FPortalRpcServer>(this, &FPortalRpcLocator::HandleMessage);

	TickerHandle = FTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateRaw(this, &FPortalRpcLocator::HandleTicker), PORTAL_RPC_LOCATE_INTERVAL);
}


FPortalRpcLocator::~FPortalRpcLocator()
{
	FTicker::GetCoreTicker().RemoveTicker(TickerHandle);
}


/* IPortalRpcLocator interface
 *****************************************************************************/

const FMessageAddress& FPortalRpcLocator::GetServerAddress() const
{
	return ServerAddress;
}


FSimpleDelegate& FPortalRpcLocator::OnServerLocated()
{
	return ServerLocatedDelegate;
}


FSimpleDelegate& FPortalRpcLocator::OnServerLost()
{
	return ServerLostDelegate;
}


/* FPortalRpcLocator callbacks
 *****************************************************************************/

void FPortalRpcLocator::HandleMessage(const FPortalRpcServer& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context)
{
	LastServerResponse = FDateTime::UtcNow();

	FMessageAddress NewServerAddress;

	if (FMessageAddress::Parse(Message.ServerAddress, NewServerAddress) && (NewServerAddress != ServerAddress))
	{
		ServerAddress = NewServerAddress;
		ServerLocatedDelegate.ExecuteIfBound();
	}
}


bool FPortalRpcLocator::HandleTicker(float DeltaTime)
{
	if (ServerAddress.IsValid() && (FDateTime::UtcNow() - LastServerResponse > PORTAL_RPC_LOCATE_TIMEOUT))
	{
		ServerAddress.Invalidate();
		ServerLostDelegate.ExecuteIfBound();
	}

	// @todo sarge: implement actual product GUID
	MessageEndpoint->Publish(new FPortalRpcLocateServer(FGuid(), GEngineVersion.ToString()), EMessageScope::Network);

	return true;
}
