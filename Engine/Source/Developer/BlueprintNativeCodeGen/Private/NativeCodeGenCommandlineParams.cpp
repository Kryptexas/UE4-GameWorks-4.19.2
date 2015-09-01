// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "BlueprintNativeCodeGenPCH.h"
#include "NativeCodeGenCommandlineParams.h"

DEFINE_LOG_CATEGORY_STATIC(LogNativeCodeGenCommand, Log, All);

/**  */
const FString FNativeCodeGenCommandlineParams::HelpMessage = TEXT("\n\
\n\
-------------------------------------------------------------------------------\n\
    GenerateBlueprintCodeModule    ::    Converts Blueprint assets into C++    \n\
-------------------------------------------------------------------------------\n\
\n\
::                                                                             \n\
:: Usage                                                                       \n\
::                                                                             \n\
\n\
    UE4Editor.exe <project> -run=GenerateBlueprintCodeModule [parameters]      \n\
\n\
::                                                                             \n\
:: Switches                                                                    \n\
::                                                                             \n\
\n\
    -whitelist=<>       \n\
                        \n\
\n\
    -blacklist=<>       \n\
                        \n\
\n\
    -output=<>          \n\
                        \n\
\n\
    -help, -h, -?       Display this message and then exit.                    \n\
\n");

/*******************************************************************************
 * NativeCodeGenCommandlineParamsImpl
 ******************************************************************************/

namespace NativeCodeGenCommandlineParamsImpl
{

}
 
/*******************************************************************************
 * FNativeCodeGenCommandlineParams
 ******************************************************************************/

//------------------------------------------------------------------------------
FNativeCodeGenCommandlineParams::FNativeCodeGenCommandlineParams(const TArray<FString>& CommandlineSwitches)
	: bHelpRequested(false)
{
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
		else
		{
			//UE_LOG(LogNativeCodeGenCommand, Warning, TEXT("Unrecognized commandline parameter: %s"), *Switch);
		}
	}
}

