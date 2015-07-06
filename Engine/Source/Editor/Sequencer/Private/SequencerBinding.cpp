// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SequencerPrivatePCH.h"
#include "SequencerBinding.h"


/* FSequencerBinding interface
 *****************************************************************************/

UObject* FSequencerBinding::GetBoundObject() const
{
	// get component-less object
	if (ComponentName.IsEmpty())
	{
		return Object;
	}

	// get cached component
	if ((CachedComponent != nullptr) && (CachedComponent->GetName() == ComponentName))
	{
		return CachedComponent;
	}

	CachedComponent = nullptr;

	// find & cache component
	AActor* Actor = Cast<AActor>(Object);

	if (Actor == nullptr)
	{
		return Object;
	}

	for (UActorComponent* ActorComponent : Actor->GetComponents())
	{
		if (ActorComponent->GetName() == ComponentName)
		{
			CachedComponent = ActorComponent;

			return ActorComponent;
		}
	}

	// component not found
	return nullptr;
}
