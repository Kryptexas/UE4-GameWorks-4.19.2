// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MessagingDebugger : ModuleRules
{
	public MessagingDebugger(TargetInfo Target)
	{
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
                "InputCore",
				"Messaging",
                "PropertyEditor",
				"Slate",
                "EditorStyle"
			}
		);

		PrivateIncludePaths.AddRange(
			new string[] {
				"MessagingDebugger/Private",
				"MessagingDebugger/Private/Models",
				"MessagingDebugger/Private/Styles",
				"MessagingDebugger/Private/Widgets",
				"MessagingDebugger/Private/Widgets/Breakpoints",
				"MessagingDebugger/Private/Widgets/EndpointDetails",
				"MessagingDebugger/Private/Widgets/Endpoints",
				"MessagingDebugger/Private/Widgets/Graph",
				"MessagingDebugger/Private/Widgets/History",
				"MessagingDebugger/Private/Widgets/Interceptors",
                "MessagingDebugger/Private/Widgets/MessageData",
				"MessagingDebugger/Private/Widgets/MessageDetails",
				"MessagingDebugger/Private/Widgets/Types",
				"MessagingDebugger/Private/Widgets/Toolbar",
			}
		);
	}
}
