// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"
#include "Paper2DEditorModule.h"

#include "SpriteAssetTypeActions.h"
#include "FlipbookAssetTypeActions.h"
#include "TileSetAssetTypeActions.h"

#include "AssetToolsModule.h"
#include "PropertyEditorModule.h"
#include "PaperStyle.h"
#include "PaperEditorCommands.h"
#include "TileMapEditing/EdModeTileMap.h"
#include "TileMapEditing/PaperTileMapDetailsCustomization.h"
#include "SpriteEditor/SpriteDetailsCustomization.h"
#include "SpriteEditor/SpritePolygonCollectionCustomization.h"
#include "PaperSpriteAssetBroker.h"
#include "PaperFlipbookAssetBroker.h"
#include "AssetEditorToolkit.h"

#include "ContentBrowserExtensions/ContentBrowserExtensions.h"

// Atlas support
#include "Atlasing/AtlasAssetTypeActions.h"
#include "Atlasing/PaperAtlasGenerator.h"

// Settings
#include "PaperRuntimeSettings.h"
#include "Settings.h"

// Intro tutorials
#include "Editor/IntroTutorials/Public/IIntroTutorials.h"

DEFINE_LOG_CATEGORY(LogPaper2DEditor);

#define LOCTEXT_NAMESPACE "Paper2DEditor"

//////////////////////////////////////////////////////////////////////////
// FPaper2DEditor

class FPaper2DEditor : public IPaper2DEditorModule
{
public:
	// IPaper2DEditorModule interface
	virtual TSharedPtr<FExtensibilityManager> GetSpriteEditorMenuExtensibilityManager() override { return SpriteEditor_MenuExtensibilityManager; }
	virtual TSharedPtr<FExtensibilityManager> GetSpriteEditorToolBarExtensibilityManager() override { return SpriteEditor_ToolBarExtensibilityManager; }

	virtual TSharedPtr<FExtensibilityManager> GetFlipbookEditorMenuExtensibilityManager() override { return FlipbookEditor_MenuExtensibilityManager; }
	virtual TSharedPtr<FExtensibilityManager> GetFlipbookEditorToolBarExtensibilityManager() override { return FlipbookEditor_ToolBarExtensibilityManager; }
	// End of IPaper2DEditorModule

private:
	TSharedPtr<FExtensibilityManager> SpriteEditor_MenuExtensibilityManager;
	TSharedPtr<FExtensibilityManager> SpriteEditor_ToolBarExtensibilityManager;

	TSharedPtr<FExtensibilityManager> FlipbookEditor_MenuExtensibilityManager;
	TSharedPtr<FExtensibilityManager> FlipbookEditor_ToolBarExtensibilityManager;

	/** All created asset type actions.  Cached here so that we can unregister them during shutdown. */
	TArray< TSharedPtr<IAssetTypeActions> > CreatedAssetTypeActions;

	TSharedPtr<IComponentAssetBroker> PaperSpriteBroker;
	TSharedPtr<IComponentAssetBroker> PaperFlipbookBroker;

	FCoreDelegates::FOnObjectPropertyChanged::FDelegate OnPropertyChangedHandle;

public:
	virtual void StartupModule() override
	{
		SpriteEditor_MenuExtensibilityManager = MakeShareable(new FExtensibilityManager);
		SpriteEditor_ToolBarExtensibilityManager = MakeShareable(new FExtensibilityManager);

		FlipbookEditor_MenuExtensibilityManager = MakeShareable(new FExtensibilityManager);
		FlipbookEditor_ToolBarExtensibilityManager = MakeShareable(new FExtensibilityManager);

		// Register slate style overrides
		FPaperStyle::Initialize();

		// Register commands
		FPaperEditorCommands::Register();

		// Register asset types
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		RegisterAssetTypeAction(AssetTools, MakeShareable(new FSpriteAssetTypeActions));
		RegisterAssetTypeAction(AssetTools, MakeShareable(new FFlipbookAssetTypeActions));
		RegisterAssetTypeAction(AssetTools, MakeShareable(new FTileSetAssetTypeActions));
		RegisterAssetTypeAction(AssetTools, MakeShareable(new FAtlasAssetTypeActions));

		PaperSpriteBroker = MakeShareable(new FPaperSpriteAssetBroker);
		FComponentAssetBrokerage::RegisterBroker(PaperSpriteBroker, UPaperSpriteComponent::StaticClass(), true, true);

		PaperFlipbookBroker = MakeShareable(new FPaperFlipbookAssetBroker);
		FComponentAssetBrokerage::RegisterBroker(PaperFlipbookBroker, UPaperFlipbookComponent::StaticClass(), true, true);

		// Register the details customizations
		{
			FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
			PropertyModule.RegisterCustomClassLayout("PaperTileMapRenderComponent", FOnGetDetailCustomizationInstance::CreateStatic(&FPaperTileMapDetailsCustomization::MakeInstance));
			PropertyModule.RegisterCustomClassLayout("PaperSprite", FOnGetDetailCustomizationInstance::CreateStatic(&FSpriteDetailsCustomization::MakeInstance));

			//@TODO: Struct registration should happen using ::StaticStruct, not by string!!!
			//PropertyModule.RegisterCustomPropertyTypeLayout( "SpritePolygonCollection", FOnGetPropertyTypeCustomizationInstance::CreateStatic( &FSpritePolygonCollectionCustomization::MakeInstance ) );

			PropertyModule.NotifyCustomizationModuleChanged();
		}

		// Register to be notified when properties are edited
		OnPropertyChangedHandle = FCoreDelegates::FOnObjectPropertyChanged::FDelegate::CreateRaw(this, &FPaper2DEditor::OnPropertyChanged);
		FCoreDelegates::OnObjectPropertyChanged.Add(OnPropertyChangedHandle);

		// Register the thumbnail renderers
		UThumbnailManager::Get().RegisterCustomRenderer(UPaperSprite::StaticClass(), UPaperSpriteThumbnailRenderer::StaticClass());
		UThumbnailManager::Get().RegisterCustomRenderer(UPaperTileSet::StaticClass(), UPaperTileSetThumbnailRenderer::StaticClass());
		UThumbnailManager::Get().RegisterCustomRenderer(UPaperFlipbook::StaticClass(), UPaperFlipbookThumbnailRenderer::StaticClass());

		// Register the editor modes
		FEditorModeRegistry::Get().RegisterMode<FEdModeTileMap>(
			FEdModeTileMap::EM_TileMap,
			LOCTEXT("TileMapEditMode", "Tile Map Editor"),
			FSlateIcon(),
			true);

		// Integrate Paper2D actions associated with existing engine types (e.g., Texture2D) into the content browser
		FPaperContentBrowserExtensions::InstallHooks();

		RegisterSettings();
		RegisterIntroTutorials();
	}

	virtual void ShutdownModule() override
	{
		SpriteEditor_MenuExtensibilityManager.Reset();
		SpriteEditor_ToolBarExtensibilityManager.Reset();

		FlipbookEditor_MenuExtensibilityManager.Reset();
		FlipbookEditor_ToolBarExtensibilityManager.Reset();

		if (UObjectInitialized())
		{
			UnregisterIntroTutorials();
			UnregisterSettings();

			FPaperContentBrowserExtensions::RemoveHooks();

			FComponentAssetBrokerage::UnregisterBroker(PaperFlipbookBroker);
			FComponentAssetBrokerage::UnregisterBroker(PaperSpriteBroker);

			// Unregister the editor modes
			FEditorModeRegistry::Get().UnregisterMode(FEdModeTileMap::EM_TileMap);

			// Unregister the thumbnail renderers
			UThumbnailManager::Get().UnregisterCustomRenderer(UPaperSprite::StaticClass());
			UThumbnailManager::Get().UnregisterCustomRenderer(UPaperTileSet::StaticClass());
			UThumbnailManager::Get().UnregisterCustomRenderer(UPaperFlipbook::StaticClass());

			// Unregister the property modification handler
			FCoreDelegates::OnObjectPropertyChanged.Remove(OnPropertyChangedHandle);
		}

		// Unregister the details customization
		//@TODO: Unregister them

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

		// Unregister commands
		FPaperEditorCommands::Unregister();
	}
private:
	void RegisterAssetTypeAction(IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action)
	{
		AssetTools.RegisterAssetTypeActions(Action);
		CreatedAssetTypeActions.Add(Action);
	}

	// Called when a property on the specified object is modified
	void OnPropertyChanged(UObject* ObjectBeingModified, FPropertyChangedEvent& PropertyChangedEvent)
	{
		if (UPaperSpriteAtlas* Atlas = Cast<UPaperSpriteAtlas>(ObjectBeingModified))
		{
			FPaperAtlasGenerator::HandleAssetChangedEvent(Atlas);
		}
	}

	void RegisterSettings()
	{
		if (ISettingsModule* SettingsModule = ISettingsModule::Get())
		{
			SettingsModule->RegisterSettings("Project", "Plugins", "Paper2D",
				LOCTEXT("RuntimeSettingsName", "Paper 2D"),
				LOCTEXT("RuntimeSettingsDescription", "Configure the Paper 2D plugin"),
				GetMutableDefault<UPaperRuntimeSettings>()
				);
		}
	}

	void UnregisterSettings()
	{
		if (ISettingsModule* SettingsModule = ISettingsModule::Get())
		{
			SettingsModule->UnregisterSettings("Project", "Plugins", "Paper2D");
		}
	}

	void RegisterIntroTutorials()
	{
		if (!IsRunningCommandlet())
		{
			//@TODO: PAPER2D: Remove the _Preview suffix on the config keys once the final doc is in place
			// (this is so that people who dismiss the early warning message still get the final intro doc later on)
			IIntroTutorials::Get().RegisterTutorialForAssetEditor(
				UPaperSprite::StaticClass(),
				TEXT("Shared/Tutorials/InPaperSpriteEditorTutorial"),
				TEXT("SeenPaperSpriteEditorWelcome_Preview"),
				FString("19D7EA18-629B-4A86-BD19-ED2B3BE53600"));
			IIntroTutorials::Get().RegisterTutorialForAssetEditor(
				UPaperFlipbook::StaticClass(),
				TEXT("Shared/Tutorials/InPaperFlipbookEditorTutorial"),
				TEXT("SeenPaperFlipbookEditorWelcome_Preview"),
				FString("B24214C1-E17A-4F95-BE18-2ED8BFCEC008"));
		}
	}

	void UnregisterIntroTutorials()
	{
		if (IIntroTutorials::IsAvailable())
		{
			IIntroTutorials::Get().UnregisterTutorialForAssetEditor(UPaperSprite::StaticClass());
			IIntroTutorials::Get().UnregisterTutorialForAssetEditor(UPaperFlipbook::StaticClass());
		}
	}
};

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_MODULE(FPaper2DEditor, Paper2DEditor);

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE