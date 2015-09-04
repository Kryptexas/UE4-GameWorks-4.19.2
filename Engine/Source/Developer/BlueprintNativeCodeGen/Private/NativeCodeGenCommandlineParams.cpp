// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "BlueprintNativeCodeGenPCH.h"
#include "NativeCodeGenCommandlineParams.h"

DEFINE_LOG_CATEGORY_STATIC(LogNativeCodeGenCommandline, Log, All);

/**  */
const FString FNativeCodeGenCommandlineParams::HelpMessage = TEXT("\n\
\n\
-------------------------------------------------------------------------------\n\
::  GenerateBlueprintCodeModule    ::    Converts Blueprint assets into C++    \n\
-------------------------------------------------------------------------------\n\
\n\
::                                                                             \n\
:: Usage                                                                       \n\
::                                                                             \n\
\n\
    UE4Editor.exe <project> -run=GenerateBlueprintCodeModule [parameters]      \n\
\n\
::                                                                             \n\
:: Parameters                                                                  \n\
::                                                                             \n\
\n\
    -whitelist=<Assets> Identifies assets that you wish to convert. <Assets>   \n\
                        can be a comma seperated list, specifying multiple     \n\
                        packages (either directories, or explicit assets) in   \n\
                        the form: /Game/MyContentDir,/Engine/EngineAssetName.  \n\
\n\
    -blacklist=<Assets> Explicitly specifies assets that you don't want        \n\
                        converted (listed in the same manner as -whitelist).   \n\
                        This takes priority over the whitelist (even if it     \n\
                        results in uncompilable code).                         \n\
\n\
    -output=<ModuleDir> Specifies the path where you want converted assets     \n\
                        saved to. If left unset, this will default to the      \n\
                        project's intermeadiate folder (within a sub-folder of \n\
                        its own). If specified as a relative path, it will be  \n\
                        relative to the target project's root directory.       \n\
\n\
    -help, -h, -?       Display this message and then exit.                    \n\
\n");

/*******************************************************************************
 * NativeCodeGenCommandlineParamsImpl
 ******************************************************************************/

namespace NativeCodeGenCommandlineParamsImpl
{
	static const FString DefaultModuleName(TEXT("GeneratedBlueprintCode"));
	static const FString ParamListDelim(TEXT(","));
}
 
/*******************************************************************************
 * FNativeCodeGenCommandlineParams
 ******************************************************************************/

//------------------------------------------------------------------------------
FNativeCodeGenCommandlineParams::FNativeCodeGenCommandlineParams(const TArray<FString>& CommandlineSwitches)
	: bHelpRequested(false)
{
	IFileManager& FileManager = IFileManager::Get();
	ModuleOutputDir = FPaths::Combine(*FPaths::GameIntermediateDir(), *NativeCodeGenCommandlineParamsImpl::DefaultModuleName);

	for (const FString& Param : CommandlineSwitches)
	{
		FString Switch = Param, Value;
		Param.Split(TEXT("="), &Switch, &Value);

		if (!Switch.Compare(TEXT("h"), ESearchCase::IgnoreCase) ||
			!Switch.Compare(TEXT("?"), ESearchCase::IgnoreCase) ||
			!Switch.Compare(TEXT("help"), ESearchCase::IgnoreCase))
		{
			bHelpRequested = true;
		}
		else if (!Switch.Compare(TEXT("whitelist"), ESearchCase::IgnoreCase)) 
		{
			Value.ParseIntoArray(WhiteListedAssetPaths, *NativeCodeGenCommandlineParamsImpl::ParamListDelim);
		}
		else if (!Switch.Compare(TEXT("blacklist"), ESearchCase::IgnoreCase))
		{
			Value.ParseIntoArray(BlackListedAssetPaths, *NativeCodeGenCommandlineParamsImpl::ParamListDelim);
		}
		else if (!Switch.Compare(TEXT("output"), ESearchCase::IgnoreCase))
		{
			// if it's a relative path, let's have it relative to the game 
			// directory (not the UE executable)
			if (FPaths::IsRelative(*Value))
			{
				Value = FPaths::Combine(*FPaths::GameDir(), *Value);
			}

			if (FileManager.DirectoryExists(*Value) || FileManager.MakeDirectory(*Value))
			{
				ModuleOutputDir = Value;
			}
			else
			{
				UE_LOG(LogNativeCodeGenCommandline, Warning, TEXT("'%s' doesn't appear to be a usable output directory, defaulting to: '%s'"), *Value, *ModuleOutputDir);
			}
		}
		else
		{
			//UE_LOG(LogNativeCodeGenCommandline, Warning, TEXT("Unrecognized commandline parameter: %s"), *Switch);
		}
	}
}

