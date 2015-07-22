// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "ActorAnimationEditorPrivatePCH.h"
#include "ModuleInterface.h"


/**
 * Implements the ActorAnimationEditor module.
 */
class FActorAnimationEditorModule
	: public IModuleInterface
{
public:

	// IModuleInterface interface

	virtual void StartupModule() override
	{
		RegisterAssetTools();
	}
	
	virtual void ShutdownModule() override
	{
		UnregisterAssetTools();
	}

protected:

	/** Registers asset tool actions. */
	void RegisterAssetTools()
	{
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

		RegisterAssetTypeAction(AssetTools, MakeShareable(new FActorAnimationActions));
	}

	/**
	 * Registers a single asset type action.
	 *
	 * @param AssetTools The asset tools object to register with.
	 * @param Action The asset type action to register.
	 */
	void RegisterAssetTypeAction( IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action )
	{
		AssetTools.RegisterAssetTypeActions(Action);
		RegisteredAssetTypeActions.Add(Action);
	}

	/** Unregisters asset tool actions. */
	void UnregisterAssetTools()
	{
		FAssetToolsModule* AssetToolsModule = FModuleManager::GetModulePtr<FAssetToolsModule>("AssetTools");

		if (AssetToolsModule != nullptr)
		{
			IAssetTools& AssetTools = AssetToolsModule->Get();

			for (auto Action : RegisteredAssetTypeActions)
			{
				AssetTools.UnregisterAssetTypeActions(Action);
			}
		}
	}

private:

	/** The collection of registered asset type actions. */
	TArray<TSharedRef<IAssetTypeActions>> RegisteredAssetTypeActions;
};


IMPLEMENT_MODULE(FActorAnimationEditorModule, ActorAnimationEditor);
