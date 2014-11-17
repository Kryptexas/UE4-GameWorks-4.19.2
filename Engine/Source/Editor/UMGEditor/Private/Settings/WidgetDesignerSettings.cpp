// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

UWidgetDesignerSettings::UWidgetDesignerSettings(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	GridSnapEnabled = true;
	GridSnapSize = 4;
}
