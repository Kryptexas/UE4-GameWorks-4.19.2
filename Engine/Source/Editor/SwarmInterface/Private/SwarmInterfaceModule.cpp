// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CoreUObject.h"

#include "ModuleManager.h"

#include "SwarmInterface.generated.inl"

class FSwarmInterfaceModule : public IModuleInterface
{

public:

	virtual void StartupModule() OVERRIDE {}

	virtual void ShutdownModule() OVERRIDE {}
};

// Dummy class initialization
USwarmMessages::USwarmMessages( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
{ }

IMPLEMENT_MODULE( FSwarmInterfaceModule, SwarmInterface );
