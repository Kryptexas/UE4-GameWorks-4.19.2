// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "UnrealEd.h"

#include "FoliageEditModule.h"

const FName FoliageEditAppIdentifier = FName(TEXT("FoliageEdApp"));

#include "FoliageEdMode.h"

/**
 * Foliage Edit Mode module
 */
class FFoliageEditModule : public IFoliageEditModule
{
private:
	TSharedPtr<FEdModeFoliage> EdModeFoliage;
public:
	/** Constructor, set up console commands and variables **/
	FFoliageEditModule()
	{
	}

	/**
	 * Called right after the module DLL has been loaded and the module object has been created
	 */
	virtual void StartupModule() OVERRIDE
	{
		TSharedRef<FEdModeFoliage> NewEditorMode = MakeShareable(new FEdModeFoliage);
		GEditorModeTools().RegisterMode(NewEditorMode);
		EdModeFoliage = NewEditorMode;
		FEditorDelegates::MapChange.AddSP( NewEditorMode, &FEdModeFoliage::NotifyMapRebuild );
	}

	/**
	 * Called before the module is unloaded, right before the module object is destroyed.
	 */
	virtual void ShutdownModule() OVERRIDE
	{
		GEditorModeTools().UnregisterMode(EdModeFoliage.ToSharedRef());
		EdModeFoliage = NULL;
		FEditorDelegates::MapChange.RemoveAll(this);
	}
};

IMPLEMENT_MODULE( FFoliageEditModule, FoliageEdit );
