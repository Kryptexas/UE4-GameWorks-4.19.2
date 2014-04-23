// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemPrivatePCH.h"
#include "OnlineSubsystemImpl.h"
#include "NamedInterfaces.h"

FOnlineSubsystemImpl::FOnlineSubsystemImpl() :
	InstanceName(NAME_None),
	bForceDedicated(false),
	NamedInterfaces(NULL)
{
}

FOnlineSubsystemImpl::FOnlineSubsystemImpl(FName InInstanceName) :
	InstanceName(InInstanceName),
	bForceDedicated(false),
	NamedInterfaces(NULL)
{
}

FOnlineSubsystemImpl::~FOnlineSubsystemImpl()
{
	if (NamedInterfaces)
	{
		NamedInterfaces->RemoveFromRoot();
		NamedInterfaces = NULL;
	}
}

void FOnlineSubsystemImpl::InitNamedInterfaces()
{
	NamedInterfaces = ConstructObject<UNamedInterfaces>(UNamedInterfaces::StaticClass());
	if (NamedInterfaces)
	{
		NamedInterfaces->Initialize();
		NamedInterfaces->AddToRoot();
	}
}

UObject* FOnlineSubsystemImpl::GetNamedInterface(FName InterfaceName)
{
	if (!NamedInterfaces)
	{
		InitNamedInterfaces();
	}

	if (NamedInterfaces)
	{
		return NamedInterfaces->GetNamedInterface(InterfaceName);
	}

	return NULL;
}

void FOnlineSubsystemImpl::SetNamedInterface(FName InterfaceName, UObject* NewInterface)
{
	if (!NamedInterfaces)
	{
		InitNamedInterfaces();
	}

	if (NamedInterfaces)
	{
		return NamedInterfaces->SetNamedInterface(InterfaceName, NewInterface);
	}
}

