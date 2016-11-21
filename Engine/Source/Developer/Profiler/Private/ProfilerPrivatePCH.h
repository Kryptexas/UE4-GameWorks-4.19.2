// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Runtime/Core/Public/Core.h"
#include "Runtime/Core/Public/HAL/FileManagerGeneric.h"
#include "Runtime/Core/Public/ProfilingDebugging/DiagnosticTable.h"
#include "Runtime/Core/Public/Stats/StatsData.h"
#include "Runtime/Core/Public/Stats/StatsFile.h"
#include "Runtime/RenderCore/Public/RenderingThread.h"
#include "Runtime/SessionServices/Public/ISessionInfo.h"
#include "Runtime/SessionServices/Public/ISessionInstanceInfo.h"
#include "Runtime/SessionServices/Public/ISessionManager.h"
#include "Runtime/Slate/Public/SlateBasics.h"
#include "Runtime/Slate/Public/Widgets/Docking/SDockTab.h"
#include "Runtime/Slate/Public/Widgets/Input/SSearchBox.h"
#include "Runtime/Slate/Public/Widgets/Notifications/SNotificationList.h"

#include "Developer/DesktopPlatform/Public/DesktopPlatformModule.h"
#include "Developer/ProfilerClient/Public/IProfilerClient.h"
#include "Developer/ProfilerClient/Public/IProfilerClientModule.h"

#include "Editor/EditorStyle/Public/EditorStyleSet.h" // @todo profiler: fix Editor style dependencies

#if WITH_EDITOR
	#include "Runtime/Analytics/Analytics/Public/Interfaces/IAnalyticsProvider.h"
	#include "Runtime/Engine/Public/EngineAnalytics.h"
#endif

#include "IProfilerModule.h"
#include "ProfilerCommon.h"
