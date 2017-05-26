// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Settings/WidgetDesignerSettings.h"

UWidgetDesignerSettings::UWidgetDesignerSettings()
{
	GridSnapEnabled = true;
	GridSnapSize = 4;
	bShowOutlines = true;
	bExecutePreConstructEvent = true;
	bRespectLocks = true;
}
