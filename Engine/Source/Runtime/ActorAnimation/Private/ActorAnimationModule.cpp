// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "ActorAnimationPrivatePCH.h"
#include "ModuleInterface.h"


/**
 * Implements the ActorAnimation module.
 */
class FActorAnimationModule
	: public IModuleInterface
{
public:

	// IModuleInterface interface

	virtual void StartupModule() override { }
	virtual void ShutdownModule() override { }
};


IMPLEMENT_MODULE(FActorAnimationModule, ActorAnimation);
