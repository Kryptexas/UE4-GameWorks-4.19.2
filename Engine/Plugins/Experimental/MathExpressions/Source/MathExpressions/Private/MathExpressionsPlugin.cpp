// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MathExpressionsPrivatePCH.h"
#include "MathExpressions.generated.inl"


class FMathExpressionsPlugin : public IMathExpressionsPlugin
{
public:
	// IModuleInterface implementation
	virtual void StartupModule() OVERRIDE;
	virtual void ShutdownModule() OVERRIDE;
	// End of IModuleInterface implementation
};

IMPLEMENT_MODULE( FMathExpressionsPlugin, MathExpressions )



void FMathExpressionsPlugin::StartupModule()
{
	// This code will execute after your module is loaded into memory (but after global variables are initialized, of course.)
}


void FMathExpressionsPlugin::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}



