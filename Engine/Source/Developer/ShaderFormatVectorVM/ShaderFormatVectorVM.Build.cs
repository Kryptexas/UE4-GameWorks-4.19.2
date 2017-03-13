// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ShaderFormatVectorVM : ModuleRules
{
	public ShaderFormatVectorVM(ReadOnlyTargetRules Target) : base(Target)
	{
        
        PrivateIncludePathModuleNames.AddRange(
            new string[] {
                "TargetPlatform",
                "NiagaraEditor"
                }
            );


        PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
                "CoreUObject",                
                "ShaderCore",
				"ShaderCompilerCommon",
				"ShaderPreprocessor",
            	"VectorVM",
                "Niagara",
                }
			);

		AddEngineThirdPartyPrivateStaticDependencies(Target, 
			"HLSLCC"
			);
	}
}
