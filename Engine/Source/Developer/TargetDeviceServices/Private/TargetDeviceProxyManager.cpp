// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	TargetDeviceProxyManager.cpp: Implements the FTargetDeviceProxyManager class.
=============================================================================*/

#include "TargetDeviceServicesPrivatePCH.h"


/* FTargetDeviceProxyManager structors
 *****************************************************************************/

FTargetDeviceProxyManager::FTargetDeviceProxyManager( )
{
	MessageEndpoint = FMessageEndpoint::Builder("FTargetDeviceProxyManager")
		.Handling<FTargetDeviceServicePong>(this, &FTargetDeviceProxyManager::HandlePongMessage);

	if (MessageEndpoint.IsValid())
	{
		TickDelegate = FTickerDelegate::CreateRaw(this, &FTargetDeviceProxyManager::HandleTicker);
		FTicker::GetCoreTicker().AddTicker(TickDelegate, TARGET_DEVICE_SERVICES_PING_INTERVAL);

		SendPing();
	}
}


FTargetDeviceProxyManager::~FTargetDeviceProxyManager( )
{
	FTicker::GetCoreTicker().RemoveTicker(TickDelegate);
}


/* ITargetDeviceProxyLocator interface
 *****************************************************************************/

ITargetDeviceProxyPtr FTargetDeviceProxyManager::FindProxy( const FString& DeviceId ) 
{
	return Proxies.FindRef(DeviceId);
}


ITargetDeviceProxyRef FTargetDeviceProxyManager::FindOrAddProxy( const FString& DeviceId )
{
	TSharedPtr<FTargetDeviceProxy>& Proxy = Proxies.FindOrAdd(DeviceId);

	if (!Proxy.IsValid())
	{
		Proxy = MakeShareable(new FTargetDeviceProxy(DeviceId));

		ProxyAddedDelegate.Broadcast(Proxy.ToSharedRef());
	}

	return Proxy.ToSharedRef();
}


void FTargetDeviceProxyManager::GetProxies( const FString& PlatformName, bool IncludeUnshared, TArray<ITargetDeviceProxyPtr>& OutProxies )
{
	OutProxies.Reset();

	for (TMap<FString, TSharedPtr<FTargetDeviceProxy> >::TConstIterator It(Proxies); It; ++It)
	{
		const TSharedPtr<FTargetDeviceProxy>& Proxy = It.Value();

		if ((IncludeUnshared || Proxy->IsShared()) || (Proxy->GetHostUser() == FPlatformProcess::UserName(false)))
		{
			if (PlatformName.IsEmpty() || (Proxy->GetPlatformName() == PlatformName))
			{
				OutProxies.Add(Proxy);
			}			
		}
	}
}


/* FTargetDeviceProxyManager implementation
 *****************************************************************************/

void FTargetDeviceProxyManager::RemoveDeadProxies( )
{
	FDateTime CurrentTime = FDateTime::UtcNow();

	for (auto ProxyIter = Proxies.CreateIterator(); ProxyIter; ++ProxyIter)
	{
		if (ProxyIter.Value()->GetLastUpdateTime() + FTimespan::FromSeconds(3.0 * TARGET_DEVICE_SERVICES_PING_INTERVAL) < CurrentTime)
		{
			ITargetDeviceProxyPtr RemovedProxy = ProxyIter.Value();
			ProxyIter.RemoveCurrent();
			ProxyRemovedDelegate.Broadcast(RemovedProxy.ToSharedRef());
		}
	}
}


void FTargetDeviceProxyManager::SendPing( )
{
	if (MessageEndpoint.IsValid())
	{
		MessageEndpoint->Publish(new FTargetDeviceServicePing(FPlatformProcess::UserName(false)), EMessageScope::Network);
	}
}


/* FTargetDeviceProxyManager event handlers
 *****************************************************************************/

void FTargetDeviceProxyManager::HandlePongMessage( const FTargetDeviceServicePong& Message, const IMessageContextRef& Context )
{
	TSharedPtr<FTargetDeviceProxy>& Proxy = Proxies.FindOrAdd(Message.DeviceID);

	if (!Proxy.IsValid())
	{
		Proxy = MakeShareable(new FTargetDeviceProxy(Message.DeviceID, Message, Context));
		ProxyAddedDelegate.Broadcast(Proxy.ToSharedRef());
	}
	else
	{
		Proxy->UpdateFromMessage(Message, Context);
	}
}


bool FTargetDeviceProxyManager::HandleTicker( float DeltaTime )
{
	RemoveDeadProxies();
	SendPing();

	return true;
}
