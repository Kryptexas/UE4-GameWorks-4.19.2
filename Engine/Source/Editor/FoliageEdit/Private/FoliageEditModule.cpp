// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


#include "UnrealEd.h"
#include "FoliageEditModule.h"

const FName FoliageEditAppIdentifier = FName(TEXT("FoliageEdApp"));

#include "FoliageEdMode.h"
#include "PropertyEditing.h"
#include "FoliageTypeDetails.h"

/**
 * Foliage Edit Mode module
 */
class FFoliageEditModule : public IFoliageEditModule
{
public:

	/**
	 * Called right after the module DLL has been loaded and the module object has been created
	 */
	virtual void StartupModule() override
	{
		FEditorModeRegistry::Get().RegisterMode<FEdModeFoliage>(
			FBuiltinEditorModes::EM_Foliage,
			NSLOCTEXT("EditorModes", "FoliageMode", "Foliage"),
			FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.FoliageMode", "LevelEditor.FoliageMode.Small"),
			true, 400
			);

		// Register the details customizer
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.RegisterCustomClassLayout("FoliageType", FOnGetDetailCustomizationInstance::CreateStatic(&FFoliageTypeDetails::MakeInstance));
	}

	/**
	 * Called before the module is unloaded, right before the module object is destroyed.
	 */
	virtual void ShutdownModule() override
	{
		FEditorModeRegistry::Get().UnregisterMode(FBuiltinEditorModes::EM_Foliage);

		if (!UObjectInitialized())
		{
			return;
		}

		// Unregister the details customization
		if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
		{
			FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
			PropertyModule.UnregisterCustomClassLayout("FoliageType");
			PropertyModule.NotifyCustomizationModuleChanged();
		}
	}
};

IMPLEMENT_MODULE( FFoliageEditModule, FoliageEdit );
