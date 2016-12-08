// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class DocumentationEditorTarget : TargetRules
{
	public DocumentationEditorTarget(TargetInfo Target)
	{
		Type = TargetType.Editor;
	}

	//
	// TargetRules interface.
	//

	public override void SetupBinaries(
		TargetInfo Target,
		ref List<UEBuildBinaryConfiguration> OutBuildBinaryConfigurations,
		ref List<string> OutExtraModuleNames
		)
	{
		OutExtraModuleNames.Add("UE4Game");
		OutExtraModuleNames.Add("OnlineSubsystemAmazon");
		OutExtraModuleNames.Add("OnlineSubsystemFacebook");
		OutExtraModuleNames.Add("OnlineSubsystemNull");
		OutExtraModuleNames.Add("OnlineSubsystemSteam");
	}
}
