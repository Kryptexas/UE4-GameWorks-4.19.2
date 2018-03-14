// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "MediaPlane.h"
#include "MediaPlaneComponent.h"


/* AMediaPlane structors
 *****************************************************************************/

AMediaPlane::AMediaPlane(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	MediaPlane = ObjectInitializer.CreateDefaultSubobject<UMediaPlaneComponent>(this, "MediaPlaneComponent");
	RootComponent = MediaPlane;
}