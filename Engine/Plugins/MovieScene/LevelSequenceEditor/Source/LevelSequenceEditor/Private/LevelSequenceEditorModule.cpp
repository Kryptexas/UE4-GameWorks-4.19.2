// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "LevelSequenceEditorPCH.h"
#include "LevelEditor.h"
#include "LevelSequenceActor.h"
#include "LevelSequenceEditorStyle.h"
#include "ModuleInterface.h"
#include "PropertyEditorModule.h"
#include "CinematicLevelViewportLayout.h"
#include "IPlacementModeModule.h"
#include "Camera/CineCameraActor.h"

#define LOCTEXT_NAMESPACE "LevelSequenceEditor"

TSharedPtr<FLevelSequenceEditorStyle> FLevelSequenceEditorStyle::Singleton;

class FLevelSequenceExtensionCommands
	: public TCommands<FLevelSequenceExtensionCommands>
{
public:

	/** Default constructor. */
	FLevelSequenceExtensionCommands()
		: TCommands("LevelSequenceEditor", LOCTEXT("ExtensionDescription", "Extension commands specific to the level sequence editor"), NAME_None, "LevelSequenceEditorStyle")
	{ }

	/** Initialize commands */
	virtual void RegisterCommands() override
	{
		UI_COMMAND(CreateNewLevelSequenceInLevel, "Add Level Sequence", "Create a new level sequence asset, and place an instance of it in this level", EUserInterfaceActionType::Button, FInputChord());
	}

	TSharedPtr<FUICommandInfo> CreateNewLevelSequenceInLevel;
};


/**
 * Implements the LevelSequenceEditor module.
 */
class FLevelSequenceEditorModule
	: public IModuleInterface
{
public:

	// IModuleInterface interface

	virtual void StartupModule() override
	{
		FLevelSequenceEditorStyle::Get();

		RegisterAssetTools();
		RegisterCustomizations();
		RegisterMenuExtensions();
		RegisterLevelEditorExtensions();
		RegisterPlacementModeExtensions();
	}
	
	virtual void ShutdownModule() override
	{
		UnregisterAssetTools();
		UnregisterCustomizations();
		UnregisterMenuExtensions();
		UnregisterLevelEditorExtensions();
		UnregisterPlacementModeExtensions();
	}

protected:

	/** Registers level editor extensions. */
	void RegisterLevelEditorExtensions()
	{
		FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");

		FCustomViewportLayoutDefinition TwoPaneDefinition = FCustomViewportLayoutDefinition::FromType<FCinematicLevelViewportLayout_OnePane>();
		TwoPaneDefinition.DisplayName = LOCTEXT("OnePaneCinematicLayoutName", "Cinematic");
		TwoPaneDefinition.Description = LOCTEXT("OnePaneCinematicLayoutDesc", "A viewport layout tailored to cinematic preview");
		TwoPaneDefinition.Icon = FSlateIcon("LevelSequenceEditorStyle", "LevelSequenceEditor.OnePaneCinematicViewportLayout");
		LevelEditorModule.RegisterCustomViewportLayout("OnePaneCinematic", TwoPaneDefinition);

		FCustomViewportLayoutDefinition OnePaneDefinition = FCustomViewportLayoutDefinition::FromType<FCinematicLevelViewportLayout_TwoPane>();
		OnePaneDefinition.DisplayName = LOCTEXT("TwoPaneCinematicLayoutName", "Two Pane Cinematic");
		OnePaneDefinition.Description = LOCTEXT("TwoPaneCinematicLayoutDesc", "A viewport layout comprising an edit viewport, and a cinematic preview viewport");
		OnePaneDefinition.Icon = FSlateIcon("LevelSequenceEditorStyle", "LevelSequenceEditor.TwoPaneCinematicViewportLayout");
		LevelEditorModule.RegisterCustomViewportLayout("TwoPaneCinematic", OnePaneDefinition);
	}

	/** Registers asset tool actions. */
	void RegisterAssetTools()
	{
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		RegisterAssetTypeAction(AssetTools, MakeShareable(new FLevelSequenceActions(FLevelSequenceEditorStyle::Get())));
	}

	/** Registers placement mode extensions. */
	void RegisterPlacementModeExtensions()
	{
		FPlacementCategoryInfo Info(
			LOCTEXT("CinematicCategoryName", "Cinematic"),
			"Cinematic",
			TEXT("PMCinematic"),
			25);
		IPlacementModeModule::Get().RegisterPlacementCategory(Info);
		IPlacementModeModule::Get().RegisterPlaceableItem(Info.UniqueHandle, MakeShareable( new FPlaceableItem(nullptr, FAssetData(ACineCameraActor::StaticClass())) ));
	}

	/**
	 * Registers a single asset type action.
	 *
	 * @param AssetTools The asset tools object to register with.
	 * @param Action The asset type action to register.
	 */
	void RegisterAssetTypeAction(IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action)
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

	/** Unregisters level editor extensions. */
	void UnregisterLevelEditorExtensions()
	{
		FLevelEditorModule* LevelEditorModule = FModuleManager::GetModulePtr<FLevelEditorModule>("LevelEditor");
		if (LevelEditorModule)
		{
			LevelEditorModule->UnRegisterCustomViewportLayout("Cinematic");
		}
	}

	/** Unregisters placement mode extensions. */
	void UnregisterPlacementModeExtensions()
	{
		if (IPlacementModeModule::IsAvailable())
		{
			IPlacementModeModule::Get().UnregisterPlacementCategory("Cinematic");
		}
	}

protected:

	/** Register details view customizations. */
	void RegisterCustomizations()
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.RegisterCustomPropertyTypeLayout(FLevelSequencePlaybackSettings::StaticStruct()->GetFName(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FLevelSequencePlaybackSettingsCustomization::MakeInstance));
	}

	/** Unregister details view customizations. */
	void UnregisterCustomizations()
	{
		FPropertyEditorModule* PropertyModule = FModuleManager::GetModulePtr<FPropertyEditorModule>("PropertyEditor");
		if (PropertyModule)
		{
			PropertyModule->UnregisterCustomPropertyTypeLayout(FLevelSequencePlaybackSettings::StaticStruct()->GetFName());
		}
	}

protected:

	/** Register menu extensions for the level editor toolbar. */
	void RegisterMenuExtensions()
	{
		FLevelSequenceExtensionCommands::Register();

		CommandList = MakeShareable(new FUICommandList);
		CommandList->MapAction(FLevelSequenceExtensionCommands::Get().CreateNewLevelSequenceInLevel,
			FExecuteAction::CreateStatic(&FLevelSequenceEditorModule::OnCreateActorInLevel)
		);

		// Create and register the level editor toolbar menu extension
		CinematicsMenuExtender = MakeShareable(new FExtender);
		CinematicsMenuExtender->AddMenuExtension("LevelEditorNewMatinee", EExtensionHook::First, CommandList, FMenuExtensionDelegate::CreateStatic([](FMenuBuilder& MenuBuilder){
			MenuBuilder.AddMenuEntry(FLevelSequenceExtensionCommands::Get().CreateNewLevelSequenceInLevel);
		}));

		FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
		LevelEditorModule.GetAllLevelEditorToolbarCinematicsMenuExtenders().Add(CinematicsMenuExtender);
	}

	/** Unregisters menu extensions for the level editor toolbar. */
	void UnregisterMenuExtensions()
	{
		if (FLevelEditorModule* LevelEditorModule = FModuleManager::GetModulePtr<FLevelEditorModule>("LevelEditor"))
		{
			LevelEditorModule->GetAllLevelEditorToolbarCinematicsMenuExtenders().Remove(CinematicsMenuExtender);
		}

		CinematicsMenuExtender = nullptr;
		CommandList = nullptr;

		FLevelSequenceExtensionCommands::Unregister();
	}

	/** Callback for creating a new level sequence asset in the level. */
	static void OnCreateActorInLevel()
	{
		// Create a new level sequence
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();

		UObject* NewAsset = nullptr;

		// Attempt to create a new asset
		for (TObjectIterator<UClass> It ; It ; ++It)
		{
			UClass* CurrentClass = *It;
			if (CurrentClass->IsChildOf(UFactory::StaticClass()) && !(CurrentClass->HasAnyClassFlags(CLASS_Abstract)))
			{
				UFactory* Factory = Cast<UFactory>(CurrentClass->GetDefaultObject());
				if (Factory->CanCreateNew() && Factory->ImportPriority >= 0 && Factory->SupportedClass == ULevelSequence::StaticClass())
				{
					NewAsset = AssetTools.CreateAsset(ULevelSequence::StaticClass(), Factory);
					break;
				}
			}
		}

		if (!NewAsset)
		{
			return;
		}

		// Spawn a  actor at the origin, and either move infront of the camera or focus camera on it (depending on the viewport) and open for edit
		UActorFactory* ActorFactory = GEditor->FindActorFactoryForActorClass(ALevelSequenceActor::StaticClass());
		if (!ensure(ActorFactory))
		{
			return;
		}

		ALevelSequenceActor* NewActor = CastChecked<ALevelSequenceActor>(GEditor->UseActorFactory(ActorFactory, FAssetData(NewAsset), &FTransform::Identity));
		if (GCurrentLevelEditingViewportClient != nullptr && GCurrentLevelEditingViewportClient->IsPerspective())
		{
			GEditor->MoveActorInFrontOfCamera(*NewActor, GCurrentLevelEditingViewportClient->GetViewLocation(), GCurrentLevelEditingViewportClient->GetViewRotation().Vector());
		}
		else
		{
			GEditor->MoveViewportCamerasToActor(*NewActor, false);
		}

		FAssetEditorManager::Get().OpenEditorForAsset(NewAsset);
	}

private:

	/** The collection of registered asset type actions. */
	TArray<TSharedRef<IAssetTypeActions>> RegisteredAssetTypeActions;

	/** Extender for the cinematics menu */
	TSharedPtr<FExtender> CinematicsMenuExtender;

	TSharedPtr<FUICommandList> CommandList;
};


IMPLEMENT_MODULE(FLevelSequenceEditorModule, LevelSequenceEditor);

#undef LOCTEXT_NAMESPACE
