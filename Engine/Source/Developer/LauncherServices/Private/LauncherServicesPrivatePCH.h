// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LauncherServicesPrivatePCH.h: Pre-compiled header file for the LauncherServices module.
=============================================================================*/

#pragma once


#include "../Public/LauncherServices.h"


/* Dependencies
 *****************************************************************************/

#include "Messaging.h"
#include "ModuleManager.h"
#include "SessionMessages.h"
#include "TargetDeviceServices.h"
#include "TargetPlatform.h"
#include "Ticker.h"
#include "UnrealEdMessages.h"


/* Private includes
 *****************************************************************************/

/**
 * Defines the launcher profile file version.
 */
#define LAUNCHERSERVICES_PROFILEVERSION 8


// profile manager
#include "LauncherDeviceGroup.h"
#include "LauncherProfileLaunchRole.h"
#include "LauncherProfile.h"
#include "LauncherProfileManager.h"

// launcher worker
#include "LauncherTaskChainState.h"
#include "LauncherTask.h"
#include "LauncherUATCommand.h"
#include "LauncherUATTask.h"
#include "LauncherBuildCommands.h"
#include "LauncherCookCommands.h"
#include "LauncherDeployCommands.h"
#include "LauncherLaunchCommands.h"
#include "LauncherPackageCommands.h"
#include "LauncherVerifyProfileTask.h"
#include "LauncherWorker.h"

#include "Launcher.h"
