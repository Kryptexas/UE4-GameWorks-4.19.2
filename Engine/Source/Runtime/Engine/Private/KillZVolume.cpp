// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

AKillZVolume::AKillZVolume(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void AKillZVolume::ActorEnteredVolume(AActor* Other)
{
	Super::ActorEnteredVolume(Other);
	
	if ( Other )
	{
		const UDamageType* DamageType = GetDefault<UDamageType>();

		UWorld* World = GetWorld();
		if ( World )
		{
			AWorldSettings* WorldSettings = World->GetWorldSettings( true );
			if ( WorldSettings && WorldSettings->KillZDamageType )
			{
				DamageType = WorldSettings->KillZDamageType->GetDefaultObject<UDamageType>();
			}
		}

		Other->FellOutOfWorld(*DamageType);
	}
}