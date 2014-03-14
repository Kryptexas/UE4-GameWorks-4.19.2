// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	Launcher.cpp: Implements the FLauncher class.
=============================================================================*/

#include "LauncherServicesPrivatePCH.h"


/* ILauncher overrides
 *****************************************************************************/

ILauncherWorkerPtr FLauncher::Launch( const ITargetDeviceProxyManagerRef& DeviceProxyManager, const ILauncherProfileRef& Profile )
{
	if (Profile->IsValidForLaunch())
	{
		FLauncherWorker* LauncherWorker = new FLauncherWorker(DeviceProxyManager, Profile);

		if ((LauncherWorker != NULL) && (FRunnableThread::Create(LauncherWorker, TEXT("LauncherWorker"), false, false, 0, TPri_Normal) != NULL))
		{
			return MakeShareable(LauncherWorker);
		}			
	}

	return NULL;
}
