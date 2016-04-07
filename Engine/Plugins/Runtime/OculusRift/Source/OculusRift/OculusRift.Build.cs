// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class OculusRift : ModuleRules
	{
		public OculusRift(TargetInfo Target)
		{
			PrivateIncludePaths.AddRange(
				new string[] {
					"OculusRift/Private",
 					"../../../../Source/Runtime/Renderer/Private",
 					"../../../../Source/ThirdParty/Oculus/Common",
					// ... add other private include paths required here ...
				});

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
					"Engine",
					"InputCore",
					"RHI",
					"RenderCore",
					"Renderer",
					"ShaderCore",
					"HeadMountedDisplay",
					"Slate",
					"SlateCore",
				});

			if (UEBuildConfiguration.bBuildEditor == true)
			{
				PrivateDependencyModuleNames.Add("UnrealEd");
			}

            // Currently, the Rift is only supported on windows and mac platforms
            if (Target.Platform == UnrealTargetPlatform.Win32 || Target.Platform == UnrealTargetPlatform.Win64)
            {
				PrivateDependencyModuleNames.AddRange(
					new string[] 
					{ 
						"LibOVR", 
						"D3D11RHI",
						"D3D12RHI",
						"OpenGLDrv",
					});

                PrivateIncludePaths.AddRange(
                    new string[] 
                    {
				        "OculusRift/Private",
				        "../../../../Source/Runtime/Windows/D3D11RHI/Private",
				        "../../../../Source/Runtime/Windows/D3D11RHI/Private/Windows",
				        "../../../../Source/Runtime/Windows/D3D12RHI/Private",
				        "../../../../Source/Runtime/Windows/D3D12RHI/Private/Windows",
			        	"../../../../Source/Runtime/OpenGLDrv/Private",
						// ... add other private include paths required here ...
				    });

				AddThirdPartyPrivateStaticDependencies(Target, "DX11");
				AddThirdPartyPrivateStaticDependencies(Target, "DX12");
				AddThirdPartyPrivateStaticDependencies(Target, "OpenGL");
				AddThirdPartyPrivateStaticDependencies(Target, "NVAPI");
				AddThirdPartyPrivateStaticDependencies(Target, "DX11Audio");
				AddThirdPartyPrivateStaticDependencies(Target, "DirectSound");
				
				// Disable Unity, where all source files are merged into a single CPP, because
				// there are symbols declared in D3D11Util.h and D3D12Util.h which conflict if
				// both headers are included in the same CPP.
				bFasterWithoutUnity = true;
            }
		}
	}
}