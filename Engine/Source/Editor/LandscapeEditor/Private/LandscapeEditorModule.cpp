// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "LandscapeEditorPrivatePCH.h"
#include "LandscapeEditorCommands.h"
#include "LandscapeEdMode.h"
#include "LandscapeEditorClasses.h"
#include "LandscapeEditor.generated.inl"

#include "LandscapeEditorDetails.h"
#include "LandscapeEditorDetailCustomizations.h"
#include "PropertyEditorModule.h"
#include "PropertyEditorDelegates.h"

#include "Editor/LevelEditor/Public/LevelEditor.h"
#include "LandscapeRender.h"

#define LOCTEXT_NAMESPACE "LandscapeEditor"

class FLandscapeEditorModule : public ILandscapeEditorModule
{
private:
	TSharedPtr<FEdModeLandscape> EdModeLandscape;
public:
	
	/**
	 * Called right after the module's DLL has been loaded and the module object has been created
	 */
	virtual void StartupModule() OVERRIDE
	{
		FLandscapeEditorCommands::Register();

		// register the editor mode
		TSharedRef<FEdModeLandscape> NewEditorMode = MakeShareable(new FEdModeLandscape);
		GEditorModeTools().RegisterMode(NewEditorMode);
		EdModeLandscape = NewEditorMode;

		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.RegisterCustomPropertyLayout(ULandscapeEditorObject::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FLandscapeEditorDetails::MakeInstance));
		PropertyModule.RegisterStructPropertyLayout("GizmoImportLayer", FOnGetStructCustomizationInstance::CreateStatic(&FLandscapeEditorStructCustomization_FGizmoImportLayer::MakeInstance));
		PropertyModule.RegisterStructPropertyLayout("LandscapeImportLayer", FOnGetStructCustomizationInstance::CreateStatic(&FLandscapeEditorStructCustomization_FLandscapeImportLayer::MakeInstance));

		TSharedRef<FUICommandList> CommandList = MakeShareable(new FUICommandList);
		const FLandscapeEditorCommands& LandscapeActions = FLandscapeEditorCommands::Get();
		CommandList->MapAction(LandscapeActions.ViewModeNormal,       FExecuteAction::CreateStatic(&ChangeLandscapeViewMode, ELandscapeViewMode::Normal),       FCanExecuteAction(), FIsActionChecked::CreateStatic(&IsLandscapeViewModeSelected, ELandscapeViewMode::Normal));
		CommandList->MapAction(LandscapeActions.ViewModeLOD,          FExecuteAction::CreateStatic(&ChangeLandscapeViewMode, ELandscapeViewMode::LOD),          FCanExecuteAction(), FIsActionChecked::CreateStatic(&IsLandscapeViewModeSelected, ELandscapeViewMode::LOD));
		CommandList->MapAction(LandscapeActions.ViewModeLayerDensity, FExecuteAction::CreateStatic(&ChangeLandscapeViewMode, ELandscapeViewMode::LayerDensity), FCanExecuteAction(), FIsActionChecked::CreateStatic(&IsLandscapeViewModeSelected, ELandscapeViewMode::LayerDensity));
		CommandList->MapAction(LandscapeActions.ViewModeLayerDebug,   FExecuteAction::CreateStatic(&ChangeLandscapeViewMode, ELandscapeViewMode::DebugLayer),   FCanExecuteAction(), FIsActionChecked::CreateStatic(&IsLandscapeViewModeSelected, ELandscapeViewMode::DebugLayer));

		TSharedRef<FExtender> ViewportMenuExtender = MakeShareable(new FExtender);
		ViewportMenuExtender->AddMenuExtension("LevelViewportExposure", EExtensionHook::Before, CommandList, FMenuExtensionDelegate::CreateStatic(&ConstructLandscapeViewportMenu));
		FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(ViewportMenuExtender);

		// Actor Factories
		UActorFactoryLandscape* LandscapeActorFactory = ConstructObject<UActorFactoryLandscape>(UActorFactoryLandscape::StaticClass());
		LandscapeActorFactory->NewActorClass = ALandscape::StaticClass();
		GEditor->ActorFactories.Add(LandscapeActorFactory);

		UActorFactoryLandscape* LandscapeProxyActorFactory = ConstructObject<UActorFactoryLandscape>(UActorFactoryLandscape::StaticClass());
		LandscapeProxyActorFactory->NewActorClass = ALandscapeProxy::StaticClass();
		GEditor->ActorFactories.Add(LandscapeProxyActorFactory);
	}

	/**
	 * Called before the module is unloaded, right before the module object is destroyed.
	 */
	virtual void ShutdownModule() OVERRIDE
	{
		FLandscapeEditorCommands::Unregister();

		// unregister the editor mode
		GEditorModeTools().UnregisterMode(EdModeLandscape.ToSharedRef());
		EdModeLandscape = NULL;
	}

	static void ConstructLandscapeViewportMenu(FMenuBuilder& MenuBuilder)
	{
		MenuBuilder.BeginSection("LevelViewportLandscapeVisualizers", LOCTEXT("LandscapeViewModeSectionName", "Landscape Visualizers") );

		//struct Local
		//{
		//	static void BuildLandscapeVisualizersMenu(FMenuBuilder& MenuBuilder)
		//	{
				const FLandscapeEditorCommands& LandscapeActions = FLandscapeEditorCommands::Get();

		//		MenuBuilder.BeginSection("LandscapeVisualizers", LOCTEXT("LandscapeHeader", "Landscape Visualizers"));
		//		{
					MenuBuilder.AddMenuEntry(LandscapeActions.ViewModeNormal,       NAME_None, LOCTEXT("LandscapeViewModeNormal", "Normal"));
					MenuBuilder.AddMenuEntry(LandscapeActions.ViewModeLOD,          NAME_None, LOCTEXT("LandscapeViewModeLOD", "LOD"));
					MenuBuilder.AddMenuEntry(LandscapeActions.ViewModeLayerDensity, NAME_None, LOCTEXT("LandscapeViewModeLayerDensity", "Layer Density"));
					MenuBuilder.AddMenuEntry(LandscapeActions.ViewModeLayerDebug,   NAME_None, LOCTEXT("LandscapeViewModeLayerDebug", "Layer Debug"));
		//		}
		//		MenuBuilder.EndSection();
		//	}
		//};
		//MenuBuilder.AddSubMenu(LOCTEXT("LandscapeSubMenu", "Landscape Visualizers"), LOCTEXT("LandscapeSubMenu_ToolTip", "Select a Landscape visualiser"), FNewMenuDelegate::CreateStatic(&Local::BuildLandscapeVisualizersMenu));

		MenuBuilder.EndSection();
	}

	static void ChangeLandscapeViewMode(ELandscapeViewMode::Type ViewMode)
	{
		GLandscapeViewMode = ViewMode;
	}

	static bool IsLandscapeViewModeSelected(ELandscapeViewMode::Type ViewMode)
	{
		return GLandscapeViewMode == ViewMode;
	}
};

IMPLEMENT_MODULE( FLandscapeEditorModule, LandscapeEditor );

#undef LOCTEXT_NAMESPACE
