// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "BlueprintNativeCodeGenPCH.h"
#include "NativeCodeGenCommandlineParams.h"
#include "BlueprintNativeCodeGenManifest.h"

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
    -wipe               Will clear out the target directory when specified.    \n\
                        Normally, without this switch, if the target directory \n\
                        contains source from a previous run, it builds atop the\n\
                        existing module and appends to its existing manifest.  \n\
\n\
    -manifest=<Path>    Specifies where to save the conversion's manifest file.\n\
                        If <Path> is not an existing directory or file, then it\n\
                        is assumed that it is a file path. If a manifest       \n\
                        already exists here, then we can use this in place of  \n\
                        the -output command (and the target module directory   \n\
                        will be determined by the old manifest file). If left  \n\
                        unset, then it will default to the module's target     \n\
                        directory.                                             \n\
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
	, bWipeRequested(false)
{
	IFileManager& FileManager = IFileManager::Get();
	const FString DefaultModulePath = FPaths::Combine(*FPaths::GameIntermediateDir(), *NativeCodeGenCommandlineParamsImpl::DefaultModuleName);

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
			FPaths::NormalizeDirectoryName(Value);
			// if it's a relative path, let's have it relative to the game 
			// directory (not the UE executable)
			if (FPaths::IsRelative(Value))
			{
				Value = FPaths::Combine(*FPaths::GameDir(), *Value);
			}

			if (FileManager.DirectoryExists(*Value) || FileManager.MakeDirectory(*Value))
			{
				ModuleOutputDir = Value;
			}
			else
			{
				UE_LOG(LogNativeCodeGenCommandline, Warning, TEXT("'%s' doesn't appear to be a valid output directory, defaulting to: '%s'"), *Value, *DefaultModulePath);
			}
		}
		else if (!Switch.Compare(TEXT("wipe"), ESearchCase::IgnoreCase))
		{
			bWipeRequested = true;
		}
		else if (!Switch.Compare(TEXT("manifest"), ESearchCase::IgnoreCase))
		{
			FPaths::NormalizeDirectoryName(Value);

			if (FileManager.FileExists(*Value))
			{
				ManifestFilePath = Value;
			}
			else if (FileManager.DirectoryExists(*Value))
			{
				ManifestFilePath = FPaths::Combine(*Value, *FBlueprintNativeCodeGenManifest::GetDefaultFilename());
			}
			else
			{
				UE_LOG(LogNativeCodeGenCommandline, Display, TEXT("Unsure if '%s' is supposed to denote a directory or a filename, assuming it's a file."), *Value);

				FString FilePath = FPaths::GetPath(*Value);
				if (FileManager.DirectoryExists(*FilePath) || FileManager.MakeDirectory(*FilePath))
				{
					ManifestFilePath = Value;
				}
				else
				{
					if (FPaths::IsRelative(Value))
					{
						Value = FPaths::ConvertRelativePathToFull(Value);
					}
					UE_LOG( LogNativeCodeGenCommandline, Warning, TEXT("'%s' doesn't appear to be a valid manifest file path, defaulting to: '%s'."), *Value,
						*FPaths::Combine(*DefaultModulePath, *FBlueprintNativeCodeGenManifest::GetDefaultFilename()) );
				}
			}
		}
		else
		{
			//UE_LOG(LogNativeCodeGenCommandline, Warning, TEXT("Unrecognized commandline parameter: %s"), *Switch);
		}
	}

	bool const bUtilizeExistingManifest = !ManifestFilePath.IsEmpty() && !bWipeRequested && FileManager.FileExists(*ManifestFilePath);
	// an existing manifest would specify where to put the module
	if (ModuleOutputDir.IsEmpty() && !bUtilizeExistingManifest)
	{
		ModuleOutputDir = DefaultModulePath;
	}
}

