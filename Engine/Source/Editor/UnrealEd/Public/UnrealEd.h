// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#ifndef __UnrealEd_h__
#define __UnrealEd_h__

#if WITH_EDITOR

#ifndef WITH_SIMPLYGON
#error WITH_SIMPLYGON must have been defined to either 0 or 1 by UnrealBuildTool to be able to include this header file
#endif

#ifndef WITH_SPEEDTREE
#error WITH_SPEEDTREE must have been defined to either 0 or 1 by UnrealBuildTool to be able to include this header file
#endif

#include "Engine.h"

#include "EngineUserInterfaceClasses.h"

#include "Slate.h"
#include "EditorStyle.h"

#include "EditorComponents.h"
#include "EditorReimportHandler.h"
#include "TexAlignTools.h"

#include "TickableEditorObject.h"

#include "UnrealEdClasses.h"

#include "Editor.h"

#include "EditorViewportClient.h"
#include "LevelEditorViewport.h"

#include "EditorModes.h"

#include "MRUList.h"
#include "Projects.h"


//#include "../Private/GeomFitUtils.h"

extern UNREALED_API class UUnrealEdEngine* GUnrealEd;

#include "UnrealEdMisc.h"
#include "EditorDirectories.h"
#include "Utils.h"
#include "FileHelpers.h"
#include "EditorModeInterpolation.h"
#include "PhysicsManipulationMode.h"
#include "PhysicsAssetUtils.h"

#include "ParticleDefinitions.h"

#include "Dialogs/Dialogs.h"
#include "Viewports.h"

#endif // WITH_EDITOR

UNREALED_API int32 EditorInit( class IEngineLoop& EngineLoop );
UNREALED_API void EditorExit();

#include "UnrealEdMessages.h"

#endif	// __UnrealEd_h__
