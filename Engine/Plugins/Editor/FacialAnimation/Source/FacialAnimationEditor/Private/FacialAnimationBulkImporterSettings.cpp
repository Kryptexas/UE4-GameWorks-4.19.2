// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "FacialAnimationBulkImporterSettings.h"

void UFacialAnimationBulkImporterSettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	SaveConfig();
}
