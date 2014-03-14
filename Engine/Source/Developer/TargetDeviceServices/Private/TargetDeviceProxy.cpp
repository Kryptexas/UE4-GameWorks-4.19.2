// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	TargetDeviceProxy.cpp: Implements the FTargetDeviceProxy class.
=============================================================================*/

#include "TargetDeviceServicesPrivatePCH.h"


/* FTargetDeviceProxy structors
 *****************************************************************************/

FTargetDeviceProxy::FTargetDeviceProxy( const FString& InId )
	: Connected(false)
	, Id(InId)
	, LastUpdateTime(0)
	, SupportsMultiLaunch(false)
	, SupportsPowerOff(false)
	, SupportsPowerOn(false)
	, SupportsReboot(false)
{
	InitializeMessaging();
}


FTargetDeviceProxy::FTargetDeviceProxy( const FString& InId, const FTargetDeviceServicePong& Message, const IMessageContextRef& Context )
	: Connected(false)
	, Id(InId)
	, SupportsMultiLaunch(false)
	, SupportsPowerOff(false)
	, SupportsPowerOn(false)
	, SupportsReboot(false)
{
	InitializeMessaging();
	UpdateFromMessage(Message, Context);
}


/* FTargetDeviceProxy interface
*****************************************************************************/

void FTargetDeviceProxy::UpdateFromMessage( const FTargetDeviceServicePong& Message, const IMessageContextRef& Context )
{
	if (Message.DeviceID == Id)
	{
		MessageAddress = Context->GetSender();

		Connected = Message.Connected;
		HostName = Message.HostName;
		HostUser = Message.HostUser;
		Id = Message.DeviceID;
		Make = Message.Make;
		Model = Message.Model;
		Name = Message.Name;
		DeviceUser = Message.DeviceUser;
		DeviceUserPassword = Message.DeviceUserPassword;
		Platform = Message.PlatformName;
		Shared = Message.Shared;
		SupportsMultiLaunch = Message.SupportsMultiLaunch;
		SupportsPowerOff = Message.SupportsPowerOff;
		SupportsPowerOn = Message.SupportsPowerOn;
		SupportsReboot = Message.SupportsReboot;

		LastUpdateTime = FDateTime::UtcNow();
	}
}


/* ITargetDeviceProxyPtr interface
 *****************************************************************************/

bool FTargetDeviceProxy::DeployApp( const TMap<FString, FString>& Files, const FGuid& TransactionId )
{
	for (TMap<FString, FString>::TConstIterator It(Files); It; ++It)
	{
		IMessageAttachmentRef FileAttachment = MakeShareable(new FFileMessageAttachment(It.Key()));
		FString SourcePath = It.Key();

		MessageEndpoint->Send(new FTargetDeviceServiceDeployFile(It.Value(), TransactionId), FileAttachment, MessageAddress);
	}

	MessageEndpoint->Send(new FTargetDeviceServiceDeployCommit(TransactionId), MessageAddress);

	return true;
}


bool FTargetDeviceProxy::LaunchApp( const FString& AppId, EBuildConfigurations::Type BuildConfiguration, const FString& Params )
{
	MessageEndpoint->Send(new FTargetDeviceServiceLaunchApp(AppId, BuildConfiguration, Params), MessageAddress);

	return true;
}


void FTargetDeviceProxy::PowerOff( bool Force )
{
	MessageEndpoint->Send(new FTargetDeviceServicePowerOff(FPlatformProcess::UserName(false), Force), MessageAddress);
}


void FTargetDeviceProxy::PowerOn( )
{
	MessageEndpoint->Send(new FTargetDeviceServicePowerOn(FPlatformProcess::UserName(false)), MessageAddress);
}


void FTargetDeviceProxy::Reboot( )
{
	MessageEndpoint->Send(new FTargetDeviceServiceReboot(FPlatformProcess::UserName(false)), MessageAddress);
}


void FTargetDeviceProxy::Run( const FString& ExecutablePath, const FString& Params )
{
	MessageEndpoint->Send(new FTargetDeviceServiceRunExecutable(ExecutablePath, Params), MessageAddress);
}


/* FTargetDeviceProxy implementation
 *****************************************************************************/

void FTargetDeviceProxy::InitializeMessaging( )
{
	MessageEndpoint = FMessageEndpoint::Builder(FName(*FString::Printf(TEXT("FTargetDeviceProxy (%s)"), *Id)))
		.Handling<FTargetDeviceServiceDeployFinished>(this, &FTargetDeviceProxy::HandleDeployFinishedMessage)
		.Handling<FTargetDeviceServiceLaunchFinished>(this, &FTargetDeviceProxy::HandleLaunchFinishedMessage);
}


/* FTargetDeviceProxy event handlers
 *****************************************************************************/

void FTargetDeviceProxy::HandleDeployFinishedMessage( const FTargetDeviceServiceDeployFinished& Message, const IMessageContextRef& Context )
{
	if (Message.Succeeded)
	{
		DeployCommittedDelegate.Broadcast(Message.TransactionId, Message.AppID);
	}
	else
	{
		DeployFailedDelegate.Broadcast(Message.TransactionId);
	}
}


void FTargetDeviceProxy::HandleLaunchFinishedMessage( const FTargetDeviceServiceLaunchFinished& Message, const IMessageContextRef& Context )
{
	if (Message.Succeeded)
	{
		LaunchSucceededDelegate.Broadcast(Message.AppID, Message.ProcessId);
	}
	else
	{
		LaunchFailedDelegate.Broadcast(Message.AppID);
	}
}
