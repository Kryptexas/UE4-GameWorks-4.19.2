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

		// Internal builds and external builds should see the marketplace by default, perforce builds should not.
		if (FEngineBuildSettings::IsInternalBuild() || !FEngineBuildSettings::IsPerforceBuild()) 
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
			LOCTEXT("QuickSettingsCombo", "Quick Settings"),
			LOCTEXT("QuickSettingsCombo_ToolTip", "Quick level editor settings"),
			FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.ViewOptions")
			);

		ToolbarBuilder.AddToolBarButton( FLevelEditorCommands::Get().WorldProperties, NAME_None, LOCTEXT( "WorldProperties_Override", "World Settings" ), LOCTEXT( "WorldProperties_ToolTipOverride", "Displays the world settings" ), TAttribute<FSlateIcon>(), "LevelToolbarWorldSettings" );

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


	// Texture Stats
	MenuBuilder.BeginSection("LevelEditorStatistics", LOCTEXT( "Statistics", "Statistics" ) );
	{
	    MenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().SceneStats, NAME_None, LOCTEXT("OpenSceneStats", "Scene Stats") );
	    MenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().TextureStats, NAME_None, LOCTEXT("OpenTextureStats", "TextureStats Stats") );
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

static void MakeScalabilityMenu( FMenuBuilder& MenuBuilder )
{
	MenuBuilder.AddWidget(SNew(SScalabilitySettings), FText(), true);
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

	MenuBuilder.BeginSection("LevelEditorSelection", LOCTEXT("SelectionHeading","Selection") );
	{
		MenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().AllowTranslucentSelection );
		MenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().AllowGroupSelection );
		MenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().StrictBoxSelect );
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("LevelEditorEditing", LOCTEXT("EditingHeading", "Editing") );
	{
		MenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().ShowTransformWidget );
		MenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().DrawBrushMarkerPolys );
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("LevelEditorPreview", LOCTEXT("PreviewHeading", "Previewing") );
	{
		MenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().OnlyLoadVisibleInPIE );
		MenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().ToggleParticleSystemLOD );
		MenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().ToggleParticleSystemHelpers );
		MenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().ToggleFreezeParticleSimulation );
		MenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().ToggleLODViewLocking );
		MenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().LevelStreamingVolumePrevis );

		MenuBuilder.AddSubMenu(
			LOCTEXT( "ScalabilitySubMenu", "Engine Scalability Settings" ),
			LOCTEXT( "ScalabilitySubMenu_ToolTip", "Open the engine scalability settings" ),
			FNewMenuDelegate::CreateStatic( &MakeScalabilityMenu ) );

		MenuBuilder.AddSubMenu(
			LOCTEXT( "MaterialQualityLevelSubMenu", "Material Quality Level" ),
			LOCTEXT( "MaterialQualityLevelSubMenu_ToolTip", "Sets the value of the CVar \"r.MaterialQualityLevel\" (low=0, high=1). This affects materials via the QualitySwitch material expression." ),
			FNewMenuDelegate::CreateStatic( &MakeMaterialQualityLevelMenu ) );
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

	MenuBuilder.BeginSection("LevelEditorActorSnap", LOCTEXT("ActorSnapHeading","Actor Snap") );
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
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection( "Snapping", LOCTEXT("SnappingHeading","Snapping") );
	{
		MenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().ToggleSocketSnapping );
		MenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().EnableVertexSnap );
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("LevelEditorViewport", LOCTEXT("ViewportHeading", "Viewport") );
	{
		MenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().ToggleHideViewportUI );
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


		/** Generates 'eopn blueprint' sub-menu */
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

	MenuBuilder.BeginSection(NAME_None, LOCTEXT("GameBlueprints", "Game Blueprints"));
	{
		FSlateIcon EditBPIcon(FEditorStyle::Get().GetStyleSetName(), TEXT("PropertyWindow.Button_Edit"));
		FSlateIcon NewBPIcon(FEditorStyle::Get().GetStyleSetName(), TEXT("PropertyWindow.Button_AddToArray"));

		// Game Mode
		TAttribute<FText>::FGetter DynamicGameModeGetter;
		DynamicGameModeGetter.BindStatic(&FLevelEditorToolBar::GetOpenGameModeBlueprintLabel, InLevelEditor);
		TAttribute<FText> DynamicGameModeLabel = TAttribute<FText>::Create(DynamicGameModeGetter);

		TAttribute<FText>::FGetter DynamicGameModeGetter_Tooltip;
		DynamicGameModeGetter_Tooltip.BindStatic(&FLevelEditorToolBar::GetOpenGameModeBlueprintTooltip, InLevelEditor);
		TAttribute<FText> DynamicGameModeTooltip = TAttribute<FText>::Create(DynamicGameModeGetter_Tooltip);
		MenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().OpenGameModeBlueprint, NAME_None, DynamicGameModeLabel, DynamicGameModeTooltip, IsValidGameModeBlueprint(InLevelEditor)? EditBPIcon : NewBPIcon );

		// Game State
		TAttribute<FText>::FGetter DynamicGameStateGetter;
		DynamicGameStateGetter.BindStatic(&FLevelEditorToolBar::GetOpenGameStateBlueprintLabel, InLevelEditor);
		TAttribute<FText> DynamicGameStateLabel = TAttribute<FText>::Create(DynamicGameStateGetter);

		TAttribute<FText>::FGetter DynamicGameStateGetter_Tooltip;
		DynamicGameStateGetter_Tooltip.BindStatic(&FLevelEditorToolBar::GetOpenGameStateBlueprintTooltip, InLevelEditor);
		TAttribute<FText> DynamicGameStateTooltip = TAttribute<FText>::Create(DynamicGameStateGetter_Tooltip);
		MenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().OpenGameStateBlueprint, NAME_None, DynamicGameStateLabel, DynamicGameStateTooltip, IsValidGameStateBlueprint(InLevelEditor)? EditBPIcon : NewBPIcon );

		// Pawn
		TAttribute<FText>::FGetter DynamicDefaultPawnGetter;
		DynamicDefaultPawnGetter.BindStatic(&FLevelEditorToolBar::GetOpenPawnBlueprintLabel, InLevelEditor);
		TAttribute<FText> DynamicDefaultPawnLabel = TAttribute<FText>::Create(DynamicDefaultPawnGetter);

		TAttribute<FText>::FGetter DynamicDefaultPawnGetter_Tooltip;
		DynamicDefaultPawnGetter_Tooltip.BindStatic(&FLevelEditorToolBar::GetOpenPawnBlueprintTooltip, InLevelEditor);
		TAttribute<FText> DynamicDefaultPawnTooltip = TAttribute<FText>::Create(DynamicDefaultPawnGetter_Tooltip);
		MenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().OpenDefaultPawnBlueprint, NAME_None, DynamicDefaultPawnLabel, DynamicDefaultPawnTooltip, IsValidPawnBlueprint(InLevelEditor)? EditBPIcon : NewBPIcon );

		// HUD
		TAttribute<FText>::FGetter DynamicHUDGetter;
		DynamicHUDGetter.BindStatic(&FLevelEditorToolBar::GetOpenHUDBlueprintLabel, InLevelEditor);
		TAttribute<FText> DynamicHUDLabel = TAttribute<FText>::Create(DynamicHUDGetter);

		TAttribute<FText>::FGetter DynamicHUDGetter_Tooltip;
		DynamicHUDGetter_Tooltip.BindStatic(&FLevelEditorToolBar::GetOpenHUDBlueprintTooltip, InLevelEditor);
		TAttribute<FText> DynamicHUDTooltip = TAttribute<FText>::Create(DynamicHUDGetter_Tooltip);
		MenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().OpenHUDBlueprint, NAME_None, DynamicHUDLabel, DynamicHUDTooltip, IsValidHUDBlueprint(InLevelEditor)? EditBPIcon : NewBPIcon );

		// Player Controller
		TAttribute<FText>::FGetter DynamicPlayerControllerGetter;
		DynamicPlayerControllerGetter.BindStatic(&FLevelEditorToolBar::GetOpenPlayerControllerBlueprintLabel, InLevelEditor);
		TAttribute<FText> DynamicPlayerControllerLabel = TAttribute<FText>::Create(DynamicPlayerControllerGetter);

		TAttribute<FText>::FGetter DynamicPlayerControllerGetter_Tooltip;
		DynamicPlayerControllerGetter_Tooltip.BindStatic(&FLevelEditorToolBar::GetOpenPlayerControllerBlueprintTooltip, InLevelEditor);
		TAttribute<FText> DynamicPlayerControllerTooltip = TAttribute<FText>::Create(DynamicPlayerControllerGetter_Tooltip);
		MenuBuilder.AddMenuEntry( FLevelEditorCommands::Get().OpenPlayerControllerBlueprint, NAME_None, DynamicPlayerControllerLabel, DynamicPlayerControllerTooltip, IsValidPlayerControllerBlueprint(InLevelEditor)? EditBPIcon : NewBPIcon );
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

bool FLevelEditorToolBar::IsValidGameModeBlueprint(TWeakPtr< SLevelEditor > InLevelEditor)
{
	AWorldSettings* WorldSettings = InLevelEditor.Pin()->GetWorld()->GetWorldSettings();
	return WorldSettings->DefaultGameMode && WorldSettings->DefaultGameMode->ClassGeneratedBy;
}

FText FLevelEditorToolBar::GetOpenGameModeBlueprintLabel(TWeakPtr< SLevelEditor > InLevelEditor)
{
#define LOCTEXT_NAMESPACE "LevelToolBarViewMenu"
	if(IsValidGameModeBlueprint(InLevelEditor))
	{
		return FText::Format( LOCTEXT("GameModeEditBlueprint", "GameMode: Edit {GameModeName}"), FText::FromString(InLevelEditor.Pin()->GetWorld()->GetWorldSettings()->DefaultGameMode->ClassGeneratedBy->GetName()));
	}

	return LOCTEXT("GameModeCreateBlueprint", "GameMode: Create...");
#undef LOCTEXT_NAMESPACE
}

FText FLevelEditorToolBar::GetOpenGameModeBlueprintTooltip(TWeakPtr< SLevelEditor > InLevelEditor)
{
#define LOCTEXT_NAMESPACE "LevelToolBarViewMenu"
	if(IsValidGameModeBlueprint(InLevelEditor))
	{
		return LOCTEXT("GameModeEditBlueprint_Tooltip", "Open the World Setting's assigned GameMode Blueprint");
	}

	return LOCTEXT("GameModeCreateBlueprint_Tooltip", "Creates a new GameMode Blueprint and auto-assigns it to the current World Settings");
#undef LOCTEXT_NAMESPACE
}


bool FLevelEditorToolBar::IsValidGameStateBlueprint(TWeakPtr< SLevelEditor > InLevelEditor)
{
	AWorldSettings* WorldSettings = InLevelEditor.Pin()->GetWorld()->GetWorldSettings();
	if(WorldSettings->DefaultGameMode)
	{
		AGameMode* ActiveGameMode = Cast<AGameMode>(WorldSettings->DefaultGameMode->GetDefaultObject());
		return ActiveGameMode && ActiveGameMode->GameStateClass && ActiveGameMode->GameStateClass->ClassGeneratedBy;
	}
	return false;
}

FText FLevelEditorToolBar::GetOpenGameStateBlueprintLabel(TWeakPtr< SLevelEditor > InLevelEditor)
{
#define LOCTEXT_NAMESPACE "LevelToolBarViewMenu"
	if(IsValidGameStateBlueprint(InLevelEditor))
	{
		AGameMode* ActiveGameMode = Cast<AGameMode>(InLevelEditor.Pin()->GetWorld()->GetWorldSettings()->DefaultGameMode->GetDefaultObject());
		return FText::Format( LOCTEXT("GameStateEditBlueprint", "GameState: Edit {GameStateName}"), FText::FromString(ActiveGameMode->GameStateClass->ClassGeneratedBy->GetName()));
	}

	return LOCTEXT("GameStateCreateBlueprint", "GameState: Create...");
#undef LOCTEXT_NAMESPACE
}

FText FLevelEditorToolBar::GetOpenGameStateBlueprintTooltip(TWeakPtr< SLevelEditor > InLevelEditor)
{
#define LOCTEXT_NAMESPACE "LevelToolBarViewMenu"
	if(IsValidGameStateBlueprint(InLevelEditor))
	{
		return LOCTEXT("GameStateEditBlueprint_Tooltip", "Open the World Setting's assigned GameState Blueprint");
	}

	return LOCTEXT("GameStateCreateBlueprint_Tooltip", "Creates a new GameState Blueprint and auto-assigns it to the current World Settings");
#undef LOCTEXT_NAMESPACE
}

bool FLevelEditorToolBar::IsValidPawnBlueprint(TWeakPtr< SLevelEditor > InLevelEditor)
{
	AWorldSettings* WorldSettings = InLevelEditor.Pin()->GetWorld()->GetWorldSettings();
	if(WorldSettings->DefaultGameMode)
	{
		AGameMode* ActiveGameMode = Cast<AGameMode>(WorldSettings->DefaultGameMode->GetDefaultObject());
		return ActiveGameMode && ActiveGameMode->DefaultPawnClass && ActiveGameMode->DefaultPawnClass->ClassGeneratedBy;
	}
	return false;
}

FText FLevelEditorToolBar::GetOpenPawnBlueprintLabel(TWeakPtr< SLevelEditor > InLevelEditor)
{
#define LOCTEXT_NAMESPACE "LevelToolBarViewMenu"
	if(IsValidPawnBlueprint(InLevelEditor))
	{
		AGameMode* ActiveGameMode = Cast<AGameMode>(InLevelEditor.Pin()->GetWorld()->GetWorldSettings()->DefaultGameMode->GetDefaultObject());
		return FText::Format( LOCTEXT("PawnEditBlueprint", "Pawn: Edit {PawnName}"), FText::FromString(ActiveGameMode->DefaultPawnClass->ClassGeneratedBy->GetName()));
	}

	return LOCTEXT("PawnCreateBlueprint", "Pawn: Create...");
#undef LOCTEXT_NAMESPACE
}

FText FLevelEditorToolBar::GetOpenPawnBlueprintTooltip(TWeakPtr< SLevelEditor > InLevelEditor)
{
#define LOCTEXT_NAMESPACE "LevelToolBarViewMenu"
	if(IsValidPawnBlueprint(InLevelEditor))
	{
		return LOCTEXT("PawnEditBlueprint_Tooltip", "Open the World Setting's assigned Pawn Blueprint");
	}

	return LOCTEXT("PawnCreateBlueprint_Tooltip", "Creates a new Pawn Blueprint and auto-assigns it to the current World Settings");
#undef LOCTEXT_NAMESPACE
}

bool FLevelEditorToolBar::IsValidHUDBlueprint(TWeakPtr< SLevelEditor > InLevelEditor)
{
	AWorldSettings* WorldSettings = InLevelEditor.Pin()->GetWorld()->GetWorldSettings();
	if(WorldSettings->DefaultGameMode)
	{
		AGameMode* ActiveGameMode = Cast<AGameMode>(WorldSettings->DefaultGameMode->GetDefaultObject());
		return ActiveGameMode && ActiveGameMode->HUDClass && ActiveGameMode->HUDClass->ClassGeneratedBy;
	}
	return false;
}

FText FLevelEditorToolBar::GetOpenHUDBlueprintLabel(TWeakPtr< SLevelEditor > InLevelEditor)
{
#define LOCTEXT_NAMESPACE "LevelToolBarViewMenu"
	if(IsValidHUDBlueprint(InLevelEditor))
	{
		AGameMode* ActiveGameMode = Cast<AGameMode>(InLevelEditor.Pin()->GetWorld()->GetWorldSettings()->DefaultGameMode->GetDefaultObject());
		return FText::Format( LOCTEXT("HUDEditBlueprint", "HUD: Edit {HUDName}"), FText::FromString(ActiveGameMode->HUDClass->ClassGeneratedBy->GetName()));
	}

	return LOCTEXT("HUDCreateBlueprint", "HUD: Create...");
#undef LOCTEXT_NAMESPACE
}

FText FLevelEditorToolBar::GetOpenHUDBlueprintTooltip(TWeakPtr< SLevelEditor > InLevelEditor)
{
#define LOCTEXT_NAMESPACE "LevelToolBarViewMenu"
	if(IsValidHUDBlueprint(InLevelEditor))
	{
		return LOCTEXT("HUDEditBlueprint_Tooltip", "Open the World Setting's assigned HUD blueprint");
	}

	return LOCTEXT("HUDCreateBlueprint_Tooltip", "Creates a new HUD Blueprint and auto-assigns it to the current World Settings");
#undef LOCTEXT_NAMESPACE
}

bool FLevelEditorToolBar::IsValidPlayerControllerBlueprint(TWeakPtr< SLevelEditor > InLevelEditor)
{
	AWorldSettings* WorldSettings = InLevelEditor.Pin()->GetWorld()->GetWorldSettings();
	if(WorldSettings->DefaultGameMode)
	{
		AGameMode* ActiveGameMode = Cast<AGameMode>(WorldSettings->DefaultGameMode->GetDefaultObject());
		return ActiveGameMode && ActiveGameMode->PlayerControllerClass && ActiveGameMode->PlayerControllerClass->ClassGeneratedBy;
	}
	return false;
}

FText FLevelEditorToolBar::GetOpenPlayerControllerBlueprintLabel(TWeakPtr< SLevelEditor > InLevelEditor)
{
#define LOCTEXT_NAMESPACE "LevelToolBarViewMenu"
	if(IsValidPlayerControllerBlueprint(InLevelEditor))
	{
		AGameMode* ActiveGameMode = Cast<AGameMode>(InLevelEditor.Pin()->GetWorld()->GetWorldSettings()->DefaultGameMode->GetDefaultObject());
		return FText::Format( LOCTEXT("PlayerControllerEditBlueprint", "PlayerController: Edit {PlayerControllerName}"), FText::FromString(ActiveGameMode->PlayerControllerClass->ClassGeneratedBy->GetName()));
	}

	return LOCTEXT("PlayerControllerCreateBlueprint", "PlayerController: Create...");
#undef LOCTEXT_NAMESPACE
}

FText FLevelEditorToolBar::GetOpenPlayerControllerBlueprintTooltip(TWeakPtr< SLevelEditor > InLevelEditor)
{
#define LOCTEXT_NAMESPACE "LevelToolBarViewMenu"
	if(IsValidPlayerControllerBlueprint(InLevelEditor))
	{
		return LOCTEXT("PlayerControllerEditBlueprint_Tooltip", "Open the World Setting's assigned PlayerController Blueprint");
	}

	return LOCTEXT("PlayerControllerCreateBlueprint_Tooltip", "Creates a new PlayerController Blueprint and auto-assigns it to the current World Settings");
#undef LOCTEXT_NAMESPACE
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

		// Only display Matinee actors
		InitOptions.ActorFilters = MakeShareable( new TFilterCollection< const AActor* const >() );

		struct Local
		{
			static bool IsMatineeActor( const AActor* const Actor )
			{
				return Actor->IsA( AMatineeActor::StaticClass() );
			}
		};

		InitOptions.ActorFilters->Add( MakeShareable( new TDelegateFilter< const AActor* const >( TDelegateFilter< const AActor* const >::FPredicate::CreateStatic( &Local::IsMatineeActor ) ) ) );
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
				FOnContextMenuOpening(), //no context menu allowed here
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
