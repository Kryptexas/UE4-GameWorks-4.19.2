// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Widgets/SCompoundWidget.h"
#include "WorkflowOrientedApp/WorkflowTabFactory.h"

namespace CallStackViewer
{
	void KISMET_API UpdateDisplayedCallstack(const TArray<const FFrame*>& ScriptStack);
	void RegisterTabSpawner();
}
