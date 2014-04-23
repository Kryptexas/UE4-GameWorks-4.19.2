// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemPrivatePCH.h"
#include "OnlineSubsystemImpl.h"
#include "NamedInterfaces.h"

FOnlineSubsystemImpl::FOnlineSubsystemImpl() :
	InstanceName(DEFAULT_INSTANCE),
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

bool FOnlineSubsystemImpl::IsServer() const
{
	int32 WorldContextHandle = INDEX_NONE;
	if (InstanceName != DEFAULT_INSTANCE && InstanceName != NAME_None)
	{
		WorldContextHandle = atoi(TCHAR_TO_ANSI(*InstanceName.ToString()));
	}

	return IsServerForOnlineSubsystems(WorldContextHandle);
}

bool FOnlineSubsystemImpl::IsLocalPlayer(const FUniqueNetId& UniqueId) const
{
	if (!IsDedicated())
	{
		IOnlineIdentityPtr IdentityInt = GetIdentityInterface();
		if (IdentityInt.IsValid())
		{
			for (int32 LocalUserNum = 0; LocalUserNum < MAX_LOCAL_PLAYERS; LocalUserNum++)
			{
				TSharedPtr<FUniqueNetId> LocalUniqueId = IdentityInt->GetUniquePlayerId(LocalUserNum);
				if (LocalUniqueId.IsValid() && UniqueId == *LocalUniqueId)
				{
					return true;
				}
			}
		}
	}

	return false;
}

