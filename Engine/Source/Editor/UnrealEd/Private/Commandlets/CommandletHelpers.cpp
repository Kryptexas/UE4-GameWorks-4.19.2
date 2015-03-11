// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "Commandlets/CommandletHelpers.h"

namespace CommandletHelpers
{
	FString BuildCommandletProcessArguments(const TCHAR* const CommandletName, const TCHAR* const ProjectPath, const TCHAR* const AdditionalArguments)
	{
		check(CommandletName && FCString::Strlen(CommandletName) != 0);

		FString Arguments;
		const FString RunCommand = FString::Printf( TEXT("-run=%s"), CommandletName );
		if (ProjectPath && FCString::Strlen(ProjectPath) != 0)
		{
			Arguments = FString::Printf( TEXT("%s %s"), ProjectPath, *RunCommand );
		}
		if (AdditionalArguments && FCString::Strlen(AdditionalArguments) != 0)
		{
			Arguments = FString::Printf( TEXT("%s %s"), *Arguments, AdditionalArguments );
		}

		return Arguments;
	}
}