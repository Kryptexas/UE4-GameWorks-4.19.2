// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "GlobalEditorNotification.h"
#include "UnrealClient.h"

void FGlobalEditorNotification::Tick(float DeltaTime)
{
	TickNotification(DeltaTime);
}

TStatId FGlobalEditorNotification::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FGlobalEditorNotification, STATGROUP_Tickables);
}
