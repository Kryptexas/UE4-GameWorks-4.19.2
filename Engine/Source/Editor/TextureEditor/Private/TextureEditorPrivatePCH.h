// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	TextureEditorPrivatePCH.h: Pre-compiled header file for the TextureEditor module.
=============================================================================*/

#ifndef TEXTURE_EDITOR_PRIVATEPCH_H
#define TEXTURE_EDITOR_PRIVATEPCH_H


#include "../Public/TextureEditor.h"


/* Dependencies
 *****************************************************************************/

#include "CubemapUnwrapUtils.h"
#include "Factories.h"
#include "MouseDeltaTracker.h"
#include "NormalMapPreview.h"
#include "SceneViewport.h"
#include "SColorPicker.h"
#include "ScopedTransaction.h"
#include "Settings.h"
#include "Texture2DPreview.h"
#include "Toolkits/IToolkitHost.h"
#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructureModule.h"
#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
#include "Editor/PropertyEditor/Public/IDetailsView.h"


/* Private includes
 *****************************************************************************/

const double MaxZoom = 16.0;
const double ZoomStep = 0.1;

#include "TextureEditorCommands.h"
#include "TextureEditorSettings.h"
#include "TextureEditorViewOptionsMenu.h"
#include "STextureEditorViewportToolbar.h"
#include "STextureEditorViewport.h"
#include "TextureEditorViewportClient.h"
#include "TextureEditorToolkit.h"


#endif
