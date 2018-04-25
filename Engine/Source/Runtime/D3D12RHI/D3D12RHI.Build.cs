// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class D3D12RHI : ModuleRules
{
	public D3D12RHI(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateIncludePaths.Add("Runtime/D3D12RHI/Private");

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"Engine",
				"RHI",
				"RenderCore",
				"ShaderCore",
				"UtilityShaders",
			    }
			);

		if (Target.Configuration != UnrealTargetConfiguration.Shipping)
		{
			PrivateIncludePathModuleNames.AddRange(new string[] { "TaskGraph" });
		}

		///////////////////////////////////////////////////////////////
        // Platform specific defines
        ///////////////////////////////////////////////////////////////

        if (Target.Platform != UnrealTargetPlatform.Win64 && Target.Platform != UnrealTargetPlatform.XboxOne)
        {
            PrecompileForTargets = PrecompileTargetsType.None;
        }

        if (Target.Platform == UnrealTargetPlatform.Win64 || Target.Platform == UnrealTargetPlatform.Win32)
		{
			AddEngineThirdPartyPrivateStaticDependencies(Target, "DX12");
			AddEngineThirdPartyPrivateStaticDependencies(Target, "DX11");
			AddEngineThirdPartyPrivateStaticDependencies(Target, "NVAPI");
			AddEngineThirdPartyPrivateStaticDependencies(Target, "AMD_AGS");

            // NVCHANGE_BEGIN: Add VXGI
            if ((Target.Platform == UnrealTargetPlatform.Win64) || (Target.Platform == UnrealTargetPlatform.Win32))
            {
                AddEngineThirdPartyPrivateStaticDependencies(Target, "VXGI");
            }
			// NVCHANGE_END: Add VXGI

			// NVCHANGE_BEGIN: Add HBAO+
			if ((Target.Platform == UnrealTargetPlatform.Win64) || (Target.Platform == UnrealTargetPlatform.Win32))
			{
				AddEngineThirdPartyPrivateStaticDependencies(Target, "GFSDK_SSAO");
			}
			// NVCHANGE_END: Add HBAO+
		}
	}
}
