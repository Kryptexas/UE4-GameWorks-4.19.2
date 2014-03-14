// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	WindowsTargetPlatformPrivatePCH.h: Pre-compiled header file for the WindowsTargetPlatform module.
=============================================================================*/

#pragma once


/* Dependencies
 *****************************************************************************/

#include "Core.h"
#include "ModuleManager.h"
#include "Settings.h"
#include "WindowsPlatformProperties.h"

#if WITH_ENGINE
	#include "Engine.h"
	#include "TextureCompressorModule.h"
#endif

#include "TargetPlatform.h"

#include "AllowWindowsPlatformTypes.h"
	#include <TlHelp32.h>
#include "HideWindowsPlatformTypes.h"

#if WITH_EDITOR
	#include "AssetSelection.h"
	#include "EditorStyle.h"
	#include "PropertyEditing.h"	
	#include "SExternalImageReference.h"	
#endif


/* Private includes
 *****************************************************************************/

#include "WindowsTargetSettings.h"
#include "LocalPcTargetDevice.h"
#include "GenericWindowsTargetPlatform.h"

#if WITH_EDITOR
	#include "WindowsTargetSettingsDetails.h"
#endif
