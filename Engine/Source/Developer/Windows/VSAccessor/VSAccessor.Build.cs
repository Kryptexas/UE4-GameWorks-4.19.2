// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class VSAccessor : ModuleRules
{
	public VSAccessor( TargetInfo Target )
	{
        PrivateDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject" });

		if (WindowsPlatform.bHasVisualStudioDTE)
		{
			// This module requires atlbase.h to be included before Windows headers, so we can make use of shared PCHs.  This
			// module will always have its own private PCH generated, if necessary.
			PCHUsage = PCHUsageMode.NoSharedPCHs;
			Definitions.Add("WITH_VSEXPRESS=0");
		}
		else
		{
			Definitions.Add("WITH_VSEXPRESS=1");
		}
	}
}
