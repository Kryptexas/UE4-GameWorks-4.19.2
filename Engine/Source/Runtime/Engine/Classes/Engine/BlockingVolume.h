// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// BlockingVolume:  a bounding volume
// used to block certain classes of actors
// primary use is to provide collision for non-zero extent traces around static meshes
//=============================================================================

#pragma once
#include "BlockingVolume.generated.h"

UCLASS(MinimalAPI)
class ABlockingVolume : public AVolume
{
	GENERATED_UCLASS_BODY()

	virtual bool UpdateNavigationRelevancy() OVERRIDE;

	// Begin UObject interface.
#if WITH_EDITOR
	virtual void LoadedFromAnotherClass(const FName& OldClassName) OVERRIDE;
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) OVERRIDE;
#endif // WITH_EDITOR	
	// End UObject interface.
};



