// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "MediaAssetEditorPrivatePCH.h"
#include "ModuleManager.h"


#define LOCTEXT_NAMESPACE "FMediaAssetEditorModule"

static const FName MediaAssetEditorAppIdentifier("MediaAssetEditorApp");


/**
 * Implements the MediaAssetEditor module.
 */
class FMediaAssetEditorModule
	: public IHasMenuExtensibility
	, public IHasToolBarExtensibility
	, public IModuleInterface
{
public:

	// IHasMenuExtensibility interface

	virtual TSharedPtr<FExtensibilityManager> GetMenuExtensibilityManager( )
	{
		return MenuExtensibilityManager;
	}

public:

	// IHasToolBarExtensibility interface

	virtual TSharedPtr<FExtensibilityManager> GetToolBarExtensibilityManager( )
	{
		return ToolBarExtensibilityManager;
	}

public:

	// IModuleInterface interface

	virtual void StartupModule( ) override
	{
		Style = MakeShareable(new FMediaAssetEditorStyle());

		FMediaAssetEditorCommands::Register();

		RegisterAssetTools();
		RegisterMenuExtensions();
		RegisterThumbnailRenderers();
	}

	virtual void ShutdownModule( ) override
	{
		UnregisterAssetTools();
		UnregisterMenuExtensions();
		UnregisterThumbnailRenderers();
	}

protected:

	/** Registers asset tool actions. */
	void RegisterAssetTools( )
	{
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

		RegisterAssetTypeAction(AssetTools, MakeShareable(new FMediaAssetActions(Style.ToSharedRef())));
		RegisterAssetTypeAction(AssetTools, MakeShareable(new FMediaSoundWaveActions));
		RegisterAssetTypeAction(AssetTools, MakeShareable(new FMediaTextureActions));
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

	/** Registers main menu and tool bar menu extensions. */
	void RegisterMenuExtensions( )
	{
		MenuExtensibilityManager = MakeShareable(new FExtensibilityManager);
		ToolBarExtensibilityManager = MakeShareable(new FExtensibilityManager);
	}

	/** Registers asset thumbnail renderers .*/
	void RegisterThumbnailRenderers( )
	{
		UThumbnailManager::Get().RegisterCustomRenderer(UMediaTexture::StaticClass(), UTextureThumbnailRenderer::StaticClass());
	}

	/** Unregisters asset tool actions. */
	void UnregisterAssetTools( )
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

	/** Unregisters main menu and tool bar menu extensions. */
	void UnregisterMenuExtensions( )
	{
		MenuExtensibilityManager.Reset();
		ToolBarExtensibilityManager.Reset();
	}

	/** Unregisters all asset thumbnail renderers. */
	void UnregisterThumbnailRenderers( )
	{
		if (UObjectInitialized())
		{
			UThumbnailManager::Get().UnregisterCustomRenderer(UMediaTexture::StaticClass());
		}
	}

private:

	/** Holds the menu extensibility manager. */
	TSharedPtr<FExtensibilityManager> MenuExtensibilityManager;

	/** The collection of registered asset type actions. */
	TArray<TSharedRef<IAssetTypeActions>> RegisteredAssetTypeActions;

	/** Holds the plug-ins style set. */
	TSharedPtr<ISlateStyle> Style;

	/** Holds the tool bar extensibility manager. */
	TSharedPtr<FExtensibilityManager> ToolBarExtensibilityManager;
};


IMPLEMENT_MODULE(FMediaAssetEditorModule, MediaAssetEditor);


#undef LOCTEXT_NAMESPACE
