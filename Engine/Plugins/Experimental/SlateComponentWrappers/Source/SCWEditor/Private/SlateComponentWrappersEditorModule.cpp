// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlateComponentWrappersEditorPrivatePCH.h"
#include "AssetToolsModule.h"

#include "SCWBlueprintEditorExtensionHook.h"

//////////////////////////////////////////////////////////////////////////
// FSlateComponentWrapperEditorModule

class FSlateComponentWrapperEditorModule : public IModuleInterface
{
private:
	/** All created asset type actions.  Cached here so that we can unregister it during shutdown. */
	TArray< TSharedPtr<IAssetTypeActions> > CreatedAssetTypeActions;

// 	TSharedPtr<IComponentAssetBroker> PaperSpriteBroker;
// 	TSharedPtr<IComponentAssetBroker> PaperFlipbookBroker;
public:
	virtual void StartupModule() OVERRIDE
	{
		// Register commands
// 		FPaperEditorCommands::Register();
// 
// 		// Register asset types
// 		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
// 		RegisterAssetTypeAction(AssetTools, MakeShareable(new FSpriteAssetTypeActions));
// 		RegisterAssetTypeAction(AssetTools, MakeShareable(new FFlipbookAssetTypeActions));
// 		RegisterAssetTypeAction(AssetTools, MakeShareable(new FTileSetAssetTypeActions));
// 
// 		PaperSpriteBroker = MakeShareable(new FPaperSpriteAssetBroker);
// 		FComponentAssetBrokerage::RegisterBroker(PaperSpriteBroker, UPaperRenderComponent::StaticClass(), true);
// 
// 		PaperFlipbookBroker = MakeShareable(new FPaperFlipbookAssetBroker);
// 		FComponentAssetBrokerage::RegisterBroker(PaperFlipbookBroker, UPaperAnimatedRenderComponent::StaticClass(), true);
// 
// 		// Register the details customizations
// 		{
// 			FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
// 			PropertyModule.RegisterCustomPropertyLayout(UPaperTileMapRenderComponent::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FPaperTileMapDetailsCustomization::MakeInstance));
// 			PropertyModule.RegisterCustomPropertyLayout(UPaperSprite::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FSpriteDetailsCustomization::MakeInstance));
// 
// 			//@TODO: Struct registration should happen using ::StaticStruct, not by string!!!
// 			//PropertyModule.RegisterStructPropertyLayout( "SpritePolygonCollection", FOnGetStructCustomizationInstance::CreateStatic( &FSpritePolygonCollectionCustomization::MakeInstance ) );
// 
// 			PropertyModule.NotifyCustomizationModuleChanged();
// 		}
// 
// 		// Register the thumbnail renderers
// 		UThumbnailManager::Get().RegisterCustomRenderer(UPaperSprite::StaticClass(), UPaperSpriteThumbnailRenderer::StaticClass());
// 		UThumbnailManager::Get().RegisterCustomRenderer(UPaperTileSet::StaticClass(), UPaperTileSetThumbnailRenderer::StaticClass());
// 		UThumbnailManager::Get().RegisterCustomRenderer(UPaperFlipbook::StaticClass(), UPaperFlipbookThumbnailRenderer::StaticClass());
// 
// 		// Register the editor modes
// 		TileMapEditorModePtr = MakeShareable(new FEdModeTileMap);
// 		TileMapEditorModePtr->Initialize();
// 		GEditorModeTools().RegisterMode(TileMapEditorModePtr.ToSharedRef());
// 
 		// Integrate the preview tab into the blueprint editor
 		FSCWBlueprintEditorExtensionHook::InstallHooks();
	}

	virtual void ShutdownModule() OVERRIDE
	{
// 		SpriteEditor_MenuExtensibilityManager.Reset();
// 		SpriteEditor_ToolBarExtensibilityManager.Reset();
// 
// 		FlipbookEditor_MenuExtensibilityManager.Reset();
// 		FlipbookEditor_ToolBarExtensibilityManager.Reset();
// 
 		if (UObjectInitialized())
 		{
 			FSCWBlueprintEditorExtensionHook::RemoveHooks();
// 
// 			FComponentAssetBrokerage::UnregisterBroker(PaperFlipbookBroker);
// 			FComponentAssetBrokerage::UnregisterBroker(PaperSpriteBroker);
// 
// 			// Unregister the editor modes
// 			GEditorModeTools().UnregisterMode(TileMapEditorModePtr.ToSharedRef());
// 
// 			// Unregister the thumbnail renderers
// 			UThumbnailManager::Get().UnregisterCustomRenderer(UPaperSprite::StaticClass());
// 			UThumbnailManager::Get().UnregisterCustomRenderer(UPaperTileSet::StaticClass());
// 			UThumbnailManager::Get().UnregisterCustomRenderer(UPaperFlipbook::StaticClass());
 		}
// 
// 		// Unregister the details customization
// 		//@TODO: Unregister them
// 
		// Unregister all the asset types that we registered
		if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
		{
			IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
			for (int32 Index = 0; Index < CreatedAssetTypeActions.Num(); ++Index)
			{
				AssetTools.UnregisterAssetTypeActions(CreatedAssetTypeActions[Index].ToSharedRef());
			}
		}
		CreatedAssetTypeActions.Empty();
// 
// 		// Unregister commands
// 		FPaperEditorCommands::Unregister();
	}
private:
	void RegisterAssetTypeAction(IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action)
	{
		AssetTools.RegisterAssetTypeActions(Action);
		CreatedAssetTypeActions.Add(Action);
	}
};

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_MODULE(FSlateComponentWrapperEditorModule, SlateComponentWrapperEditor);
