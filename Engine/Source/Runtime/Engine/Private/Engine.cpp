// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	Engine.cpp: Unreal engine package.
=============================================================================*/

#include "EnginePrivate.h"
#include "EngineKismetLibraryClasses.h"

/*-----------------------------------------------------------------------------
	Globals.
-----------------------------------------------------------------------------*/

// Suppress linker warning "warning LNK4221: no public symbols found; archive member will be inaccessible"
int32 EngineLinkerHelper;


#if WITH_EDITOR
/*-----------------------------------------------------------------------------
 FEditorSupportDelegates
 Delegates that are needed for proper editor functionality, but are accessed 
 or triggered in engine code.
 -----------------------------------------------------------------------------*/
/** Called when all viewports need to be redrawn */
FSimpleMulticastDelegate FEditorSupportDelegates::RedrawAllViewports;
/** Called when the editor is cleansing of transient references before a map change event */
FSimpleMulticastDelegate FEditorSupportDelegates::CleanseEditor;
/** Called when the world is modified */
FSimpleMulticastDelegate FEditorSupportDelegates::WorldChange;
/** Sent to force a property window rebuild */
FEditorSupportDelegates::FOnForcePropertyWindowRebuild FEditorSupportDelegates::ForcePropertyWindowRebuild;
/** Sent when events happen that affect how the editors UI looks (mode changes, grid size changes, etc) */
FSimpleMulticastDelegate FEditorSupportDelegates::UpdateUI;
/** Called for a material after the user has change a texture's compression settings.
	Needed to notify the material editors that the need to reattach their preview objects */
FEditorSupportDelegates::FOnMaterialTextureSettingsChanged FEditorSupportDelegates::MaterialTextureSettingsChanged;
/** Refresh property windows w/o creating/destroying controls */
FSimpleMulticastDelegate FEditorSupportDelegates::RefreshPropertyWindows;
/** Sent before the given windows message is handled in the given viewport */
FEditorSupportDelegates::FOnWindowsMessage FEditorSupportDelegates::PreWindowsMessage;
/** Sent after the given windows message is handled in the given viewport */
FEditorSupportDelegates::FOnWindowsMessage FEditorSupportDelegates::PostWindowsMessage;
/** Sent after the usages flags on a material have changed*/
FEditorSupportDelegates::FOnMaterialUsageFlagsChanged FEditorSupportDelegates::MaterialUsageFlagsChanged;

#endif // WITH_EDITOR

IRendererModule* CachedRendererModule = NULL;

IRendererModule& GetRendererModule()
{
	if (!CachedRendererModule)
	{
		CachedRendererModule = &FModuleManager::LoadModuleChecked<IRendererModule>(TEXT("Renderer"));
	}

	return *CachedRendererModule;
}

void ResetCachedRendererModule()
{
	CachedRendererModule = NULL;
}
