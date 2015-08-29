// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "GameFramework/DefaultPhysicsVolume.h"
#include "PhysicsEngine/PhysicsSettings.h"

ADefaultPhysicsVolume::ADefaultPhysicsVolume(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

#if WITH_EDITORONLY_DATA
	// Not allowed to be selected or edited within Unreal Editor
	bEditable = false;
#endif // WITH_EDITORONLY_DATA

	// update default values when world is restarted
	TerminalVelocity = UPhysicsSettings::Get()->DefaultTerminalVelocity;
	FluidFriction = UPhysicsSettings::Get()->DefaultFluidFriction;
}