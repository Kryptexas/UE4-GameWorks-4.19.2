// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class UE4ClientTarget : TargetRules
{
    public UE4ClientTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Client;
		BuildEnvironment = TargetBuildEnvironment.Shared;
		ExtraModuleNames.Add("UE4Game");
	}
}
