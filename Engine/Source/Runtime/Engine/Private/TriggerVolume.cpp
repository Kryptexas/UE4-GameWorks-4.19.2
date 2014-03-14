// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

ATriggerVolume::ATriggerVolume(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	static FName CollisionProfileName(TEXT("Trigger"));
	BrushComponent->SetCollisionProfileName(CollisionProfileName);

	bColored = true;
	BrushColor.R = 100;
	BrushColor.G = 255;
	BrushColor.B = 100;
	BrushColor.A = 255;

}

#if WITH_EDITOR

void ATriggerVolume::LoadedFromAnotherClass(const FName& OldClassName)
{
	Super::LoadedFromAnotherClass(OldClassName);

	if(GetLinkerUE4Version() < VER_UE4_REMOVE_DYNAMIC_VOLUME_CLASSES)
	{
		static FName DynamicTriggerVolume_NAME(TEXT("DynamicTriggerVolume"));

		if(OldClassName == DynamicTriggerVolume_NAME)
		{
			BrushComponent->Mobility = EComponentMobility::Movable;
		}
	}
}

#endif