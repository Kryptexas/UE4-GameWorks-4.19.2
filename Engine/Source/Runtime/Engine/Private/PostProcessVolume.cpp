// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

APostProcessVolume::APostProcessVolume(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	BrushComponent->BodyInstance.bEnableCollision_DEPRECATED = false;
	BrushComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	// post process volume needs physics data for trace
	BrushComponent->bAlwaysCreatePhysicsState = true;
	BrushComponent->Mobility = EComponentMobility::Movable;
	
	bEnabled = true;
	BlendRadius = 100.0f;
	BlendWeight = 1.0f;
}

#if WITH_EDITOR
void APostProcessVolume::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	static const FName NAME_Blendables = FName(TEXT("Blendables"));
	
	if(PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == NAME_Blendables)
	{
		// remove unsupported types
		{
			uint32 Count = Settings.Blendables.Num();
			for(uint32 i = 0; i < Count; ++i)
			{
				UObject* Obj = Settings.Blendables[i];

				if(!InterfaceCast<IBlendableInterface>(Obj))
				{
					Settings.Blendables[i] = 0;
				}
			}
		}
	}
}
#endif // WITH_EDITOR