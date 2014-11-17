// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "GameFramework/CameraBlockingVolume.h"

ACameraBlockingVolume::ACameraBlockingVolume(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	BrushComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);
}