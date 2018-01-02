// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "CLionSourceCodeAccessModule.h"
#include "Modules/ModuleManager.h"
#include "Features/IModularFeatures.h"

#define LOCTEXT_NAMESPACE "CLionSourceCodeAccessor"

IMPLEMENT_MODULE(FCLionSourceCodeAccessModule, CLionSourceCodeAccess);

void FCLionSourceCodeAccessModule::ShutdownModule()
{
	// Unbind provider from editor
	IModularFeatures::Get().UnregisterModularFeature(TEXT("SourceCodeAccessor"), &CLionSourceCodeAccessor);
}

void FCLionSourceCodeAccessModule::StartupModule()
{
	// Quick forced check of availability before anyone touches the module
	CLionSourceCodeAccessor.RefreshAvailability();

	// Bind our source control provider to the editor
	IModularFeatures::Get().RegisterModularFeature(TEXT("SourceCodeAccessor"), &CLionSourceCodeAccessor);
}

bool FCLionSourceCodeAccessModule::SupportsDynamicReloading()
{
	return true;
}

FCLionSourceCodeAccessor& FCLionSourceCodeAccessModule::GetAccessor()
{
	return CLionSourceCodeAccessor;
}

#undef LOCTEXT_NAMESPACE
