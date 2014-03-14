// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "UnrealEd.h"
#include "SurfaceIterators.h"
#include "BSPOps.h"


FModeTool::FModeTool():
	ID( MT_None ),
	bUseWidget( 1 )
{}

FModeTool::~FModeTool()
{
}

void FModeTool::DrawHUD(FLevelEditorViewportClient* ViewportClient,FViewport* Viewport,const FSceneView* View,FCanvas* Canvas)
{
}

void FModeTool::Render(const FSceneView* View,FViewport* Viewport,FPrimitiveDrawInterface* PDI)
{
}


