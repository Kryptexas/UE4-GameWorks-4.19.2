// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "ModuleInterface.h"
#include "ModuleManager.h"

class FLiveLinkMessageBusFrameworkModule : public IModuleInterface
{
public:

	virtual bool SupportsDynamicReloading() override
	{
		return false;
	}
};

IMPLEMENT_MODULE(FLiveLinkMessageBusFrameworkModule, LiveLinkMessageBusFramework);