// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

// Mode constants
const FName FWidgetBlueprintApplicationModes::DesignerMode("DesignModeName");
const FName FWidgetBlueprintApplicationModes::GraphMode("GraphModeName");

FText FWidgetBlueprintApplicationModes::GetLocalizedMode(const FName InMode)
{
	static TMap< FName, FText > LocModes;

	if ( LocModes.Num() == 0 )
	{
		LocModes.Add(DesignerMode, NSLOCTEXT("WidgetBlueprintModes", "DesignerMode", "Designer"));
		LocModes.Add(GraphMode, NSLOCTEXT("WidgetBlueprintModes", "GraphMode", "Graph"));
	}

	check(InMode != NAME_None);
	const FText* OutDesc = LocModes.Find(InMode);
	check(OutDesc);

	return *OutDesc;
}
