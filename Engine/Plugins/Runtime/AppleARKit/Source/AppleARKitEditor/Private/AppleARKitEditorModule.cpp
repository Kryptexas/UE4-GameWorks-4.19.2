// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

// UE4
#include "ModuleManager.h"
#include "Features/IModularFeature.h"

class FAppleARKitEditorModule : public IModuleInterface, public IModularFeature
{
	virtual void StartupModule() override
	{
		IModuleInterface::StartupModule();

		FModuleManager::LoadModulePtr<IModuleInterface>("AugmentedReality");
	}
};

IMPLEMENT_MODULE(FAppleARKitEditorModule, AppleARKitEditor);
