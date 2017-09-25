// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "IFlexModule.h"

#include "FlexPluginBridge.h"
#include "FlexFluidSurfaceRendering.h"

class FFlexModule : public IFlexModule
{
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

IMPLEMENT_MODULE( FFlexModule, Flex )



void FFlexModule::StartupModule()
{
	GFlexPluginBridge = new FFlexPluginBridge();

	GFlexFluidSurfaceRenderer = new FFlexFluidSurfaceRenderer();
}


void FFlexModule::ShutdownModule()
{
	delete GFlexFluidSurfaceRenderer;
	GFlexFluidSurfaceRenderer = nullptr;

	delete GFlexPluginBridge;
	GFlexPluginBridge = nullptr;
}
