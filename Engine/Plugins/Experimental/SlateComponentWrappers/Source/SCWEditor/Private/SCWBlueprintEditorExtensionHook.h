// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

// Integrate SlateComponentWrapper into the blueprint editor (custom preview panel)
class FSCWBlueprintEditorExtensionHook
{
public:
	static void InstallHooks();
	static void RemoveHooks();
};