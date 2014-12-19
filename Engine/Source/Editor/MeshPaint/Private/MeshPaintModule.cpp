// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MeshPaintPrivatePCH.h"
#include "MeshPaintEdMode.h"
#include "MeshPaintAdapterFactory.h"
#include "MeshPaintStaticMeshAdapter.h"

class FMeshPaintModule : public IMeshPaintModule
{
public:
	
	/**
	 * Called right after the module's DLL has been loaded and the module object has been created
	 */
	virtual void StartupModule() override
	{
		FEditorModeRegistry::Get().RegisterMode<FEdModeMeshPaint>(
			FBuiltinEditorModes::EM_MeshPaint,
			NSLOCTEXT("MeshPaint_Mode", "MeshPaint_ModeName", "Paint"),
			FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.MeshPaintMode", "LevelEditor.MeshPaintMode.Small"),
			true, 200
			);


		RegisterGeometryAdapterFactory(MakeShareable(new FMeshPaintGeometryAdapterForStaticMeshesFactory));
	}

	/**
	 * Called before the module is unloaded, right before the module object is destroyed.
	 */
	virtual void ShutdownModule() override
	{
		FEditorModeRegistry::Get().UnregisterMode(FBuiltinEditorModes::EM_MeshPaint);
	}

	virtual void RegisterGeometryAdapterFactory(TSharedRef<IMeshPaintGeometryAdapterFactory> Factory) override
	{
		FMeshPaintAdapterFactory::FactoryList.Add(Factory);
	}

	virtual void UnregisterGeometryAdapterFactory(TSharedRef<IMeshPaintGeometryAdapterFactory> Factory) override
	{
		FMeshPaintAdapterFactory::FactoryList.Remove(Factory);
	}
};

IMPLEMENT_MODULE( FMeshPaintModule, MeshPaint );

