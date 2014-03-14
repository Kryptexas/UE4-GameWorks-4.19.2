// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class FriendsAndChat : ModuleRules
{
	public FriendsAndChat(TargetInfo Target)
	{
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
                "InputCore",
				"Slate",
				"OnlineSubsystem"
			}
		);

		PrivateDependencyModuleNames.AddRange(
		new string[]
			{
				"Sockets",
				"OnlineSubsystem"
			}
		);

		PrivateIncludePaths.AddRange(
			new string[]
			{
				"Developer/FriendsAndChat/Private",
				"Developer/FriendsAndChat/Private/UI/Widgets",
				"Developer/FriendsAndChat/Private/Models",
			}
		);

		PublicIncludePaths.AddRange(
			new string[]
			{
				"Developer/FriendsAndChat/Public/Models",
				"Developer/FriendsAndChat/Public/Interfaces"
			}
		);

		DynamicallyLoadedModuleNames.AddRange(
		new string[] {
				"HTTP",
				"EditorStyle",
				"OnlineSubsystem"
			}
		);

		PrivateIncludePathModuleNames.AddRange(
			new string[] {
				"HTTP",
				"EditorStyle",
			}
		);

		if (UEBuildConfiguration.bCompileMcpOSS == true)
		{
			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"OnlineSubsystemMcp",
				}
			);	

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"OnlineSubsystemMcp",
				}
			);
		
			DynamicallyLoadedModuleNames.AddRange(
				new string[]
				{
					"OnlineSubsystemMcp",
				}
			);
		}
	}
}
