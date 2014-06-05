// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SessionFrontendPrivatePCH.h: Pre-compiled header file for the SessionFrontend module.
=============================================================================*/

#pragma once

#include "SessionFrontend.h"


/* Dependencies
 *****************************************************************************/

#include "DesktopPlatformModule.h"
#include "EditorStyle.h"
#include "Json.h"
#include "Messaging.h"
#include "ModuleManager.h"
#include "Slate.h"
#include "TargetDeviceServices.h"
#include "TargetPlatform.h"
#include "PlatformInfo.h"

// @todo gmp: remove these dependencies by making the session front-end extensible
#include "AutomationWindow.h"
#include "ScreenShotComparison.h"
#include "ScreenShotComparisonTools.h"
#include "Profiler.h"


/* Private includes
 *****************************************************************************/

// session browser
#include "SessionBrowserOwnerFilter.h"

#include "SSessionBrowserInstanceListRow.h"
#include "SSessionBrowserInstanceList.h"
#include "SSessionBrowserSessionListRow.h"
#include "SSessionBrowserCommandBar.h"
#include "SSessionBrowser.h"

// session console
#include "SessionConsoleCategoryFilter.h"
#include "SessionConsoleCommands.h"
#include "SessionConsoleLogMessage.h"
#include "SessionConsoleVerbosityFilter.h"

#include "SSessionConsoleLogTableRow.h"
#include "SSessionConsoleCommandBar.h"
#include "SSessionConsoleFilterBar.h"
#include "SSessionConsoleShortcutWindow.h"
#include "SSessionConsoleToolbar.h"
#include "SSessionConsole.h"

// session front-end
#include "SSessionFrontend.h"
