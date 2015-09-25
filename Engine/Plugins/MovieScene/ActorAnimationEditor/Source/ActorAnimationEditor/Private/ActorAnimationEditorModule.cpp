// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "ActorAnimationEditorPrivatePCH.h"
#include "ActorAnimationEditorStyle.h"
#include "ModuleInterface.h"
#include "LevelEditor.h"
#include "MovieSceneActor.h"

#define LOCTEXT_NAMESPACE "ActorAnimationEditor"

class FActorAnimationExtensionCommands : public TCommands<FActorAnimationExtensionCommands>
{
public:
	FActorAnimationExtensionCommands()
		: TCommands("ActorAnimationEditor", LOCTEXT("ExtensionDescription", "Extension commands specific to the actor animation editor"), NAME_None, "ActorAnimationEditorStyle")
	{}

	TSharedPtr<FUICommandInfo> CreateNewActorAnimationInLevel;

	/** Initialize commands */
	virtual void RegisterCommands() override
	{
		UI_COMMAND(CreateNewActorAnimationInLevel, "Add New Actor Animation", "Create a new actor animation asset, and place an instance of it in this level", EUserInterfaceActionType::Button, FInputChord());
	}
};

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
		Style = MakeShareable(new FActorAnimationEditorStyle());

		RegisterAssetTools();
		RegisterMenuExtensions();
	}
	
	virtual void ShutdownModule() override
	{
		UnregisterAssetTools();
		UnregisterMenuExtensions();
	}

protected:

	/** Registers asset tool actions. */
	void RegisterAssetTools()
	{
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

		RegisterAssetTypeAction(AssetTools, MakeShareable(new FActorAnimationActions(Style.ToSharedRef())));
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

protected:

	/**
	 * Registers menu extensions for the level editor toolbar
	 */
	void RegisterMenuExtensions()
	{
		FActorAnimationExtensionCommands::Register();

		CommandList = MakeShareable(new FUICommandList);

		CommandList->MapAction(FActorAnimationExtensionCommands::Get().CreateNewActorAnimationInLevel,
			FExecuteAction::CreateStatic(&FActorAnimationEditorModule::OnCreateActorInLevel)
			);

		// Create and register the level editor toolbar menu extension
		CinematicsMenuExtender = MakeShareable(new FExtender);
		CinematicsMenuExtender->AddMenuExtension("LevelEditorNewMatinee", EExtensionHook::First, CommandList, FMenuExtensionDelegate::CreateStatic([](FMenuBuilder& MenuBuilder){
			MenuBuilder.AddMenuEntry(FActorAnimationExtensionCommands::Get().CreateNewActorAnimationInLevel, NAME_None, TAttribute<FText>(), TAttribute<FText>(), FSlateIcon(FEditorStyle::GetStyleSetName(), "ClassIcon.ActorAnimation") );
		}));

		FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
		LevelEditorModule.GetAllLevelEditorToolbarCinematicsMenuExtenders().Add(CinematicsMenuExtender);
	}

	/**
	 * Unregisters menu extensions for the level editor toolbar
	 */
	void UnregisterMenuExtensions()
	{
		if (FLevelEditorModule* LevelEditorModule = FModuleManager::GetModulePtr<FLevelEditorModule>("LevelEditor"))
		{
			LevelEditorModule->GetAllLevelEditorToolbarCinematicsMenuExtenders().Remove(CinematicsMenuExtender);
		}
		CinematicsMenuExtender = nullptr;
		CommandList = nullptr;

		FActorAnimationExtensionCommands::Unregister();
	}

	/**
	 * Callback for creating a new actor animation asset in the level
	 */
	static void OnCreateActorInLevel()
	{
		// Create a new actor animation
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();

		UObject* NewAsset = nullptr;

		// Attempt to create a new asset
		for (TObjectIterator<UClass> It ; It ; ++It)
		{
			UClass* CurrentClass = *It;
			if (CurrentClass->IsChildOf(UFactory::StaticClass()) && !(CurrentClass->HasAnyClassFlags(CLASS_Abstract)))
			{
				UFactory* Factory = Cast<UFactory>(CurrentClass->GetDefaultObject());
				if (Factory->CanCreateNew() && Factory->ImportPriority >= 0 && Factory->SupportedClass == UActorAnimation::StaticClass())
				{
					NewAsset = AssetTools.CreateAsset(UActorAnimation::StaticClass(), Factory);
					break;
				}
			}
		}

		if (!NewAsset)
		{
			return;
		}

		// Spawn a  actor at the origin, and either move infront of the camera or focus camera on it (depending on the viewport) and open for edit
		UActorFactory* ActorFactory = GEditor->FindActorFactoryForActorClass( AMovieSceneActor::StaticClass() );
		if (!ensure(ActorFactory))
		{
			return;
		}

		AMovieSceneActor* NewActor = CastChecked<AMovieSceneActor>(GEditor->UseActorFactory(ActorFactory, FAssetData(NewAsset), &FTransform::Identity));
		if( GCurrentLevelEditingViewportClient->IsPerspective() )
		{
			GEditor->MoveActorInFrontOfCamera( *NewActor, GCurrentLevelEditingViewportClient->GetViewLocation(), GCurrentLevelEditingViewportClient->GetViewRotation().Vector() );
		}
		else
		{
			GEditor->MoveViewportCamerasToActor( *NewActor, false );
		}

		FAssetEditorManager::Get().OpenEditorForAsset(NewAsset);
	}

private:

	/** The collection of registered asset type actions. */
	TArray<TSharedRef<IAssetTypeActions>> RegisteredAssetTypeActions;

	/** Holds the plug-ins style set. */
	TSharedPtr<ISlateStyle> Style;

	/** Extender for the cinematics menu */
	TSharedPtr<FExtender> CinematicsMenuExtender;

	TSharedPtr<FUICommandList> CommandList;
};


IMPLEMENT_MODULE(FActorAnimationEditorModule, ActorAnimationEditor);

#undef LOCTEXT_NAMESPACE