// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class ShaderCompileWorker : ModuleRules
{
	public ShaderCompileWorker(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"Projects",
				"ShaderCore",
				"SandboxFile",
				"TargetPlatform",
				"ApplicationCore"
			});

		if (Target.Platform == UnrealTargetPlatform.Linux)
		{
			PrivateDependencyModuleNames.AddRange(
			new string[] {
				"NetworkFile",
				"PakFile",
				"StreamingFile",
				});
		}

		PrivateIncludePathModuleNames.AddRange(
			new string[] {
				"Launch",
				"TargetPlatform",
			});

		PrivateIncludePaths.Add("Runtime/Launch/Private");      // For LaunchEngineLoop.cpp include

		// Include D3D compiler binaries
		string EngineDir = Path.GetFullPath(Target.RelativeEnginePath);

		if (Target.Platform == UnrealTargetPlatform.Win32)
		{
			RuntimeDependencies.Add(EngineDir + "Binaries/ThirdParty/Windows/DirectX/x86/d3dcompiler_47.dll");
		}
		else if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			RuntimeDependencies.Add(EngineDir + "Binaries/ThirdParty/Windows/DirectX/x64/d3dcompiler_47.dll");
		}
	}
}

