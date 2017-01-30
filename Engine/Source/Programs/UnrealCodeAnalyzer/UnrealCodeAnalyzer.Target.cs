// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;
using System.Text;

[SupportedPlatforms(UnrealTargetPlatform.Win32)]
public class UnrealCodeAnalyzerTarget : TargetRules
{
	public UnrealCodeAnalyzerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Program;
		LaunchModuleName = "UnrealCodeAnalyzer";
	}

	//
	// TargetRules interface.
	//

	public override void SetupGlobalEnvironment(
		TargetInfo Target,
		ref LinkEnvironmentConfiguration OutLinkEnvironmentConfiguration,
		ref CPPEnvironmentConfiguration OutCPPEnvironmentConfiguration
		)
	{
		// UnrealCodeAnalyzer is a console application, not a Windows app (sets entry point to main(), instead of WinMain())
		OutLinkEnvironmentConfiguration.bIsBuildingConsoleApplication = true;

		// Lean and mean
		UEBuildConfiguration.bCompileLeanAndMeanUE = true;

		// No editor needed
		UEBuildConfiguration.bBuildEditor = false;

		// Do not link against CoreUObject
		UEBuildConfiguration.bCompileAgainstCoreUObject = false;

		// Currently this app is not linking against the engine, so we'll compile out references from Core to the rest of the engine
		UEBuildConfiguration.bCompileAgainstEngine = false;

		/*
		if (Target.Platform == UnrealTargetPlatform.Win32)
		{
			// Clean up some warnings issued by cl.exe when compiling llvm/clang source headers.
			StringBuilder ExtraArguments = new StringBuilder();

			// Unreferenced formal parameter.
			ExtraArguments.Append(" /wd4100");

			// Unary minus operator applied to unsigned type, result still unsigned.
			ExtraArguments.Append(" /wd4146");

			// Conversion from <type> to <type>, possible loss of data.
			ExtraArguments.Append(" /wd4244");

			// Conditional expression is constant.
			ExtraArguments.Append(" /wd4127");

			// Default constructor could not be generated.
			ExtraArguments.Append(" /wd4510");

			// Assignment operator could not be generated.
			ExtraArguments.Append(" /wd4512");

			// <struct> can never be instantiated - user defined constructor required.
			ExtraArguments.Append(" /wd4610");

			// Destructor could not be generated because a base class destructor is inaccessible or deleted.
			ExtraArguments.Append(" /wd4624");

			// Unreachable code.
			ExtraArguments.Append(" /wd4702");

			// Forcing value to bool 'true' or 'false' (performance warning).
			ExtraArguments.Append(" /wd4800");

			OutCPPEnvironmentConfiguration.AdditionalArguments += ExtraArguments.ToString();
		}*/
	}
}
