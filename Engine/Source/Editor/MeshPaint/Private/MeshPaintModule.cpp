// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MeshPaintPrivatePCH.h"
#include "MeshPaintEdMode.h"

class FMeshPaintModule : public IMeshPaintModule
{
private:
	TSharedPtr<FEdModeMeshPaint> EdModeMeshPaint;
public:
	
	/**
	 * Called right after the module's DLL has been loaded and the module object has been created
	 */
	virtual void StartupModule() OVERRIDE
	{
		TSharedRef<FEdModeMeshPaint> NewEditorMode = MakeShareable(new FEdModeMeshPaint);
		GEditorModeTools().RegisterMode(NewEditorMode);
		EdModeMeshPaint = NewEditorMode;
	}

	/**
	 * Called before the module is unloaded, right before the module object is destroyed.
	 */
	virtual void ShutdownModule() OVERRIDE
	{
		GEditorModeTools().UnregisterMode(EdModeMeshPaint.ToSharedRef());
		EdModeMeshPaint = NULL;
	}
};

IMPLEMENT_MODULE( FMeshPaintModule, MeshPaint );

