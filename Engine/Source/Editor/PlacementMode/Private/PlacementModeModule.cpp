// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "PlacementModePrivatePCH.h"
#include "IPlacementModeModule.h"
#include "PlacementMode.h"

class FPlacementModeModule : public IPlacementModeModule
{
public:
	
	/**
	 * Called right after the module's DLL has been loaded and the module object has been created
	 */
	virtual void StartupModule() OVERRIDE
	{
		PlacementMode = FPlacementMode::Create();
		GEditorModeTools().RegisterMode( PlacementMode.ToSharedRef() );
		GEditorModeTools().RegisterCompatibleModes(PlacementMode->GetID(), FBuiltinEditorModes::EM_Default);
		GEditorModeTools().RegisterCompatibleModes(PlacementMode->GetID(), FBuiltinEditorModes::EM_Bsp);
		GEditorModeTools().RegisterCompatibleModes(PlacementMode->GetID(), FBuiltinEditorModes::EM_Geometry);
		GEditorModeTools().RegisterCompatibleModes(PlacementMode->GetID(), FBuiltinEditorModes::EM_InterpEdit);
		GEditorModeTools().RegisterCompatibleModes(PlacementMode->GetID(), FBuiltinEditorModes::EM_MeshPaint);
		GEditorModeTools().RegisterCompatibleModes(PlacementMode->GetID(), FBuiltinEditorModes::EM_Landscape);
		GEditorModeTools().RegisterCompatibleModes(PlacementMode->GetID(), FBuiltinEditorModes::EM_Foliage);
		GEditorModeTools().RegisterCompatibleModes(PlacementMode->GetID(), FBuiltinEditorModes::EM_Level);
		GEditorModeTools().RegisterCompatibleModes(PlacementMode->GetID(), FBuiltinEditorModes::EM_Physics);
		GEditorModeTools().RegisterCompatibleModes(PlacementMode->GetID(), FBuiltinEditorModes::EM_ActorPicker);
	}

	/**
	 * Called before the module is unloaded, right before the module object is destroyed.
	 */
	virtual void ShutdownModule() OVERRIDE
	{
		if ( PlacementMode.IsValid() )
		{
			GEditorModeTools().UnregisterCompatibleModes(PlacementMode->GetID(), FBuiltinEditorModes::EM_Default);
			GEditorModeTools().UnregisterCompatibleModes(PlacementMode->GetID(), FBuiltinEditorModes::EM_Bsp);
			GEditorModeTools().UnregisterCompatibleModes(PlacementMode->GetID(), FBuiltinEditorModes::EM_Geometry);
			GEditorModeTools().UnregisterCompatibleModes(PlacementMode->GetID(), FBuiltinEditorModes::EM_InterpEdit);
			GEditorModeTools().UnregisterCompatibleModes(PlacementMode->GetID(), FBuiltinEditorModes::EM_MeshPaint);
			GEditorModeTools().UnregisterCompatibleModes(PlacementMode->GetID(), FBuiltinEditorModes::EM_Landscape);
			GEditorModeTools().UnregisterCompatibleModes(PlacementMode->GetID(), FBuiltinEditorModes::EM_Foliage);
			GEditorModeTools().UnregisterCompatibleModes(PlacementMode->GetID(), FBuiltinEditorModes::EM_Level);
			GEditorModeTools().UnregisterCompatibleModes(PlacementMode->GetID(), FBuiltinEditorModes::EM_Physics);
			GEditorModeTools().UnregisterCompatibleModes(PlacementMode->GetID(), FBuiltinEditorModes::EM_ActorPicker);
			GEditorModeTools().UnregisterMode( PlacementMode.ToSharedRef() );
			PlacementMode = NULL;
		}
	}

	virtual void AddToRecentlyPlaced( const TArray< UObject* >& Assets, UActorFactory* FactoryUsed = NULL ) OVERRIDE
	{
		PlacementMode->AddToRecentlyPlaced( Assets, FactoryUsed );
	}

	virtual void AddToRecentlyPlaced( UObject* Asset, UActorFactory* FactoryUsed = NULL ) OVERRIDE
	{
		TArray< UObject* > Assets;
		Assets.Add( Asset );
		PlacementMode->AddToRecentlyPlaced( Assets, FactoryUsed );
	}

	virtual const TArray< FActorPlacementInfo >& GetRecentlyPlaced() const OVERRIDE
	{
		return PlacementMode->GetRecentlyPlaced();
	}

	virtual bool IsPlacementModeAvailable() const OVERRIDE
	{
		return PlacementMode.IsValid();
	}

	virtual TSharedRef< IPlacementMode > GetPlacementMode() const OVERRIDE
	{
		return PlacementMode.ToSharedRef();
	}

private:

	TSharedPtr< FPlacementMode > PlacementMode;

	TArray< TSharedPtr<FExtender> > ContentPaletteFiltersExtenders;
	TArray< TSharedPtr<FExtender> > PaletteExtenders;
};

IMPLEMENT_MODULE( FPlacementModeModule, PlacementMode );

