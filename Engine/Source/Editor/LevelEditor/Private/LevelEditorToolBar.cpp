// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "LevelEditor.h"
#include "LevelEditorToolBar.h"
#include "LevelEditorActions.h"
#include "Editor/UnrealEd/Public/SourceCodeNavigation.h"
#include "Editor/UnrealEd/Public/Kismet2/DebuggerCommands.h"
#include "Editor/SceneOutliner/Public/SceneOutlinerModule.h"
#include "DelegateFilter.h"
#include "EditorViewportCommands.h"
#include "SScalabilitySettings.h"
#include "Editor/LevelEditor/Private/SLevelEditor.h"
#include "AssetData.h"
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"
#include "EngineBuildSettings.h"
#include "Matinee/MatineeActor.h"
#include "Engine/LevelScriptBlueprint.h"
#include "Settings.h"
#include "DesktopPlatformModule.h"
#include "Editor/ClassViewer/Private/SClassViewer.h"
#include "Editor/ClassViewer/Public/ClassViewerFilter.h"
#include "KismetEditorUtilities.h"

namespace LevelEditorActionHelpers
{
	/** Filters out any classes for the Class Picker when creating or selecting classes in the Blueprints dropdown */
	class FBlueprintParentFilter_MapModeSettings : public IClassViewerFilter
	{
	public:
		/** Classes to not allow any children of into the Class Viewer/Picker. */
		TSet< const UClass* > AllowedChildrenOfClasses;

		virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs ) override
		{
			return InFilterFuncs->IfInChildOfClassesSet(AllowedChildrenOfClasses, InClass) == EFilterReturn::Passed;
		}

		virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef< const IUnloadedBlueprintData > InUnloadedClassData, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) override
		{
			return InFilterFuncs->IfInChildOfClassesSet(AllowedChildrenOfClasses, InUnloadedClassData) == EFilterReturn::Passed;
		}
	};

	/**
	 * Builds a sub-menu for selecting a class
	 *
	 * @param InMenuBuilder		Object to append menu items/widgets to
	 * @param InRootClass		The root class to filter the Class Viewer by to only show children of
	 * @param InOnClassPicked	Callback delegate to fire when a class is picked
	 */
	void GetSelectSettingsClassSubMenu(FMenuBuilder& InMenuBuilder, UClass* InRootClass, FOnClassPicked InOnClassPicked)
	{
		FClassViewerInitializationOptions Options;
		Options.Mode = EClassViewerMode::ClassPicker;
		Options.DisplayMode = EClassViewerDisplayMode::ListView;
		Options.bShowObjectRootClass = true;
		Options.bShowNoneOption = true;

		// Only want blueprint actor base classes.
		Options.bIsBlueprintBaseOnly = true;

		// This will allow unloaded blueprints to be shown.
		Options.bShowUnloadedBlueprints = true;

		TSharedPtr< FBlueprintParentFilter_MapModeSettings > Filter = MakeShareable(new FBlueprintParentFilter_MapModeSettings);
		Filter->AllowedChildrenOfClasses.Add(InRootClass);
		Options.ClassFilter = Filter;

		FText RootClassName = FText::FromString(InRootClass->GetName());
		auto ClassViewer = StaticCastSharedRef<SClassViewer>(FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer").CreateClassViewer(Options, InOnClassPicked));
		InMenuBuilder.BeginSection(NAME_None, FText::Format( NSLOCTEXT("LevelToolBarViewMenu", "SelectGameModeLabel", "Select {RootClass} class"), RootClassName ));
		InMenuBuilder.AddWidget(ClassViewer, FText::GetEmpty(), true);
		InMenuBuilder.EndSection();
	}

	/**
	 * Builds a sub-menu for creating a class
	 *
	 * @param InMenuBuilder		Object to append menu items/widgets to
	 * @param InRootClass		The root class to filter the Class Viewer by to only show children of
	 * @param InOnClassPicked	Callback delegate to fire when a class is picked
	 */
	void GetCreateSettingsClassSubMenu(FMenuBuilder& InMenuBuilder, UClass* InRootClass, FOnClassPicked InOnClassPicked)
	{
		FClassViewerInitializationOptions Options;
		Options.Mode = EClassViewerMode::ClassPicker;
		Options.DisplayMode = EClassViewerDisplayMode::ListView;
		Options.bShowObjectRootClass = true;

		// Only want blueprint actor base classes.
		Options.bIsBlueprintBaseOnly = true;

		// This will allow unloaded blueprints to be shown.
		Options.bShowUnloadedBlueprints = true;

		TSharedPtr< FBlueprintParentFilter_MapModeSettings > Filter = MakeShareable(new FBlueprintParentFilter_MapModeSettings);
		Filter->AllowedChildrenOfClasses.Add(InRootClass);
		Options.ClassFilter = Filter;

		FText RootClassName = FText::FromString(InRootClass->GetName());
		auto ClassViewer = StaticCastSharedRef<SClassViewer>(FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer").CreateClassViewer(Options, InOnClassPicked));
		InMenuBuilder.BeginSection(NAME_None, FText::Format( NSLOCTEXT("LevelToolBarViewMenu", "CreateGameModeLabel", "Select {RootClass} parent class"), RootClassName ));
		InMenuBuilder.AddWidget(ClassViewer, FText::GetEmpty(), true);
		InMenuBuilder.EndSection();
	}

	/** Helper struct for passing all required data to the GetBlueprintSettingsSubMenu function */
	struct FBlueprintMenuSettings
	{
		/** The UI command for editing the Blueprint class associated with the menu */
		TSharedPtr< FUICommandInfo > EditCommand;

		/** Current class associated with the menu */
		UClass* CurrentClass;

		/** Root class that defines what class children can be set through the menu */
		UClass* RootClass;

		/** Callback when a class is picked, to assign the new class */
		FOnClassPicked OnSelectClassPicked;

		/** Callback when a class is picked, to create a new child class of and assign */
		FOnClassPicked OnCreateClassPicked;
	};

	/**
	 * A sub-menu for the Blueprints dropdown, facilitates all the sub-menu actions such as creating, editing, and selecting classes for the world settings game mode.
	 *
	 * @param InMenuBuilder		Object to append menu items/widgets to
	 * @param InSettingsData	All the data needed to create the menu actions
	 */
	void GetBlueprintSettingsSubMenu(FMenuBuilder& InMenuBuilder, FBlueprintMenuSettings InSettingsData)
	{
#define LOCTEXT_NAMESPACE "LevelToolBarViewMenu"

		FSlateIcon EditBPIcon(FEditorStyle::Get().GetStyleSetName(), TEXT("PropertyWindow.Button_Edit"));
		FSlateIcon NewBPIcon(FEditorStyle::Get().GetStyleSetName(), TEXT("PropertyWindow.Button_AddToArray"));
		FText RootClassName = FText::FromString(InSettingsData.RootClass->GetName());

		// If there is currently a valid GameMode Blueprint, offer to edit the Blueprint
		if(InSettingsData.CurrentClass && InSettingsData.CurrentClass->ClassGeneratedBy)
		{
			FText BlueprintName = FText::FromString(InSettingsData.CurrentClass->ClassGeneratedBy->GetName());
			InMenuBuilder.AddMenuEntry( InSettingsData.EditCommand, NAME_None, FText::Format( LOCTEXT("EditBlueprint", "Edit {Blueprint}"), BlueprintName), FText::Format( LOCTEXT("EditBlueprint_Tooltip", "Open the world's assigned {RootClass} blueprint"), RootClassName), EditBPIcon );
		}

		// Create a new GameMode, this is always available so the user can easily create a new one
		InMenuBuilder.AddSubMenu( LOCTEXT("CreateBlueprint", "Create..."), FText::Format( LOCTEXT("CreateClass_Tooltip", "Create a new {RootClass} based on a selected class and auto-assign it to the world"), RootClassName ), FNewMenuDelegate::CreateStatic( &LevelEditorActionHelpers::GetCreateSettingsClassSubMenu, InSettingsData.RootClass, InSettingsData.OnCreateClassPicked ), false, NewBPIcon );

		// Select a game mode, this is always available so the user can switch his selection
		InMenuBuilder.AddSubMenu( FText::Format( LOCTEXT("SelectGameModeClass", "Select {RootClass} Class"), RootClassName ), FText::Format( LOCTEXT("SelectGameModeClass_Tooltip", "Select a class to be the active {RootClass} in the world"), RootClassName ), FNewMenuDelegate::CreateStatic( &LevelEditorActionHelpers::GetSelectSettingsClassSubMenu, InSettingsData.RootClass, InSettingsData.OnSelectClassPicked ) );

#undef LOCTEXT_NAMESPACE
	}
}

/**
 * Static: Creates a widget for the level editor tool bar
 *
 * @return	New widget
 */
TSharedRef< SWidget > FLevelEditorToolBar::MakeLevelEditorToolBar( const TSharedRef<FUICommandList>& InCommandList, const TSharedRef<SLevelEditor> InLevelEditor )
{
#define LOCTEXT_NAMESPACE "LevelEditorToolBar"

	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	TSharedPtr<FExtender> Extenders = LevelEditorModule.GetToolBarExtensibilityManager()->GetAllExtenders();

	static const FName LevelEditorToolBarName = "LevelEditorToolBar";
	FToolBarBuilder ToolbarBuilder( InCommandList, FMultiBoxCustomization::AllowCustomization( LevelEditorToolBarName ), Extenders );

	ToolbarBuilder.BeginSection("File");
	{
		// Save All Levels
		ToolbarBuilder.AddToolBarButton( FLevelEditorCommands::Get().Save, NAME_None, TAttribute<FText>(), TAttribute<FText>(), FSlateIcon(FEditorStyle::GetStyleSetName(), "AssetEditor.SaveAsset") );
	}
	ToolbarBuilder.EndSection();

	ToolbarBuilder.BeginSection("Content");
	{
		ToolbarBuilder.AddToolBarButton( FLevelEditorCommands::Get().OpenContentBrowser, NAME_None, LOCTEXT( "ContentBrowser_Override", "Content" ), TAttribute<FText>(), TAttribute<FSlateIcon>(), "LevelToolbarContent" );
		if (FDesktopPlatformModule::Get()->CanOpenLauncher(true)) 
		{
			ToolbarBuilder.AddToolBarButton(FLevelEditorCommands::Get().OpenMarketplace, NAME_None, LOCTEXT("Marketplace_Override", "Marketplace"), TAttribute<FText>(), TAttribute<FSlateIcon>(), "LevelToolbarMarketplace");
		}
	}
	ToolbarBuilder.EndSection();

	ToolbarBuilder.BeginSection("Settings");
	{
		ToolbarBuilder.AddComboButton(
			FUIAction(),
			FOnGetContent::CreateStatic(&FLevelEditorToolBar::GenerateQuickSettingsMenu, InCommandList),
			LOCTEXT("QuickSettingsCombo", "Settings"),
			LOCTEXT("QuickSettingsCombo_ToolTip", "Project and Editor settings"),
			FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.GameSettings"),
			false,
			"LevelToolbarQuickSettings"
			);

	}
	ToolbarBuilder.EndSection();

	ToolbarBuilder.BeginSection( NAME_None );
	{
		ToolbarBuilder.AddComboButton(
			FUIAction(),
			FOnGetContent::CreateStatic( &FLevelEditorToolBar::GenerateOpenBlueprintMenuContent, InCommandList, TWeakPtr<SLevelEditor>( InLevelEditor ) ),
			LOCTEXT( "OpenBlueprint_Label", "Blueprints" ),
			LOCTEXT( "OpenBlueprint_ToolTip", "List of world Blueprints available to the user for editing or creation." ),
			FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.OpenLevelBlueprint")
			);

		ToolbarBuilder.AddComboButton(
			FUIAction(),
			FOnGetContent::CreateStatic( &FLevelEditorToolBar::GenerateMatineeMenuContent, InCommandList, TWeakPtr<SLevelEditor>( InLevelEditor ) ),
			LOCTEXT( "EditMatinee_Label", "Matinee" ),
			LOCTEXT( "EditMatinee_Tooltip", "Displays a list of Matinee objects to open in the Matinee Editor"),
			FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.EditMatinee") 
			);
	}
	ToolbarBuilder.EndSection();
	
	ToolbarBuilder.BeginSection("Compile");
	{
		// Build			
		ToolbarBuilder.AddToolBarButton( FLevelEditorCommands::Get().Build, NAME_None, LOCTEXT("BuildAll", "Build") );

		// Build menu drop down
		ToolbarBuilder.AddComboButton(
			FUIAction(),
			FOnGetContent::CreateStatic( &FLevelEditorToolBar::GenerateBuildMenuContent, InCommandList ),
			LOCTEXT( "BuildCombo_Label", "Build Options" ),
			LOCTEXT( "BuildComboToolTip", "Build options menu" ),
			FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Build"),
			true);

		// Only show the compile options on machines with the solution (assuming they can build it)
		if ( FSourceCodeNavigation::IsCompilerAvailable() && FLevelEditorActionCallbacks::CanShowSourceCodeActions() )
		{
			ToolbarBuilder.AddToolBarButton(
				FLevelEditorCommands::Get().RecompileGameCode,
				NAME_None,
				LOCTEXT( "CompileMenuButton", "Compile" ),
				FLevelEditorCommands::Get().RecompileGameCode->GetDescription(),
				FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Recompile")
				);

			ToolbarBuilder.AddComboButton(
				FUIAction(),
				FOnGetContent::CreateStatic( &FLevelEditorToolBar::GenerateCompileMenuContent, InCommandList ),
				LOCTEXT( "CompileCombo_Label", "Compile Options" ),
				LOCTEXT( "CompileMenuCombo_ToolTip", "Recompile options" ),
				FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Recompile"),
				true);
		}
	}
	ToolbarBuilder.EndSection();
	
	ToolbarBuilder.BeginSection("Game");
	{
		// Add the shared play-world commands that will be shown on the Kismet toolbar as well
		FPlayWorldCommands::BuildToolbar(ToolbarBuilder, true);
	}
	ToolbarBuilder.EndSection();

	// Create the tool bar!
	return
		SNew( SBorder )
		.Padding(0)
		.BorderImage( FEditorStyle::GetBrush("NoBorder") )
		.IsEnabled( FSlateApplication::Get().GetNormalExecutionAttribute() )
		[
			ToolbarBuilder.MakeWidget()
		];
#undef LOCTEXT_NAMESPACE
}



TSharedRef< SWidget > FLevelEditorToolBar::GenerateBuildMenuContent( TSharedRef<FUICommandList> InCommandList )
{
#define LOCTEXT_NAMESPACE "LevelToolBarBuildMenu"

	// Get all menu extenders for this context menu from the level editor module
	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>( TEXT("LevelEditor") );
	TArray<FLevelEditorModule::FLevelEditorMenuExtender> MenuExtenderDelegates = LevelEditorModule.GetAllLevelEditorToolbarBuildMenuExtenders();

	TArray<TSharedPtr<FExtender>> Extenders;
	for (int32 i = 0; i < MenuExtenderDelegates.Num(); ++i)
	{
		if (MenuExtenderDelegates[i].IsBound())
		{
			Extenders.Add(MenuExtenderDelegates[i].Execute(InCommandList));
		}
	}
	TSharedPtr<FExtender> MenuExtender = FExtender::Combine(Extenders);

	const bool bShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder MenuBuilder( bShouldCloseWindowAfterMenuSelection, InCommandList, MenuExtender );

	struct FLightingMenus
	{

		/** Generates a lighting quality sub-menu */
		static void MakeLightingQualityMenu( FMenuBuilder& InMenuBuilder )
		{
			InMenuBuilder.BeginSection("LevelEditorBuildLightingQuality", LOCTEXT( "LightingQualityHeading", "Quality Level" ) );
			{
				InMenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().LightingQuality_Production );
				InMenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().LightingQuality_High );
				InMenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().LightingQuality_Medium );
				InMenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().LightingQuality_Preview );
			}
			InMenuBuilder.EndSection();
		}

		/** Generates a lighting tools sub-menu */
		static void MakeLightingToolsMenu( FMenuBuilder& InMenuBuilder )
		{
			InMenuBuilder.BeginSection("LevelEditorBuildLightingTools", LOCTEXT( "LightingToolsHeading", "Light Environments" ) );
			{
				InMenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().LightingTools_ShowBounds );
				InMenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().LightingTools_ShowTraces );
				InMenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().LightingTools_ShowDirectOnly );
				InMenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().LightingTools_ShowIndirectOnly );
				InMenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().LightingTools_ShowIndirectSamples );
			}
			InMenuBuilder.EndSection();
		}

		/** Generates a lighting density sub-menu */
		static void MakeLightingDensityMenu( FMenuBuilder& InMenuBuilder )
		{
			InMenuBuilder.BeginSection("LevelEditorBuildLightingDensity", LOCTEXT( "LightingDensityHeading", "Density Rendering" ) );
			{
				TSharedRef<SWidget> Ideal =		SNew(SHorizontalBox)
												+SHorizontalBox::Slot()
												.Padding( FMargin( 27.0f, 0.0f, 0.0f, 0.0f ) )
												.FillWidth(1.0f)
												[
													SNew(SSpinBox<float>)
													.MinValue(0.f)
													.MaxValue(100.f)
													.Value(FLevelEditorActionCallbacks::GetLightingDensityIdeal())
													.OnValueChanged_Static(&FLevelEditorActionCallbacks::SetLightingDensityIdeal)
												];
				InMenuBuilder.AddWidget(Ideal, LOCTEXT("LightingDensity_Ideal","Ideal Density"));
				
				TSharedRef<SWidget> Maximum =	SNew(SHorizontalBox)
												+SHorizontalBox::Slot()
												.FillWidth(1.0f)
												[
													SNew(SSpinBox<float>)
													.MinValue(0.01f)
													.MaxValue(100.01f)
													.Value(FLevelEditorActionCallbacks::GetLightingDensityMaximum())
													.OnValueChanged_Static(&FLevelEditorActionCallbacks::SetLightingDensityMaximum)
												];
				InMenuBuilder.AddWidget(Maximum, LOCTEXT("LightingDensity_Maximum","Maximum Density"));

				TSharedRef<SWidget> ClrScale =	SNew(SHorizontalBox)
												+SHorizontalBox::Slot()
												.Padding( FMargin( 35.0f, 0.0f, 0.0f, 0.0f ) )
												.FillWidth(1.0f)
												[
													SNew(SSpinBox<float>)
													.MinValue(0.f)
													.MaxValue(10.f)
													.Value(FLevelEditorActionCallbacks::GetLightingDensityColorScale())
													.OnValueChanged_Static(&FLevelEditorActionCallbacks::SetLightingDensityColorScale)
												];
				InMenuBuilder.AddWidget(ClrScale, LOCTEXT("LightingDensity_ColorScale","Color Scale"));

				TSharedRef<SWidget> GrayScale =	SNew(SHorizontalBox)
												+SHorizontalBox::Slot()
												.Padding( FMargin( 11.0f, 0.0f, 0.0f, 0.0f ) )
												.FillWidth(1.0f)
												[
													SNew(SSpinBox<float>)
													.MinValue(0.f)
													.MaxValue(10.f)
													.Value(FLevelEditorActionCallbacks::GetLightingDensityGrayscaleScale())
													.OnValueChanged_Static(&FLevelEditorActionCallbacks::SetLightingDensityGrayscaleScale)
												];
				InMenuBuilder.AddWidget(GrayScale, LOCTEXT("LightingDensity_GrayscaleScale","Grayscale Scale"));

				InMenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().LightingDensity_RenderGrayscale );
			}
			InMenuBuilder.EndSection();
		}

		/** Generates a lighting resolution sub-menu */
		static void MakeLightingResolutionMenu( FMenuBuilder& InMenuBuilder )
		{
			InMenuBuilder.BeginSection("LevelEditorBuildLightingResolution1", LOCTEXT( "LightingResolutionHeading1", "Primitive Types" ) );
			{
				TSharedRef<SWidget> Meshes =	SNew(SHorizontalBox)
												+SHorizontalBox::Slot()
												.AutoWidth()
												[
													SNew( SCheckBox )
													.Style( FEditorStyle::Get(), "Menu.CheckBox" )
													.ToolTipText(LOCTEXT( "StaticMeshesToolTip", "Static Meshes will be adjusted if checked." ))
													.IsChecked_Static(&FLevelEditorActionCallbacks::IsLightingResolutionStaticMeshesChecked)
													.OnCheckStateChanged_Static(&FLevelEditorActionCallbacks::SetLightingResolutionStaticMeshes)
													.Content()
													[
														SNew( STextBlock )
														.Text( LOCTEXT("StaticMeshes", "Static Meshes") )
													]
												]
												+SHorizontalBox::Slot()
												.AutoWidth()
												.Padding( FMargin( 4.0f, 0.0f, 11.0f, 0.0f ) )
												[
													SNew(SSpinBox<float>)
													.MinValue(4.f)
													.MaxValue(4096.f)
													.ToolTipText(LOCTEXT( "LightingResolutionStaticMeshesMinToolTip", "The minimum lightmap resolution for static mesh adjustments. Anything outside of Min/Max range will not be touched when adjusting." ))
													.Value(FLevelEditorActionCallbacks::GetLightingResolutionMinSMs())
													.OnValueChanged_Static(&FLevelEditorActionCallbacks::SetLightingResolutionMinSMs)
												]
												+SHorizontalBox::Slot()
												.AutoWidth()
												[
													SNew(SSpinBox<float>)
													.MinValue(4.f)
													.MaxValue(4096.f)
													.ToolTipText(LOCTEXT( "LightingResolutionStaticMeshesMaxToolTip", "The maximum lightmap resolution for static mesh adjustments. Anything outside of Min/Max range will not be touched when adjusting." ))
													.Value(FLevelEditorActionCallbacks::GetLightingResolutionMaxSMs())
													.OnValueChanged_Static(&FLevelEditorActionCallbacks::SetLightingResolutionMaxSMs)
												];
				InMenuBuilder.AddWidget(Meshes, FText::GetEmpty(), true);
				
				TSharedRef<SWidget> BSPs =		SNew(SHorizontalBox)
												+SHorizontalBox::Slot()
												.AutoWidth()
												[
													SNew( SCheckBox )
													.Style(FEditorStyle::Get(), "Menu.CheckBox")
													.ToolTipText(LOCTEXT( "BSPSurfacesToolTip", "BSP Surfaces will be adjusted if checked." ))
													.IsChecked_Static(&FLevelEditorActionCallbacks::IsLightingResolutionBSPSurfacesChecked)
													.OnCheckStateChanged_Static(&FLevelEditorActionCallbacks::SetLightingResolutionBSPSurfaces)
													.Content()
													[
														SNew( STextBlock )
														.Text( LOCTEXT("BSPSurfaces", "BSP Surfaces") )
													]
												]
												+SHorizontalBox::Slot()
												.AutoWidth()
												.Padding( FMargin( 6.0f, 0.0f, 4.0f, 0.0f ) )
												[
													SNew(SSpinBox<float>)
													.MinValue(1.f)
													.MaxValue(63556.f)
													.ToolTipText(LOCTEXT( "LightingResolutionBSPsMinToolTip", "The minimum lightmap resolution of a BSP surface to adjust. When outside of the Min/Max range, the BSP surface will no be altered." ))
													.Value(FLevelEditorActionCallbacks::GetLightingResolutionMinBSPs())
													.OnValueChanged_Static(&FLevelEditorActionCallbacks::SetLightingResolutionMinBSPs)
												]
												+SHorizontalBox::Slot()
												.AutoWidth()
												[
													SNew(SSpinBox<float>)
													.MinValue(1.f)
													.MaxValue(63556.f)
													.ToolTipText(LOCTEXT( "LightingResolutionBSPsMaxToolTip", "The maximum lightmap resolution of a BSP surface to adjust. When outside of the Min/Max range, the BSP surface will no be altered." ))
													.Value(FLevelEditorActionCallbacks::GetLightingResolutionMaxBSPs())
													.OnValueChanged_Static(&FLevelEditorActionCallbacks::SetLightingResolutionMaxBSPs)
												];
				InMenuBuilder.AddWidget(BSPs, FText::GetEmpty(), true);
			}
			InMenuBuilder.EndSection(); //LevelEditorBuildLightingResolution1

			InMenuBuilder.BeginSection("LevelEditorBuildLightingResolution2", LOCTEXT( "LightingResolutionHeading2", "Select Options" ) );
			{
				InMenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().LightingResolution_CurrentLevel );
				InMenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().LightingResolution_SelectedLevels );
				InMenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().LightingResolution_AllLoadedLevels );
				InMenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().LightingResolution_SelectedObjectsOnly );
			}
			InMenuBuilder.EndSection();

			InMenuBuilder.BeginSection("LevelEditorBuildLightingResolution3", LOCTEXT( "LightingResolutionHeading3", "Ratio" ) );
			{
				TSharedRef<SWidget> Ratio =		SNew(SSpinBox<int32>)
												.MinValue(0)
												.MaxValue(400)
												.ToolTipText(LOCTEXT( "LightingResolutionRatioToolTip", "Ratio to apply (New Resolution = Ratio / 100.0f * CurrentResolution)." ))
												.Value(FLevelEditorActionCallbacks::GetLightingResolutionRatio())
												.OnEndSliderMovement_Static(&FLevelEditorActionCallbacks::SetLightingResolutionRatio)
												.OnValueCommitted_Static(&FLevelEditorActionCallbacks::SetLightingResolutionRatioCommit);
				InMenuBuilder.AddWidget(Ratio, LOCTEXT( "LightingResolutionRatio", "Ratio" ));
			}
			InMenuBuilder.EndSection();
		}

		/** Generates a lighting info dialogs sub-menu */
		static void MakeLightingInfoMenu( FMenuBuilder& InMenuBuilder )
		{
			InMenuBuilder.BeginSection("LevelEditorBuildLightingInfo", LOCTEXT( "LightingInfoHeading", "Lighting Info Dialogs" ) );
			{
				InMenuBuilder.AddSubMenu(
					LOCTEXT( "LightingToolsSubMenu", "Lighting Tools" ),
					LOCTEXT( "LightingToolsSubMenu_ToolTip", "Shows the Lighting Tools options." ),
					FNewMenuDelegate::CreateStatic( &FLightingMenus::MakeLightingToolsMenu ) );
					
				InMenuBuilder.AddSubMenu(
					LOCTEXT( "LightingDensityRenderingSubMenu", "LightMap Density Rendering Options" ),
					LOCTEXT( "LightingDensityRenderingSubMenu_ToolTip", "Shows the LightMap Density Rendering viewmode options." ),
					FNewMenuDelegate::CreateStatic( &FLightingMenus::MakeLightingDensityMenu ) );

				InMenuBuilder.AddSubMenu(
					LOCTEXT( "LightingResolutionAdjustmentSubMenu", "LightMap Resolution Adjustment" ),
					LOCTEXT( "LightingResolutionAdjustmentSubMenu_ToolTip", "Shows the LightMap Resolution Adjustment options." ),
					FNewMenuDelegate::CreateStatic( &FLightingMenus::MakeLightingResolutionMenu ) );

				InMenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().LightingStaticMeshInfo, NAME_None, LOCTEXT( "BuildLightingInfo_LightingStaticMeshInfo", "Lighting StaticMesh Info..." ) );
			}
			InMenuBuilder.EndSection();
		}
	};

	MenuBuilder.BeginSection("LevelEditorLighting", LOCTEXT( "LightingHeading", "Lighting" ) );
	{
		MenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().BuildLightingOnly, NAME_None, LOCTEXT( "BuildLightingOnlyHeading", "Build Lighting Only" ) );

		MenuBuilder.AddSubMenu(
			LOCTEXT( "LightingQualitySubMenu", "Lighting Quality" ),
			LOCTEXT( "LightingQualitySubMenu_ToolTip", "Allows you to select the quality level for precomputed lighting" ),
			FNewMenuDelegate::CreateStatic( &FLightingMenus::MakeLightingQualityMenu ) );

		MenuBuilder.AddSubMenu(
			LOCTEXT( "BuildLightingInfoSubMenu", "Lighting Info" ),
			LOCTEXT( "BuildLightingInfoSubMenu_ToolTip", "Access the lighting info dialogs" ),
			FNewMenuDelegate::CreateStatic( &FLightingMenus::MakeLightingInfoMenu ) );

		MenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().LightingBuildOptions_UseErrorColoring );
		MenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().LightingBuildOptions_ShowLightingStats );
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("LevelEditorReflections", LOCTEXT( "ReflectionHeading", "Reflections" ) );
	{
		MenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().BuildReflectionCapturesOnly );
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("LevelEditorVisibility", LOCTEXT( "VisibilityHeading", "Visibility" ) );
	{
		MenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().BuildLightingOnly_VisibilityOnly );
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("LevelEditorGeometry", LOCTEXT( "GeometryHeading", "Geometry" ) );
	{
		MenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().BuildGeometryOnly );
		MenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().BuildGeometryOnly_OnlyCurrentLevel );
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("LevelEditorNavigation", LOCTEXT( "NavigationHeading", "Navigation" ) );
	{
		MenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().BuildPathsOnly );
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("LevelEditorAutomation", LOCTEXT( "AutomationHeading", "Automation" ) );
	{
		MenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().BuildAndSubmitToSourceControl );
	}
	MenuBuilder.EndSection();

	// Map Check
	MenuBuilder.BeginSection("LevelEditorVerification", LOCTEXT( "VerificationHeading", "Verification" ) );
	{
		MenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().MapCheck, NAME_None, LOCTEXT("OpenMapCheck", "Map Check") );
	}
	MenuBuilder.EndSection();

#undef LOCTEXT_NAMESPACE

	return MenuBuilder.MakeWidget();
}

static void MakeMaterialQualityLevelMenu( FMenuBuilder& MenuBuilder )
{
	MenuBuilder.BeginSection("LevelEditorMaterialQualityLevel", NSLOCTEXT( "LevelToolBarViewMenu", "MaterialQualityLevelHeading", "Material Quality Level" ) );
	{
		MenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().MaterialQualityLevel_Low );
		MenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().MaterialQualityLevel_High );
	}
	MenuBuilder.EndSection();
}

static void MakeShaderModelPreviewMenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.BeginSection("LevelEditorShaderModelPreview", NSLOCTEXT("LevelToolBarViewMenu", "FeatureLevelPreviewHeading", "Feature Level Preview"));
	{
		for (int32 i = GMaxRHIFeatureLevel; i >= 0; --i)
		{
			if (i != ERHIFeatureLevel::SM3)
			{
				MenuBuilder.AddMenuEntry(FLevelEditorCommands::Get().FeatureLevelPreview[i]);
			}
		}
	}
	MenuBuilder.EndSection();
}

static void MakeScalabilityMenu( FMenuBuilder& MenuBuilder )
{
	MenuBuilder.AddWidget(SNew(SScalabilitySettings), FText(), true);
}

static void MakePreviewSettingsMenu( FMenuBuilder& MenuBuilder )
{
#define LOCTEXT_NAMESPACE "LevelToolBarViewMenu"

	MenuBuilder.BeginSection("LevelEditorPreview", LOCTEXT("PreviewHeading", "Previewing"));
	{
		MenuBuilder.AddMenuEntry(FLevelEditorCommands::Get().DrawBrushMarkerPolys);
		MenuBuilder.AddMenuEntry(FLevelEditorCommands::Get().OnlyLoadVisibleInPIE);
		MenuBuilder.AddMenuEntry(FLevelEditorCommands::Get().ToggleParticleSystemLOD);
		MenuBuilder.AddMenuEntry(FLevelEditorCommands::Get().ToggleParticleSystemHelpers);
		MenuBuilder.AddMenuEntry(FLevelEditorCommands::Get().ToggleFreezeParticleSimulation);
		MenuBuilder.AddMenuEntry(FLevelEditorCommands::Get().ToggleLODViewLocking);
		MenuBuilder.AddMenuEntry(FLevelEditorCommands::Get().LevelStreamingVolumePrevis);
	}
	MenuBuilder.EndSection();
#undef LOCTEXT_NAMESPACE
}

TSharedRef< SWidget > FLevelEditorToolBar::GenerateQuickSettingsMenu( TSharedRef<FUICommandList> InCommandList )
{
#define LOCTEXT_NAMESPACE "LevelToolBarViewMenu"

	// Get all menu extenders for this context menu from the level editor module
	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>( TEXT("LevelEditor") );
	TArray<FLevelEditorModule::FLevelEditorMenuExtender> MenuExtenderDelegates = LevelEditorModule.GetAllLevelEditorToolbarViewMenuExtenders();

	TArray<TSharedPtr<FExtender>> Extenders;
	for (int32 i = 0; i < MenuExtenderDelegates.Num(); ++i)
	{
		if (MenuExtenderDelegates[i].IsBound())
		{
			Extenders.Add(MenuExtenderDelegates[i].Execute(InCommandList));
		}
	}
	TSharedPtr<FExtender> MenuExtender = FExtender::Combine(Extenders);

	const bool bShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder MenuBuilder( bShouldCloseWindowAfterMenuSelection, InCommandList, MenuExtender );

	struct Local
	{
		static void OpenSettings(FName ContainerName, FName CategoryName, FName SectionName)
		{
			FModuleManager::LoadModuleChecked<ISettingsModule>("Settings").ShowViewer(ContainerName, CategoryName, SectionName);
		}
	};

	MenuBuilder.BeginSection("ProjectSettingsSection", LOCTEXT("ProjectSettings","Game Specific Settings") );
	{

		MenuBuilder.AddMenuEntry(FLevelEditorCommands::Get().WorldProperties);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("ProjectSettingsMenuLabel", "Project Settings..."),
			LOCTEXT("ProjectSettingsMenuToolTip", "Change the settings of the currently loaded project"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateStatic(&Local::OpenSettings, FName("Project"), FName("Project"), FName("General")))
			);
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("LevelEditorSelection", LOCTEXT("SelectionHeading","Selection") );
	{
		MenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().AllowTranslucentSelection );
		MenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().AllowGroupSelection );
		MenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().StrictBoxSelect );
		MenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().ShowTransformWidget );
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("LevelEditorScalability", LOCTEXT("ScalabilityHeading", "Scalability") );
	{
		MenuBuilder.AddSubMenu(
			LOCTEXT( "ScalabilitySubMenu", "Engine Scalability Settings" ),
			LOCTEXT( "ScalabilitySubMenu_ToolTip", "Open the engine scalability settings" ),
			FNewMenuDelegate::CreateStatic( &MakeScalabilityMenu ) );

		MenuBuilder.AddSubMenu(
			LOCTEXT( "MaterialQualityLevelSubMenu", "Material Quality Level" ),
			LOCTEXT( "MaterialQualityLevelSubMenu_ToolTip", "Sets the value of the CVar \"r.MaterialQualityLevel\" (low=0, high=1). This affects materials via the QualitySwitch material expression." ),
			FNewMenuDelegate::CreateStatic( &MakeMaterialQualityLevelMenu ) );

		static const auto CVarFeatureLevelPreview = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.FeatureLevelPreview"));
		if (CVarFeatureLevelPreview->GetValueOnGameThread() != 0)
		{
			MenuBuilder.AddSubMenu(
				LOCTEXT("FeatureLevelPreviewSubMenu", "Feature Level Preview"),
				LOCTEXT("FeatureLevelPreviewSubMenu_ToolTip", "Sets the feature level preview mode"),
				FNewMenuDelegate::CreateStatic(&MakeShaderModelPreviewMenu));
		}
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("LevelEditorAudio", LOCTEXT("AudioHeading", "Real Time Audio") );
	{
		TSharedRef<SWidget> VolumeItem = SNew(SHorizontalBox)
											+SHorizontalBox::Slot()
											.FillWidth(0.9f)
											.Padding( FMargin(2.0f, 0.0f, 0.0f, 0.0f) )
											[
												SNew(SVolumeControl)
												.ToolTipText_Static(&FLevelEditorActionCallbacks::GetAudioVolumeToolTip)
												.Volume_Static(&FLevelEditorActionCallbacks::GetAudioVolume)
												.OnVolumeChanged_Static(&FLevelEditorActionCallbacks::OnAudioVolumeChanged)
												.Muted_Static(&FLevelEditorActionCallbacks::GetAudioMuted)
												.OnMuteChanged_Static(&FLevelEditorActionCallbacks::OnAudioMutedChanged)
											]
											+SHorizontalBox::Slot()
											.FillWidth(0.1f);

		MenuBuilder.AddWidget(VolumeItem, LOCTEXT("VolumeControlLabel","Volume"));
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection( "Snapping", LOCTEXT("SnappingHeading","Snapping") );
	{
		MenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().EnableActorSnap );
		TSharedRef<SWidget> SnapItem = 
		SNew(SHorizontalBox)
	          +SHorizontalBox::Slot()
	          .FillWidth(0.9f)
	          [
		          SNew(SSlider)
		          .ToolTipText_Static(&FLevelEditorActionCallbacks::GetActorSnapTooltip)
		          .Value_Static(&FLevelEditorActionCallbacks::GetActorSnapSetting)
		          .OnValueChanged_Static(&FLevelEditorActionCallbacks::SetActorSnapSetting)
	          ]
	          +SHorizontalBox::Slot()
	          .FillWidth(0.1f);
		MenuBuilder.AddWidget(SnapItem, LOCTEXT("ActorSnapLabel","Distance"));

		MenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().ToggleSocketSnapping );
		MenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().EnableVertexSnap );
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("LevelEditorViewport", LOCTEXT("ViewportHeading", "Viewport") );
	{
		MenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().ToggleHideViewportUI );

		MenuBuilder.AddSubMenu( LOCTEXT("PreviewMenu", "Previewing"), LOCTEXT("PreviewMenuTooltip","Game Preview Settings"), FNewMenuDelegate::CreateStatic( &MakePreviewSettingsMenu ) );
	}
	MenuBuilder.EndSection();

#undef LOCTEXT_NAMESPACE

	return MenuBuilder.MakeWidget();
}

TSharedRef< SWidget > FLevelEditorToolBar::GenerateCompileMenuContent( TSharedRef<FUICommandList> InCommandList )
{
#define LOCTEXT_NAMESPACE "LevelToolBarViewMenu"

	// Get all menu extenders for this context menu from the level editor module
	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>( TEXT("LevelEditor") );
	TArray<FLevelEditorModule::FLevelEditorMenuExtender> MenuExtenderDelegates = LevelEditorModule.GetAllLevelEditorToolbarCompileMenuExtenders();

	TArray<TSharedPtr<FExtender>> Extenders;
	for (int32 i = 0; i < MenuExtenderDelegates.Num(); ++i)
	{
		if (MenuExtenderDelegates[i].IsBound())
		{
			Extenders.Add(MenuExtenderDelegates[i].Execute(InCommandList));
		}
	}
	TSharedPtr<FExtender> MenuExtender = FExtender::Combine(Extenders);

	const bool bShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder MenuBuilder( bShouldCloseWindowAfterMenuSelection, InCommandList, MenuExtender );

	if (LevelEditorModule.CanBeRecompiled())
	{
		MenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().RecompileLevelEditor );
		MenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().ReloadLevelEditor );
	}

	MenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().RecompileGameCode );

#undef LOCTEXT_NAMESPACE

	return MenuBuilder.MakeWidget();
}

TSharedRef< SWidget > FLevelEditorToolBar::GenerateOpenBlueprintMenuContent( TSharedRef<FUICommandList> InCommandList, TWeakPtr< SLevelEditor > InLevelEditor )
{
#define LOCTEXT_NAMESPACE "LevelToolBarViewMenu"

	struct FBlueprintMenus
	{
		/** Generates a sub-level Blueprints sub-menu */
		static void MakeSubLevelsMenu(FMenuBuilder& InMenuBuilder, TWeakPtr< SLevelEditor > InLvlEditor)
		{
			FSlateIcon EditBP(FEditorStyle::Get().GetStyleSetName(), TEXT("LevelEditor.OpenLevelBlueprint"));

			InMenuBuilder.BeginSection(NAME_None, LOCTEXT("SubLevelsHeading", "Sub-Level Blueprints"));
			{
				UWorld* World = InLvlEditor.Pin()->GetWorld();
				for (int32 iLevel = 0; iLevel < World->GetNumLevels(); iLevel++)
				{
					ULevel* Level = World->GetLevel(iLevel);
					if (Level != NULL && Level->GetOutermost() != NULL)
					{
						if (!Level->IsPersistentLevel())
						{
							FUIAction UIAction
								(
								FExecuteAction::CreateStatic(&FLevelEditorToolBar::OnOpenSubLevelBlueprint, Level)
								);

							FText DisplayName = FText::Format(LOCTEXT("SubLevelBlueprintItem", "Edit {LevelName}"), FText::FromString(FPaths::GetCleanFilename(Level->GetOutermost()->GetName())));
							InMenuBuilder.AddMenuEntry(DisplayName, FText::GetEmpty(), EditBP, UIAction);
						}
					}
				}
			}
			InMenuBuilder.EndSection();
		}

		/** Handle BP being selected from popup picker */
		static void OnBPSelected(const class FAssetData& AssetData)
		{
			UBlueprint* SelectedBP = Cast<UBlueprint>(AssetData.GetAsset());
			if(SelectedBP)
			{
				FAssetEditorManager::Get().OpenEditorForAsset(SelectedBP);
			}
		}


		/** Generates 'open blueprint' sub-menu */
		static void MakeOpenClassBPMenu(FMenuBuilder& InMenuBuilder)
		{
			FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

			// Configure filter for asset picker
			FAssetPickerConfig Config;
			Config.Filter.ClassNames.Add(UBlueprint::StaticClass()->GetFName());
			Config.InitialAssetViewType = EAssetViewType::List;
			Config.ThumbnailScale = 0; // make thumbnails as small as possible
			Config.OnAssetSelected = FOnAssetSelected::CreateStatic(&FBlueprintMenus::OnBPSelected);
			Config.bAllowDragging = false;
			// Don't show stuff in Engine
			Config.Filter.PackagePaths.Add("/Game");
			Config.Filter.bRecursivePaths = true;

			TSharedRef<SWidget> Widget = 
				SNew(SBox)
				.WidthOverride(300.f)
				.HeightOverride(300.f)
				[
					ContentBrowserModule.Get().CreateAssetPicker(Config)
				];
		

			InMenuBuilder.BeginSection(NAME_None, LOCTEXT("BrowseHeader", "Browse"));
			{
				InMenuBuilder.AddWidget(Widget, FText::GetEmpty());
			}
			InMenuBuilder.EndSection();
		}
	};


	const bool bShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder MenuBuilder( bShouldCloseWindowAfterMenuSelection, InCommandList );

	MenuBuilder.BeginSection(NAME_None, LOCTEXT("LevelScriptBlueprints", "Level Blueprints"));
	{
		MenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().OpenLevelBlueprint );

		// If there are any sub-levels, display the sub-menu. A single level means there is only the persistent level
		UWorld* World = InLevelEditor.Pin()->GetWorld();
		if(World->GetNumLevels() > 1)
		{
			MenuBuilder.AddSubMenu(
				LOCTEXT( "SubLevelsSubMenu", "Sub-Levels" ),
				LOCTEXT( "SubLevelsSubMenu_ToolTip", "Shows available sub-level Blueprints that can be edited." ),
				FNewMenuDelegate::CreateStatic( &FBlueprintMenus::MakeSubLevelsMenu, InLevelEditor ), 
				FUIAction(), NAME_None, EUserInterfaceActionType::Button, false, FSlateIcon(FEditorStyle::Get().GetStyleSetName(), TEXT("LevelEditor.OpenLevelBlueprint")) );
		}
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection(NAME_None, LOCTEXT("WorldSettingsClasses", "World Settings Classes"));
	{
		AWorldSettings* WorldSettings = InLevelEditor.Pin()->GetWorld()->GetWorldSettings();

		// Game Mode
		AGameMode* ActiveGameMode = NULL;

		// If the WorldSettings' default game mode is valid, pull out the default object so we can edit it
		if(WorldSettings->DefaultGameMode)
		{
			ActiveGameMode = Cast<AGameMode>(InLevelEditor.Pin()->GetWorld()->GetWorldSettings()->DefaultGameMode->GetDefaultObject());
		}

		LevelEditorActionHelpers::FBlueprintMenuSettings GameModeMenuSettings;
		GameModeMenuSettings.EditCommand = FLevelEditorCommands::Get().OpenGameModeBlueprint;
		GameModeMenuSettings.OnCreateClassPicked = FOnClassPicked::CreateStatic( &FLevelEditorToolBar::OnCreateGameModeClassPicked, InLevelEditor );;
		GameModeMenuSettings.OnSelectClassPicked = FOnClassPicked::CreateStatic( &FLevelEditorToolBar::OnSelectGameModeClassPicked, InLevelEditor );;
		GameModeMenuSettings.CurrentClass = GetActiveGameModeClass(InLevelEditor);
		GameModeMenuSettings.RootClass = AGameMode::StaticClass();

		MenuBuilder.AddSubMenu( GetOpenGameModeBlueprintLabel(InLevelEditor), LOCTEXT("GameMode_Tooltip", "Select, edit, or create a new GameMode blueprint for the world"), FNewMenuDelegate::CreateStatic( &LevelEditorActionHelpers::GetBlueprintSettingsSubMenu, GameModeMenuSettings) );

		if(ActiveGameMode && ActiveGameMode->GetClass()->ClassGeneratedBy)
		{
			// Game State
			LevelEditorActionHelpers::FBlueprintMenuSettings GameStateMenuSettings;
			GameStateMenuSettings.EditCommand = FLevelEditorCommands::Get().OpenGameStateBlueprint;
			GameStateMenuSettings.OnCreateClassPicked = FOnClassPicked::CreateStatic( &FLevelEditorToolBar::OnCreateGameStateClassPicked, InLevelEditor );;
			GameStateMenuSettings.OnSelectClassPicked = FOnClassPicked::CreateStatic( &FLevelEditorToolBar::OnSelectGameStateClassPicked, InLevelEditor );;
			GameStateMenuSettings.CurrentClass = GetActiveGameStateClass(InLevelEditor);
			GameStateMenuSettings.RootClass = AGameState::StaticClass();

			MenuBuilder.AddSubMenu( GetOpenGameStateBlueprintLabel(InLevelEditor), LOCTEXT("GameState_Tooltip", "Select, edit, or create a new GameState blueprint for the world"), FNewMenuDelegate::CreateStatic( &LevelEditorActionHelpers::GetBlueprintSettingsSubMenu, GameStateMenuSettings ) );
		
			// Pawn
			LevelEditorActionHelpers::FBlueprintMenuSettings PawnMenuSettings;
			PawnMenuSettings.EditCommand = FLevelEditorCommands::Get().OpenDefaultPawnBlueprint;
			PawnMenuSettings.OnCreateClassPicked = FOnClassPicked::CreateStatic( &FLevelEditorToolBar::OnCreatePawnClassPicked, InLevelEditor );;
			PawnMenuSettings.OnSelectClassPicked = FOnClassPicked::CreateStatic( &FLevelEditorToolBar::OnSelectPawnClassPicked, InLevelEditor );;
			PawnMenuSettings.CurrentClass = GetActivePawnClass(InLevelEditor);
			PawnMenuSettings.RootClass = APawn::StaticClass();

			MenuBuilder.AddSubMenu( GetOpenPawnBlueprintLabel(InLevelEditor), LOCTEXT("Pawn_Tooltip", "Select, edit, or create a new Pawn blueprint for the world"), FNewMenuDelegate::CreateStatic( &LevelEditorActionHelpers::GetBlueprintSettingsSubMenu, PawnMenuSettings ) );

			// HUD
			LevelEditorActionHelpers::FBlueprintMenuSettings HUDMenuSettings;
			HUDMenuSettings.EditCommand = FLevelEditorCommands::Get().OpenHUDBlueprint;
			HUDMenuSettings.OnCreateClassPicked = FOnClassPicked::CreateStatic( &FLevelEditorToolBar::OnCreateHUDClassPicked, InLevelEditor );;
			HUDMenuSettings.OnSelectClassPicked = FOnClassPicked::CreateStatic( &FLevelEditorToolBar::OnSelectHUDClassPicked, InLevelEditor );;
			HUDMenuSettings.CurrentClass = GetActiveHUDClass(InLevelEditor);
			HUDMenuSettings.RootClass = AHUD::StaticClass();

			MenuBuilder.AddSubMenu( GetOpenHUDBlueprintLabel(InLevelEditor), LOCTEXT("HUD_Tooltip", "Select, edit, or create a new HUD blueprint for the world"), FNewMenuDelegate::CreateStatic( &LevelEditorActionHelpers::GetBlueprintSettingsSubMenu, HUDMenuSettings ) );

			// Player Controller
			LevelEditorActionHelpers::FBlueprintMenuSettings PlayerControllerMenuSettings;
			PlayerControllerMenuSettings.EditCommand = FLevelEditorCommands::Get().OpenHUDBlueprint;
			PlayerControllerMenuSettings.OnCreateClassPicked = FOnClassPicked::CreateStatic( &FLevelEditorToolBar::OnCreatePlayerControllerClassPicked, InLevelEditor );;
			PlayerControllerMenuSettings.OnSelectClassPicked = FOnClassPicked::CreateStatic( &FLevelEditorToolBar::OnSelectPlayerControllerClassPicked, InLevelEditor );;
			PlayerControllerMenuSettings.CurrentClass = GetActivePlayerControllerClass(InLevelEditor);
			PlayerControllerMenuSettings.RootClass = APlayerController::StaticClass();

			MenuBuilder.AddSubMenu( GetOpenPlayerControllerBlueprintLabel(InLevelEditor), LOCTEXT("PlayerController_Tooltip", "Select, edit, or create a new Player controller blueprint for the world"), FNewMenuDelegate::CreateStatic( &LevelEditorActionHelpers::GetBlueprintSettingsSubMenu, PlayerControllerMenuSettings ) );

		}
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection(NAME_None, LOCTEXT("ClassBlueprints", "Class Blueprints"));
	{
		// New Class Blueprint...
		MenuBuilder.AddMenuEntry(FLevelEditorCommands::Get().CreateClassBlueprint, NAME_None, LOCTEXT("NewClassBlueprint", "New Class Blueprint..."));

		// Open Class Blueprint...
		FSlateIcon OpenBPIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.OpenClassBlueprint");
		MenuBuilder.AddSubMenu(
			LOCTEXT("OpenClassBlueprintSubMenu", "Open Class Blueprint..."),
			LOCTEXT("OpenClassBlueprintSubMenu_ToolTip", "Open an existing Class Blueprint in this project"),
			FNewMenuDelegate::CreateStatic(&FBlueprintMenus::MakeOpenClassBPMenu), 
			false, 
			OpenBPIcon );
	}
	MenuBuilder.EndSection();

#undef LOCTEXT_NAMESPACE

	return MenuBuilder.MakeWidget();
}

void FLevelEditorToolBar::OnOpenSubLevelBlueprint( ULevel* InLevel )
{
	ULevelScriptBlueprint* LevelScriptBlueprint = InLevel->GetLevelScriptBlueprint();

	if( LevelScriptBlueprint )
	{
		FAssetEditorManager::Get().OpenEditorForAsset(LevelScriptBlueprint);
	}
	else
	{
		FMessageDialog::Open( EAppMsgType::Ok, NSLOCTEXT("UnrealEd", "UnableToCreateLevelScript", "Unable to find or create a level blueprint for this level.") );
	}
}

UClass* FLevelEditorToolBar::GetActiveGameModeClass(TWeakPtr< SLevelEditor > InLevelEditor)
{
	AWorldSettings* WorldSettings = InLevelEditor.Pin()->GetWorld()->GetWorldSettings();
	if(WorldSettings->DefaultGameMode)
	{
		return WorldSettings->DefaultGameMode;
	}
	return NULL;
}

FText FLevelEditorToolBar::GetOpenGameModeBlueprintLabel(TWeakPtr< SLevelEditor > InLevelEditor)
{
#define LOCTEXT_NAMESPACE "LevelToolBarViewMenu"
	if(UClass* GameModeClass = GetActiveGameModeClass(InLevelEditor))
	{
		if(GameModeClass->ClassGeneratedBy)
		{
			return FText::Format( LOCTEXT("GameModeEditBlueprint", "GameMode: Edit {GameModeName}"), FText::FromString(GameModeClass->ClassGeneratedBy->GetName()));
		}

		return FText::Format( LOCTEXT("GameModeBlueprint", "GameMode: {GameModeName}"), FText::FromString(GameModeClass->GetName()));
	}

	return LOCTEXT("GameModeCreateBlueprint", "GameMode: New...");
#undef LOCTEXT_NAMESPACE
}

void FLevelEditorToolBar::OnCreateGameModeClassPicked(UClass* InChosenClass, TWeakPtr< SLevelEditor > InLevelEditor)
{
	if(InChosenClass)
	{
		const FString NewBPName(TEXT("NewGameMode"));
		UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprintFromClass(NSLOCTEXT("LevelEditorCommands", "CreateGameModeBlueprint_Title", "Create GameMode Blueprint"), InChosenClass, NewBPName);

		if(Blueprint)
		{
			// @todo Re-enable once world centric works
			const bool bOpenWorldCentric = false;
			FAssetEditorManager::Get().OpenEditorForAsset(
				Blueprint,
				bOpenWorldCentric ? EToolkitMode::WorldCentric : EToolkitMode::Standalone,
				InLevelEditor.Pin()  );

			InLevelEditor.Pin()->GetWorld()->GetWorldSettings()->DefaultGameMode = Cast<UClass>(Blueprint->GeneratedClass);
		}
	}
	FSlateApplication::Get().DismissAllMenus();
}

void FLevelEditorToolBar::OnSelectGameModeClassPicked(UClass* InChosenClass, TWeakPtr< SLevelEditor > InLevelEditor)
{
	InLevelEditor.Pin()->GetWorld()->GetWorldSettings()->DefaultGameMode = InChosenClass;

	FSlateApplication::Get().DismissAllMenus();
}

UClass* FLevelEditorToolBar::GetActiveGameStateClass(TWeakPtr< SLevelEditor > InLevelEditor)
{
	AWorldSettings* WorldSettings = InLevelEditor.Pin()->GetWorld()->GetWorldSettings();
	if(WorldSettings->DefaultGameMode)
	{
		AGameMode* ActiveGameMode = Cast<AGameMode>(WorldSettings->DefaultGameMode->GetDefaultObject());
		if(ActiveGameMode)
		{
			return ActiveGameMode->GameStateClass;
		}
	}
	return NULL;
}

FText FLevelEditorToolBar::GetOpenGameStateBlueprintLabel(TWeakPtr< SLevelEditor > InLevelEditor)
{
#define LOCTEXT_NAMESPACE "LevelToolBarViewMenu"
	if(UClass* GameStateClass = GetActiveGameStateClass(InLevelEditor))
	{
		if(GameStateClass->ClassGeneratedBy)
		{
			return FText::Format( LOCTEXT("GameStateEditBlueprint", "GameState: Edit {GameStateName}"), FText::FromString(GameStateClass->ClassGeneratedBy->GetName()));
		}

		return FText::Format( LOCTEXT("GameStateBlueprint", "GameState: {GameStateName}"), FText::FromString(GameStateClass->GetName()));
	}

	return LOCTEXT("GameStateCreateBlueprint", "GameState: New...");
#undef LOCTEXT_NAMESPACE
}

void FLevelEditorToolBar::OnCreateGameStateClassPicked(UClass* InChosenClass, TWeakPtr< SLevelEditor > InLevelEditor)
{
	if(InChosenClass)
	{
		const FString NewBPName(TEXT("NewGameState"));
		UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprintFromClass(NSLOCTEXT("LevelEditorCommands", "CreateGameStateBlueprint_Title", "Create GameState Blueprint"), InChosenClass, NewBPName);

		if(Blueprint)
		{
			// @todo Re-enable once world centric works
			const bool bOpenWorldCentric = false;
			FAssetEditorManager::Get().OpenEditorForAsset(
				Blueprint,
				bOpenWorldCentric ? EToolkitMode::WorldCentric : EToolkitMode::Standalone,
				InLevelEditor.Pin()  );

			AGameMode* ActiveGameMode = Cast<AGameMode>(InLevelEditor.Pin()->GetWorld()->GetWorldSettings()->DefaultGameMode->GetDefaultObject());
			ActiveGameMode->GameStateClass = Cast<UClass>(Blueprint->GeneratedClass);
		}
	}
	FSlateApplication::Get().DismissAllMenus();
}

void FLevelEditorToolBar::OnSelectGameStateClassPicked(UClass* InChosenClass, TWeakPtr< SLevelEditor > InLevelEditor)
{
	AGameMode* ActiveGameMode = Cast<AGameMode>(InLevelEditor.Pin()->GetWorld()->GetWorldSettings()->DefaultGameMode->GetDefaultObject());

	ActiveGameMode->GameStateClass = InChosenClass;

	FSlateApplication::Get().DismissAllMenus();
}

UClass* FLevelEditorToolBar::GetActivePawnClass(TWeakPtr< SLevelEditor > InLevelEditor)
{
	AWorldSettings* WorldSettings = InLevelEditor.Pin()->GetWorld()->GetWorldSettings();
	if(WorldSettings->DefaultGameMode)
	{
		AGameMode* ActiveGameMode = Cast<AGameMode>(WorldSettings->DefaultGameMode->GetDefaultObject());
		if(ActiveGameMode)
		{
			return ActiveGameMode->DefaultPawnClass;
		}
	}
	return NULL;
}

FText FLevelEditorToolBar::GetOpenPawnBlueprintLabel(TWeakPtr< SLevelEditor > InLevelEditor)
{
#define LOCTEXT_NAMESPACE "LevelToolBarViewMenu"
	if(UClass* PawnClass = GetActivePawnClass(InLevelEditor))
	{
		if(PawnClass->ClassGeneratedBy)
		{
			return FText::Format( LOCTEXT("PawnEditBlueprint", "Pawn: Edit {PawnName}"), FText::FromString(PawnClass->ClassGeneratedBy->GetName()));
		}

		return FText::Format( LOCTEXT("PawnBlueprint", "Pawn: {PawnName}"), FText::FromString(PawnClass->GetName()));
	}

	return LOCTEXT("PawnCreateBlueprint", "Pawn: New...");
#undef LOCTEXT_NAMESPACE
}

void FLevelEditorToolBar::OnCreatePawnClassPicked(UClass* InChosenClass, TWeakPtr< SLevelEditor > InLevelEditor)
{
	if(InChosenClass)
	{
		const FString NewBPName(TEXT("NewPawn"));
		UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprintFromClass(NSLOCTEXT("LevelEditorCommands", "CreatePawnBlueprint_Title", "Create Pawn Blueprint"), InChosenClass, NewBPName);

		if(Blueprint)
		{
			// @todo Re-enable once world centric works
			const bool bOpenWorldCentric = false;
			FAssetEditorManager::Get().OpenEditorForAsset(
				Blueprint,
				bOpenWorldCentric ? EToolkitMode::WorldCentric : EToolkitMode::Standalone,
				InLevelEditor.Pin()  );

			AGameMode* ActiveGameMode = Cast<AGameMode>(InLevelEditor.Pin()->GetWorld()->GetWorldSettings()->DefaultGameMode->GetDefaultObject());
			ActiveGameMode->DefaultPawnClass = Cast<UClass>(Blueprint->GeneratedClass);
		}
	}
	FSlateApplication::Get().DismissAllMenus();
}

void FLevelEditorToolBar::OnSelectPawnClassPicked(UClass* InChosenClass, TWeakPtr< SLevelEditor > InLevelEditor)
{
	AGameMode* ActiveGameMode = Cast<AGameMode>(InLevelEditor.Pin()->GetWorld()->GetWorldSettings()->DefaultGameMode->GetDefaultObject());

	ActiveGameMode->DefaultPawnClass = InChosenClass;

	FSlateApplication::Get().DismissAllMenus();
}

UClass* FLevelEditorToolBar::GetActiveHUDClass(TWeakPtr< SLevelEditor > InLevelEditor)
{
	AWorldSettings* WorldSettings = InLevelEditor.Pin()->GetWorld()->GetWorldSettings();
	if(WorldSettings->DefaultGameMode)
	{
		AGameMode* ActiveGameMode = Cast<AGameMode>(WorldSettings->DefaultGameMode->GetDefaultObject());
		if(ActiveGameMode)
		{
			return ActiveGameMode->HUDClass;
		}
	}
	return NULL;
}

FText FLevelEditorToolBar::GetOpenHUDBlueprintLabel(TWeakPtr< SLevelEditor > InLevelEditor)
{
#define LOCTEXT_NAMESPACE "LevelToolBarViewMenu"
	if(UClass* HUDClass = GetActiveHUDClass(InLevelEditor))
	{
		if(HUDClass->ClassGeneratedBy)
		{
			return FText::Format( LOCTEXT("HUDEditBlueprint", "HUD: Edit {HUDName}"), FText::FromString(HUDClass->ClassGeneratedBy->GetName()));
		}

		return FText::Format( LOCTEXT("HUDBlueprint", "HUD: {HUDName}"), FText::FromString(HUDClass->GetName()));
	}

	return LOCTEXT("HUDCreateBlueprint", "HUD: New...");
#undef LOCTEXT_NAMESPACE
}

void FLevelEditorToolBar::OnCreateHUDClassPicked(UClass* InChosenClass, TWeakPtr< SLevelEditor > InLevelEditor)
{
	if(InChosenClass)
	{
		const FString NewBPName(TEXT("NewHUD"));
		UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprintFromClass(NSLOCTEXT("LevelEditorCommands", "CreateHUDBlueprint_Title", "Create HUD Blueprint"), InChosenClass, NewBPName);

		if(Blueprint)
		{
			// @todo Re-enable once world centric works
			const bool bOpenWorldCentric = false;
			FAssetEditorManager::Get().OpenEditorForAsset(
				Blueprint,
				bOpenWorldCentric ? EToolkitMode::WorldCentric : EToolkitMode::Standalone,
				InLevelEditor.Pin()  );

			AGameMode* ActiveGameMode = Cast<AGameMode>(InLevelEditor.Pin()->GetWorld()->GetWorldSettings()->DefaultGameMode->GetDefaultObject());
			ActiveGameMode->HUDClass = Cast<UClass>(Blueprint->GeneratedClass);
		}
	}
	FSlateApplication::Get().DismissAllMenus();
}

void FLevelEditorToolBar::OnSelectHUDClassPicked(UClass* InChosenClass, TWeakPtr< SLevelEditor > InLevelEditor)
{
	AGameMode* ActiveGameMode = Cast<AGameMode>(InLevelEditor.Pin()->GetWorld()->GetWorldSettings()->DefaultGameMode->GetDefaultObject());

	ActiveGameMode->HUDClass = InChosenClass;

	FSlateApplication::Get().DismissAllMenus();
}

UClass* FLevelEditorToolBar::GetActivePlayerControllerClass(TWeakPtr< SLevelEditor > InLevelEditor)
{
	AWorldSettings* WorldSettings = InLevelEditor.Pin()->GetWorld()->GetWorldSettings();
	if(WorldSettings->DefaultGameMode)
	{
		AGameMode* ActiveGameMode = Cast<AGameMode>(WorldSettings->DefaultGameMode->GetDefaultObject());
		if(ActiveGameMode)
		{
			return ActiveGameMode->PlayerControllerClass;
		}
	}
	return NULL;
}

FText FLevelEditorToolBar::GetOpenPlayerControllerBlueprintLabel(TWeakPtr< SLevelEditor > InLevelEditor)
{
#define LOCTEXT_NAMESPACE "LevelToolBarViewMenu"
	if(UClass* PlayerControllerClass = GetActivePlayerControllerClass(InLevelEditor))
	{
		if(PlayerControllerClass->ClassGeneratedBy)
		{
			return FText::Format( LOCTEXT("PlayerControllerEditBlueprint", "PlayerController: Edit {PlayerControllerName}"), FText::FromString(PlayerControllerClass->ClassGeneratedBy->GetName()));
		}

		return FText::Format( LOCTEXT("PlayerControllerBlueprint", "PlayerController: {PlayerControllerName}"), FText::FromString(PlayerControllerClass->GetName()));
	}

	return LOCTEXT("PlayerControllerCreateBlueprint", "PlayerController: New...");
#undef LOCTEXT_NAMESPACE
}

void FLevelEditorToolBar::OnCreatePlayerControllerClassPicked(UClass* InChosenClass, TWeakPtr< SLevelEditor > InLevelEditor)
{
	if(InChosenClass)
	{
		const FString NewBPName(TEXT("NewPlayerController"));
		UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprintFromClass(NSLOCTEXT("LevelEditorCommands", "CreatePlayerControllerBlueprint_Title", "Create PlayerController Blueprint"), InChosenClass, NewBPName);

		if(Blueprint)
		{
			// @todo Re-enable once world centric works
			const bool bOpenWorldCentric = false;
			FAssetEditorManager::Get().OpenEditorForAsset(
				Blueprint,
				bOpenWorldCentric ? EToolkitMode::WorldCentric : EToolkitMode::Standalone,
				InLevelEditor.Pin()  );

			AGameMode* ActiveGameMode = Cast<AGameMode>(InLevelEditor.Pin()->GetWorld()->GetWorldSettings()->DefaultGameMode->GetDefaultObject());
			ActiveGameMode->PlayerControllerClass = Cast<UClass>(Blueprint->GeneratedClass);
		}
	}
	FSlateApplication::Get().DismissAllMenus();
}

void FLevelEditorToolBar::OnSelectPlayerControllerClassPicked(UClass* InChosenClass, TWeakPtr< SLevelEditor > InLevelEditor)
{
	AGameMode* ActiveGameMode = Cast<AGameMode>(InLevelEditor.Pin()->GetWorld()->GetWorldSettings()->DefaultGameMode->GetDefaultObject());

	ActiveGameMode->PlayerControllerClass = InChosenClass;

	FSlateApplication::Get().DismissAllMenus();
}

TSharedRef< SWidget > FLevelEditorToolBar::GenerateMatineeMenuContent( TSharedRef<FUICommandList> InCommandList, TWeakPtr<SLevelEditor> LevelEditorWeakPtr )
{
#define LOCTEXT_NAMESPACE "LevelToolBarMatineeMenu"

	const bool bShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder MenuBuilder( bShouldCloseWindowAfterMenuSelection, InCommandList );

	// We can't build a list of Matinees while the current World is a PIE world.
	FSceneOutlinerInitializationOptions InitOptions;
	{
		InitOptions.Mode = ESceneOutlinerMode::ActorPicker;

		// We hide the header row to keep the UI compact.
		// @todo: Might be useful to have this sometimes, actually.  Ideally the user could summon it.
		InitOptions.bShowHeaderRow = false;

		struct Local
		{
			static bool IsMatineeActor( const AActor* const Actor )
			{
				return Actor->IsA( AMatineeActor::StaticClass() );
			}
		};

		// Only display Matinee actors
		InitOptions.Filters->AddFilterPredicate( SceneOutliner::FActorFilterPredicate::CreateStatic( &Local::IsMatineeActor ) );
	}

	// actor selector to allow the user to choose a Matinee actor
	FSceneOutlinerModule& SceneOutlinerModule = FModuleManager::LoadModuleChecked<FSceneOutlinerModule>( "SceneOutliner" );
	TSharedRef< SWidget > MiniSceneOutliner =
		SNew( SVerticalBox )
		+SVerticalBox::Slot()
		.AutoHeight()
		.MaxHeight(400.0f)
		[
			SceneOutlinerModule.CreateSceneOutliner(
				InitOptions,
				FOnActorPicked::CreateStatic( &FLevelEditorToolBar::OnMatineeActorPicked ) )
		];

	// Give the scene outliner a border and background
	const FSlateBrush* BackgroundBrush = FEditorStyle::GetBrush( "Menu.Background" );
	TSharedRef< SBorder > RootBorder =
		SNew( SBorder )
		.Padding(3)
		.BorderImage( BackgroundBrush )
		.ForegroundColor( FEditorStyle::GetSlateColor("DefaultForeground") )

		// Assign the box panel as the child
		[
			SNew( SVerticalBox )
			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding( 5 )
			.HAlign( HAlign_Center )
			[
				SNew( STextBlock )
				.Text( LOCTEXT( "SelectMatineeActorToEdit", "Select a Matinee actor" ) )
			]

			+SVerticalBox::Slot()
				.AutoHeight()
				.Padding( 2 )
				[
					MiniSceneOutliner
				]
		]
	;

	MenuBuilder.BeginSection("LevelEditorNewMatinee", LOCTEXT("MatineeMenuCombo_NewHeading", "New"));
	{
		MenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().AddMatinee );
	}
	MenuBuilder.EndSection();

	bool bHasAnyMatineeActors = false;
	TActorIterator<AMatineeActor> MatineeIt( LevelEditorWeakPtr.Pin()->GetWorld() );

	bHasAnyMatineeActors = MatineeIt;

	//Add a heading to separate the existing matinees from the 'Add New Matinee Actor' button
	MenuBuilder.BeginSection("LevelEditorExistingMatinee", LOCTEXT( "MatineeMenuCombo_ExistingHeading", "Edit Existing Matinee" ) );
	{
		if( bHasAnyMatineeActors )
		{
			MenuBuilder.AddWidget(MiniSceneOutliner, FText::GetEmpty(), false);
		}
	}
	MenuBuilder.EndSection();
#undef LOCTEXT_NAMESPACE

	return MenuBuilder.MakeWidget();
}

void FLevelEditorToolBar::OnMatineeActorPicked( AActor* Actor )
{
	//The matinee editor will not tick unless the editor viewport is in realtime mode.
	//the scene outliner eats input, so we must close any popups manually.
	FSlateApplication::Get().DismissAllMenus();

	// Make sure we dismiss the menus before we open this
	AMatineeActor* MatineeActor = Cast<AMatineeActor>( Actor );
	if( MatineeActor != NULL )
	{
		// Open Matinee for editing!
		GEditor->OpenMatinee( MatineeActor );
	}
}
