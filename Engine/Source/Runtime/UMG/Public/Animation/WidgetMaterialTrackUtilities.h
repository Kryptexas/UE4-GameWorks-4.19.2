// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

namespace WidgetMaterialTrackUtilities
{
	/** Gets a slate brush from a property on a widget by the properties FName. */
	UMG_API FSlateBrush* GetBrush( UWidget* Widget, FName BrushPropertyName );

	/** Gets the properties on a widget which are slate brush properties, and who's slate brush has a valid material. */
	UMG_API void GetMaterialBrushProperties( UWidget* Widget, TArray<UProperty*>& MaterialBrushProperties );
}