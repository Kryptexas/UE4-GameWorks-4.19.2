// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "BlueprintNativeCodeGenPCH.h"
#include "GenerateBlueprintCodeModuleCommandlet.h"
#include "NativeCodeGenCommandlineParams.h"
#include "BlueprintNativeCodeGenCoordinator.h"
#include "BlueprintNativeCodeGenUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogBlueprintCodeGen, Log, All);

/*******************************************************************************
 * GenerateBlueprintCodeModuleImpl
*******************************************************************************/

namespace GenerateBlueprintCodeModuleImpl
{

}

/*******************************************************************************
 * UGenerateBlueprintCodeModuleCommandlet
*******************************************************************************/

//------------------------------------------------------------------------------
UGenerateBlueprintCodeModuleCommandlet::UGenerateBlueprintCodeModuleCommandlet(FObjectInitializer const& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

//------------------------------------------------------------------------------
int32 UGenerateBlueprintCodeModuleCommandlet::Main(FString const& Params)
{
	TArray<FString> Tokens, Switches;
	ParseCommandLine(*Params, Tokens, Switches);

	FNativeCodeGenCommandlineParams CommandlineParams(Switches);

	if (CommandlineParams.bHelpRequested)
	{
		UE_LOG(LogBlueprintCodeGen, Display, TEXT("%s"), *FNativeCodeGenCommandlineParams::HelpMessage);
		return 0;
	}

	FBlueprintNativeCodeGenUtils::FScopedFeedbackContext ScopedErrorTracker;
	FBlueprintNativeCodeGenCoordinator Coordinator(CommandlineParams);

	if (!ScopedErrorTracker.HasErrors())
	{
		FBlueprintNativeCodeGenUtils::GenerateCodeModule(Coordinator);
	}
	return ScopedErrorTracker.HasErrors();
}

