// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SessionFrontendPrivatePCH.h: Pre-compiled header file for the SessionFrontend module.
=============================================================================*/

#pragma once


#include "../Public/SessionFrontend.h"


/* Dependencies
 *****************************************************************************/

#include "ModuleManager.h"
#include "DesktopPlatformModule.h"
#include "Json.h"
#include "Slate.h"
#include "EditorStyle.h"

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
#include "SSessionBrowserContextMenu.h"
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
