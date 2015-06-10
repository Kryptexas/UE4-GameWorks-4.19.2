// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphRuntimePrivatePCH.h"
//#include "Paper2DModule.h"

//////////////////////////////////////////////////////////////////////////
// FAnimGraphRuntimeModule

class FAnimGraphRuntimeModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
	}

	virtual void ShutdownModule() override
	{
	}
};

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_MODULE(FAnimGraphRuntimeModule, AnimGraphRuntime);
