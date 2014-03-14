// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemUtilsPrivatePCH.h"
#include "Online.h"

ATestBeaconHost::ATestBeaconHost(const FPostConstructInitializeProperties& PCIP) :
	Super(PCIP)
{
}

bool ATestBeaconHost::InitHost()
{
#if !UE_BUILD_SHIPPING
	UE_LOG(LogBeacon, Verbose, TEXT("InitHost"));
	if(AOnlineBeaconHost::InitHost())
	{
		OnBeaconConnected(FName(TEXT("TestBeacon"))).BindUObject(this, &ATestBeaconHost::ClientConnected);
		return true;
	}
#endif
	return false;
}

void ATestBeaconHost::ClientConnected(AOnlineBeaconClient* NewClientActor, UNetConnection* ClientConnection)
{
#if !UE_BUILD_SHIPPING
	UE_LOG(LogBeacon, Verbose, TEXT("ClientConnected %s from (%s)"), 
		NewClientActor ? *NewClientActor->GetName() : TEXT("NULL"), 
		NewClientActor ? *NewClientActor->GetNetConnection()->LowLevelDescribe() : TEXT("NULL"));

	ATestBeaconClient* BeaconClient = Cast<ATestBeaconClient>(NewClientActor);
	if (BeaconClient != NULL)
	{
		BeaconClient->ClientPing();
	}
#endif
}

AOnlineBeaconClient* ATestBeaconHost::SpawnBeaconActor()
{	
#if !UE_BUILD_SHIPPING
	FActorSpawnParameters SpawnInfo;
	ATestBeaconClient* BeaconActor = GetWorld()->SpawnActor<ATestBeaconClient>(ATestBeaconClient::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnInfo);
	if (BeaconActor)
	{
		BeaconActor->SetBeaconOwner(this);
	}

	return BeaconActor;
#else
	return NULL;
#endif
}
