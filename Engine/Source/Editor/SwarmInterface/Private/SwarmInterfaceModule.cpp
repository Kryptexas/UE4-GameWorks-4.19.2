// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CoreUObject.h"

#include "ModuleManager.h"

#include "SwarmMessages.h"

class FSwarmInterfaceModule : public IModuleInterface
{

public:

	virtual void StartupModule() override {}

	virtual void ShutdownModule() override {}
};

// Dummy class initialization
USwarmMessages::USwarmMessages( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
{ }

IMPLEMENT_MODULE( FSwarmInterfaceModule, SwarmInterface );
