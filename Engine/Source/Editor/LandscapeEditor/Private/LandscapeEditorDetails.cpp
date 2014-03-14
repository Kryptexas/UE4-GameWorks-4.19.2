// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "LandscapeEditorPrivatePCH.h"
#include "LandscapeEditorDetails.h"
#include "LandscapeEditorDetailCustomizations.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"

#include "LandscapeEdMode.h"
#include "LandscapeEditorCommands.h"
#include "LandscapeEditorObject.h"
#include "LandscapeEditorDetailWidgets.h"

#define LOCTEXT_NAMESPACE "LandscapeEditor"


TSharedRef<IDetailCustomization> FLandscapeEditorDetails::MakeInstance()
{
	return MakeShareable(new FLandscapeEditorDetails);
}

void FLandscapeEditorDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	//FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	//TSharedPtr<SLevelEditor> LevelEditor = StaticCastSharedPtr<SLevelEditor>(LevelEditorModule.GetFirstLevelEditor());
	//CommandList = LevelEditor->GetLevelEditorActions();

	check(!CommandList.IsValid());
	CommandList = MakeShareable(new FUICommandList);

	auto NameToCommandMap = FLandscapeEditorCommands::Get().NameToCommandMap;

#define MAP_TOOL(ToolSetName) CommandList->MapAction(NameToCommandMap.FindChecked(ToolSetName), FUIAction(FExecuteAction::CreateSP(this, &FLandscapeEditorDetails::OnChangeTool, FName(ToolSetName)), FCanExecuteAction::CreateSP(this, &FLandscapeEditorDetails::IsToolEnabled, FName(ToolSetName)), FIsActionChecked::CreateStatic(&FLandscapeEditorDetails::IsToolActive, FName(ToolSetName))));
	MAP_TOOL("ToolSet_NewLandscape");
	MAP_TOOL("ToolSet_ResizeLandscape");

	MAP_TOOL("ToolSet_Sculpt");
	MAP_TOOL("ToolSet_Paint");
	MAP_TOOL("ToolSet_Smooth");
	MAP_TOOL("ToolSet_Flatten");
	MAP_TOOL("ToolSet_Ramp");
	MAP_TOOL("ToolSet_Erosion");
	MAP_TOOL("ToolSet_HydraErosion");
	MAP_TOOL("ToolSet_Noise");
	MAP_TOOL("ToolSet_Retopologize");
	MAP_TOOL("ToolSet_Visibility");

	MAP_TOOL("ToolSet_Select");
	MAP_TOOL("ToolSet_AddComponent");
	MAP_TOOL("ToolSet_DeleteComponent");
	MAP_TOOL("ToolSet_MoveToLevel");

	MAP_TOOL("ToolSet_Mask");
	MAP_TOOL("ToolSet_CopyPaste");

	MAP_TOOL("ToolSet_Splines");
#undef MAP_TOOL
#define MAP_BRUSH_SET(BrushSetName) CommandList->MapAction(NameToCommandMap.FindChecked(BrushSetName), FUIAction(FExecuteAction::CreateSP(this, &FLandscapeEditorDetails::OnChangeBrushSet, FName(BrushSetName)), FCanExecuteAction::CreateSP(this, &FLandscapeEditorDetails::IsBrushSetEnabled, FName(BrushSetName)), FIsActionChecked::CreateStatic(&FLandscapeEditorDetails::IsBrushSetActive, FName(BrushSetName))));
	MAP_BRUSH_SET("BrushSet_Circle");
	MAP_BRUSH_SET("BrushSet_Alpha");
	MAP_BRUSH_SET("BrushSet_Pattern");
	MAP_BRUSH_SET("BrushSet_Component");
	MAP_BRUSH_SET("BrushSet_Gizmo");
#undef MAP_BRUSH_SET
#define MAP_BRUSH(BrushName) CommandList->MapAction(NameToCommandMap.FindChecked(BrushName), FUIAction(FExecuteAction::CreateSP(this, &FLandscapeEditorDetails::OnChangeBrush, FName(BrushName)), FCanExecuteAction(), FIsActionChecked::CreateSP(this, &FLandscapeEditorDetails::IsBrushActive, FName(BrushName))));
	MAP_BRUSH("Circle_Smooth");
	MAP_BRUSH("Circle_Linear");
	MAP_BRUSH("Circle_Spherical");
	MAP_BRUSH("Circle_Tip");
#undef MAP_BRUSH

	static const FLinearColor BorderColor = FLinearColor(0.2f, 0.2f, 0.2f, 0.2f);
	static const FSlateBrush* BorderStyle =  FEditorStyle::GetBrush("DetailsView.GroupSection");

	IDetailCategoryBuilder& LandscapeEditorCategory = DetailBuilder.EditCategory("LandscapeEditor", TEXT(""), ECategoryPriority::TypeSpecific);

	LandscapeEditorCategory.AddCustomRow("")
	.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateStatic(&FLandscapeEditorDetails::GetTargetLandscapeSelectorVisibility)))
	[
		SNew(SComboButton)
		.OnGetMenuContent_Static(&FLandscapeEditorDetails::GetTargetLandscapeMenu)
		.ButtonContent()
		[
			SNew(STextBlock)
			.Text(GetTargetLandscapeName())
		]
	];

	FToolSelectorBuilder ToolBrushSelectorButtons(CommandList, FMultiBoxCustomization::None);
	{
		FUIAction ToolSelectorUIAction;
		//ToolSelectorUIAction.IsActionVisibleDelegate.BindSP(this, &FLandscapeEditorDetails::GetToolSelectorIsVisible);
		ToolBrushSelectorButtons.AddComboButton(
			ToolSelectorUIAction,
			FOnGetContent::CreateSP(this, &FLandscapeEditorDetails::GetToolSelector),
			LOCTEXT("ToolSelector", "Tool"),
			TAttribute<FText>(this, &FLandscapeEditorDetails::GetCurrentToolName),
			LOCTEXT("ToolSelector.Tooltip", "Select Tool"),
			TAttribute<FSlateIcon>(this, &FLandscapeEditorDetails::GetCurrentToolIcon)
			);

		FUIAction BrushSelectorUIAction;
		BrushSelectorUIAction.IsActionVisibleDelegate.BindSP(this, &FLandscapeEditorDetails::GetBrushSelectorIsVisible);
		ToolBrushSelectorButtons.AddComboButton(
			BrushSelectorUIAction,
			FOnGetContent::CreateSP(this, &FLandscapeEditorDetails::GetBrushSelector),
			LOCTEXT("BrushSelector", "Brush"),
			TAttribute<FText>(this, &FLandscapeEditorDetails::GetCurrentBrushName),
			LOCTEXT("BrushSelector.Tooltip", "Select Brush"),
			TAttribute<FSlateIcon>(this, &FLandscapeEditorDetails::GetCurrentBrushIcon)
			);

		FUIAction BrushFalloffSelectorUIAction;
		BrushFalloffSelectorUIAction.IsActionVisibleDelegate.BindSP(this, &FLandscapeEditorDetails::GetBrushFalloffSelectorIsVisible);
		ToolBrushSelectorButtons.AddComboButton(
			BrushFalloffSelectorUIAction,
			FOnGetContent::CreateSP(this, &FLandscapeEditorDetails::GetBrushFalloffSelector),
			LOCTEXT("BrushFalloffSelector", "Falloff"),
			TAttribute<FText>(this, &FLandscapeEditorDetails::GetCurrentBrushFalloffName),
			LOCTEXT("BrushFalloffSelector.Tooltip", "Select Brush Falloff Type"),
			TAttribute<FSlateIcon>(this, &FLandscapeEditorDetails::GetCurrentBrushFalloffIcon)
			);
	}

	LandscapeEditorCategory.AddCustomRow("")
	.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FLandscapeEditorDetails::GetToolSelectorVisibility)))
	[
		ToolBrushSelectorButtons.MakeWidget()
	];

	// Tools:
	Customization_NewLandscape = MakeShareable(new FLandscapeEditorDetailCustomization_NewLandscape);
	Customization_NewLandscape->CustomizeDetails(DetailBuilder);
	Customization_ResizeLandscape = MakeShareable(new FLandscapeEditorDetailCustomization_ResizeLandscape);
	Customization_ResizeLandscape->CustomizeDetails(DetailBuilder);
	Customization_CopyPaste = MakeShareable(new FLandscapeEditorDetailCustomization_CopyPaste);
	Customization_CopyPaste->CustomizeDetails(DetailBuilder);
	Customization_MiscTools = MakeShareable(new FLandscapeEditorDetailCustomization_MiscTools);
	Customization_MiscTools->CustomizeDetails(DetailBuilder);

	// Brushes:
	Customization_AlphaBrush = MakeShareable(new FLandscapeEditorDetailCustomization_AlphaBrush);
	Customization_AlphaBrush->CustomizeDetails(DetailBuilder);

	// Target Layers:
	Customization_TargetLayers = MakeShareable(new FLandscapeEditorDetailCustomization_TargetLayers);
	Customization_TargetLayers->CustomizeDetails(DetailBuilder);
}

FText FLandscapeEditorDetails::GetLocalizedName(FName Name)
{
	static bool bInitialized = false;
	if (!bInitialized)
	{
		bInitialized = true;
		LOCTEXT("ToolSet_NewLandscape", "New Landscape");
		LOCTEXT("ToolSet_ResizeLandscape", "Change Component Size");
		LOCTEXT("ToolSet_Sculpt", "Sculpt");
		LOCTEXT("ToolSet_Paint", "Paint");
		LOCTEXT("ToolSet_Smooth", "Smooth");
		LOCTEXT("ToolSet_Flatten", "Flatten");
		LOCTEXT("ToolSet_Ramp", "Ramp");
		LOCTEXT("ToolSet_Erosion", "Erosion");
		LOCTEXT("ToolSet_HydraErosion", "HydroErosion");
		LOCTEXT("ToolSet_Noise", "Noise");
		LOCTEXT("ToolSet_Retopologize", "Retopologize");
		LOCTEXT("ToolSet_Visibility", "Visibility");

		LOCTEXT("ToolSet_Select", "Selection");
		LOCTEXT("ToolSet_AddComponent", "Add");
		LOCTEXT("ToolSet_DeleteComponent", "Delete");
		LOCTEXT("ToolSet_MoveToLevel", "Move to Level");

		LOCTEXT("ToolSet_Mask", "Selection");
		LOCTEXT("ToolSet_CopyPaste", "Copy/Paste");

		LOCTEXT("ToolSet_Splines", "Edit Splines");

		LOCTEXT("BrushSet_Circle", "Circle");
		LOCTEXT("BrushSet_Alpha", "Alpha");
		LOCTEXT("BrushSet_Pattern", "Pattern");
		LOCTEXT("BrushSet_Component", "Component");
		LOCTEXT("BrushSet_Gizmo", "Gizmo");

		LOCTEXT("Circle_Smooth", "Smooth");
		LOCTEXT("Circle_Linear", "Linear");
		LOCTEXT("Circle_Spherical", "Spherical");
		LOCTEXT("Circle_Tip", "Tip");
	}

	FText Result;
	ensure(FText::FindText(LOCTEXT_NAMESPACE, Name.ToString(), Result));
	return Result;
}

EVisibility FLandscapeEditorDetails::GetTargetLandscapeSelectorVisibility()
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode && LandscapeEdMode->GetLandscapeList().Num() > 1)
	{
		return EVisibility::Visible;
	}

	return EVisibility::Collapsed;
}

FText FLandscapeEditorDetails::GetTargetLandscapeName()
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode && LandscapeEdMode->CurrentToolTarget.LandscapeInfo.IsValid())
	{
		return FText::FromString(LandscapeEdMode->CurrentToolTarget.LandscapeInfo->GetLandscapeProxy()->GetActorLabel());
	}

	return FText();
}

TSharedRef<SWidget> FLandscapeEditorDetails::GetTargetLandscapeMenu()
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode)
	{
		FMenuBuilder MenuBuilder(true, NULL);

		const TArray<FLandscapeListInfo>& LandscapeList = LandscapeEdMode->GetLandscapeList();
		for (auto It = LandscapeList.CreateConstIterator(); It; It++)
		{
			FUIAction Action = FUIAction(FExecuteAction::CreateStatic(&FLandscapeEditorDetails::OnChangeTargetLandscape, TWeakObjectPtr<ULandscapeInfo>(It->Info)));
			MenuBuilder.AddMenuEntry( FText::FromString( It->Info->GetLandscapeProxy()->GetActorLabel() ), FText(), FSlateIcon(), Action);
		}

		return MenuBuilder.MakeWidget();
	}

	return SNullWidget::NullWidget;
}

void FLandscapeEditorDetails::OnChangeTargetLandscape(TWeakObjectPtr<ULandscapeInfo> LandscapeInfo)
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode)
	{
		LandscapeEdMode->CurrentToolTarget.LandscapeInfo = LandscapeInfo.Get();
		LandscapeEdMode->UpdateTargetList();
		if (LandscapeEdMode->CurrentGizmoActor.IsValid())
		{
			LandscapeEdMode->CurrentGizmoActor->SetTargetLandscape(LandscapeEdMode->CurrentToolTarget.LandscapeInfo.Get());
		}
	}
}

FText FLandscapeEditorDetails::GetCurrentToolName() const
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode != NULL && LandscapeEdMode->CurrentToolSet != NULL)
	{
		const FName CurrentToolSetName = LandscapeEdMode->CurrentToolSet->GetToolSetName();
		return GetLocalizedName(CurrentToolSetName);
	}

	return LOCTEXT("Unknown", "Unknown");
}

FSlateIcon FLandscapeEditorDetails::GetCurrentToolIcon() const
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode != NULL && LandscapeEdMode->CurrentToolSet != NULL)
	{
		const FName CurrentToolSetName = LandscapeEdMode->CurrentToolSet->GetToolSetName();
		return FLandscapeEditorCommands::Get().NameToCommandMap.FindChecked(CurrentToolSetName)->GetIcon();
	}

	return FSlateIcon(FEditorStyle::GetStyleSetName(), "Default");
}

TSharedRef<SWidget> FLandscapeEditorDetails::GetToolSelector()
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode != NULL)
	{
		auto NameToCommandMap = FLandscapeEditorCommands::Get().NameToCommandMap;

		FToolMenuBuilder MenuBuilder(true, CommandList);

		if (LandscapeEdMode->CurrentToolMode->ToolModeName == "ToolMode_Manage")
		{
			MenuBuilder.BeginSection(NAME_None, LOCTEXT("NewLandscapeToolsTitle", "New Landscape"));
			MenuBuilder.AddToolButton(NameToCommandMap.FindChecked("ToolSet_NewLandscape"), NAME_None, LOCTEXT("Tool.NewLandscape", "New Landscape"), LOCTEXT("Tool.NewLandscape.Tooltip", "Create or import a new landscape"));
			MenuBuilder.EndSection();

			MenuBuilder.BeginSection(NAME_None, LOCTEXT("ComponentToolsTitle", "Component Tools"));
			MenuBuilder.AddToolButton(NameToCommandMap.FindChecked("ToolSet_Select"), NAME_None, LOCTEXT("Tool.SelectComponent", "Selection"), LOCTEXT("Tool.SelectComponent.Tooltip", "Select components to use with other tools"));
			MenuBuilder.AddToolButton(NameToCommandMap.FindChecked("ToolSet_AddComponent"), NAME_None, LOCTEXT("Tool.AddComponent", "Add"), LOCTEXT("Tool.AddComponent.Tooltip", "Add components to the landscape"));
			MenuBuilder.AddToolButton(NameToCommandMap.FindChecked("ToolSet_DeleteComponent"), NAME_None, LOCTEXT("Tool.DeleteComponent", "Delete"), LOCTEXT("Tool.DeleteComponent.Tooltip", "Delete components from the landscape, leaving a hole"));
			MenuBuilder.AddToolButton(NameToCommandMap.FindChecked("ToolSet_MoveToLevel"), NAME_None, LOCTEXT("Tool.MoveToLevel", "Move Level"), LOCTEXT("Tool.MoveToLevel.Tooltip", "Move landscape components to a landscape proxy in the currently active streaming level, so that they can be streamed in/out independently of the rest of the landscape"));
			MenuBuilder.AddToolButton(NameToCommandMap.FindChecked("ToolSet_ResizeLandscape"), NAME_None, LOCTEXT("Tool.ResizeLandscape", "Change Component Size"), LOCTEXT("Tool.ResizeLandscape.Tooltip", "Change the size of the landscape components"));
			MenuBuilder.EndSection();

			MenuBuilder.BeginSection(NAME_None, LOCTEXT("SplineToolsTitle", "Spline Tools"));
			MenuBuilder.AddToolButton(NameToCommandMap.FindChecked("ToolSet_Splines"), NAME_None, LOCTEXT("Tool.Spline", "Edit Splines"), LOCTEXT("Tool.Spline.Tooltip", "Ctrl+click to add control points\nHaving a control point selected when you ctrl+click will connect to the new control point with a segment\nSpline mesh settings can be found on the details panel when you have segments selected"));
			MenuBuilder.EndSection();
		}

		if (LandscapeEdMode->CurrentToolMode->ToolModeName == "ToolMode_Sculpt")
		{
			MenuBuilder.BeginSection(NAME_None, LOCTEXT("SculptToolsTitle", "Sculpting Tools"));
			MenuBuilder.AddToolButton(NameToCommandMap.FindChecked("ToolSet_Sculpt"), NAME_None, LOCTEXT("Tool.Sculpt", "Sculpt"), LOCTEXT("Tool.Sculpt.Tooltip", "Sculpt height data.\nCtrl+Click to Raise, Ctrl+Shift+Click to lower"));
			MenuBuilder.AddToolButton(NameToCommandMap.FindChecked("ToolSet_Smooth"), NAME_None, LOCTEXT("Tool.Smooth", "Smooth"), LOCTEXT("Tool.Smooth.Tooltip", "Smooths heightmaps or blend layers"));
			MenuBuilder.AddToolButton(NameToCommandMap.FindChecked("ToolSet_Flatten"), NAME_None, LOCTEXT("Tool.Flatten", "Flatten"), LOCTEXT("Tool.Flatten.Tooltip", "Flattens an area of heightmap or blend layer"));
			MenuBuilder.AddToolButton(NameToCommandMap.FindChecked("ToolSet_Ramp"), NAME_None, LOCTEXT("Tool.Ramp", "Ramp"), LOCTEXT("Tool.Ramp.Tooltip", "Creates a ramp between two points"));
			MenuBuilder.AddToolButton(NameToCommandMap.FindChecked("ToolSet_Erosion"), NAME_None, LOCTEXT("Tool.Erosion", "Erosion"), LOCTEXT("Tool.Erosion.Tooltip", "Thermal Erosion - Simulates erosion caused by the movement of soil from higher areas to lower areas"));
			MenuBuilder.AddToolButton(NameToCommandMap.FindChecked("ToolSet_HydraErosion"), NAME_None, LOCTEXT("Tool.HydroErosion", "Hydro Erosion"), LOCTEXT("Tool.HydroErosion.Tooltip", "Hydro Erosion - Simulates erosion caused by rainfall"));
			MenuBuilder.AddToolButton(NameToCommandMap.FindChecked("ToolSet_Noise"), NAME_None, LOCTEXT("Tool.Noise", "Noise"), LOCTEXT("Tool.Noise.Tooltip", "Adds noise to the heightmap or blend layer"));
			MenuBuilder.AddToolButton(NameToCommandMap.FindChecked("ToolSet_Retopologize"), NAME_None, LOCTEXT("Tool.Retopologize", "Retopologize"), LOCTEXT("Tool.Retopologize.Tooltip", "Automatically adjusts landscape vertices with an X/Y offset map to improve vertex density on cliffs, reducing texture stretching.\nNote: An X/Y offset map makes the landscape slower to render and paint on with other tools, so only use if needed"));
			MenuBuilder.AddToolButton(NameToCommandMap.FindChecked("ToolSet_Visibility"), NAME_None, LOCTEXT("Tool.Visibility", "Visibility"), LOCTEXT("Tool.Visibility.Tooltip", "Mask out individual quads in the landscape, leaving a hole."));
			MenuBuilder.EndSection();

			MenuBuilder.BeginSection(NAME_None, LOCTEXT("RegionToolsTitle", "Region Tools"));
			MenuBuilder.AddToolButton(NameToCommandMap.FindChecked("ToolSet_Mask"), NAME_None, LOCTEXT("Tool.RegionSelect", "Selection"), LOCTEXT("Tool.RegionSelect.Tooltip", "Select a region of landscape to use as a mask for other tools"));
			MenuBuilder.AddToolButton(NameToCommandMap.FindChecked("ToolSet_CopyPaste"), NAME_None, LOCTEXT("Tool.RegionCopyPaste", "Copy/Paste"), LOCTEXT("Tool.RegionCopyPaste.Tooltip", "Copy/Paste areas of the landscape, or import/export a copied area of landscape from disk"));
			MenuBuilder.EndSection();
		}

		if (LandscapeEdMode->CurrentToolMode->ToolModeName == "ToolMode_Paint")
		{
			MenuBuilder.BeginSection(NAME_None, LOCTEXT("PaintToolsTitle", "Paint Tools"));
			MenuBuilder.AddToolButton(NameToCommandMap.FindChecked("ToolSet_Paint"), NAME_None, LOCTEXT("Tool.Paint", "Paint"), LOCTEXT("Tool.Paint.Tooltip", "Paints weight data.\nCtrl+Click to paint, Ctrl+Shift+Click to erase"));
			MenuBuilder.AddToolButton(NameToCommandMap.FindChecked("ToolSet_Smooth"), NAME_None, LOCTEXT("Tool.Smooth", "Smooth"), LOCTEXT("Tool.Smooth.Tooltip", "Smooths heightmaps or blend layers"));
			MenuBuilder.AddToolButton(NameToCommandMap.FindChecked("ToolSet_Flatten"), NAME_None, LOCTEXT("Tool.Flatten", "Flatten"), LOCTEXT("Tool.Flatten.Tooltip", "Flattens an area of heightmap or blend layer"));
			MenuBuilder.AddToolButton(NameToCommandMap.FindChecked("ToolSet_Noise"), NAME_None, LOCTEXT("Tool.Noise", "Noise"), LOCTEXT("Tool.Noise.Tooltip", "Adds noise to the heightmap or blend layer"));
			MenuBuilder.EndSection();
		}

		return MenuBuilder.MakeWidget();
	}

	return SNullWidget::NullWidget;
}

bool FLandscapeEditorDetails::GetToolSelectorIsVisible() const
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode && LandscapeEdMode->CurrentToolSet)
	{
		if (!IsToolActive("ToolSet_NewLandscape") || LandscapeEdMode->GetLandscapeList().Num() > 0)
		{
			return true;
		}
	}

	return false;
}

EVisibility FLandscapeEditorDetails::GetToolSelectorVisibility() const
{
	if (GetToolSelectorIsVisible())
	{
		return EVisibility::Visible;
	}
	return EVisibility::Collapsed;
}

void FLandscapeEditorDetails::OnChangeTool(FName ToolSetName)
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode != NULL)
	{
		LandscapeEdMode->SetCurrentTool(ToolSetName);
	}
}

bool FLandscapeEditorDetails::IsToolEnabled(FName ToolSetName) const
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode != NULL)
	{
		if (ToolSetName == "ToolSet_NewLandscape" || LandscapeEdMode->GetLandscapeList().Num() > 0)
		{
			return true;
		}
	}

	return false;
}

FText FLandscapeEditorDetails::GetCurrentBrushName() const
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode != NULL && LandscapeEdMode->CurrentBrush != NULL)
	{
		const FName CurrentBrushSetName = LandscapeEdMode->LandscapeBrushSets[LandscapeEdMode->CurrentBrushSetIndex].BrushSetName;
		return GetLocalizedName(CurrentBrushSetName);
	}

	return LOCTEXT("Unknown", "Unknown");
}

FSlateIcon FLandscapeEditorDetails::GetCurrentBrushIcon() const
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode != NULL && LandscapeEdMode->CurrentBrush != NULL)
	{
		const FName CurrentBrushSetName = LandscapeEdMode->LandscapeBrushSets[LandscapeEdMode->CurrentBrushSetIndex].BrushSetName;
		TSharedPtr<FUICommandInfo> Command = FLandscapeEditorCommands::Get().NameToCommandMap.FindRef(CurrentBrushSetName);
		if (Command.IsValid())
		{
			return Command->GetIcon();
		}
	}

	return FSlateIcon(FEditorStyle::GetStyleSetName(), "Default");
}

TSharedRef<SWidget> FLandscapeEditorDetails::GetBrushSelector()
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode && LandscapeEdMode->CurrentToolSet)
	{
		auto NameToCommandMap = FLandscapeEditorCommands::Get().NameToCommandMap;

		FToolMenuBuilder MenuBuilder(true, CommandList);
		MenuBuilder.BeginSection(NAME_None, LOCTEXT("BrushesTitle", "Brushes"));

		if (LandscapeEdMode->CurrentToolSet->ValidBrushes.Contains("BrushSet_Circle"))
		{
			MenuBuilder.AddToolButton(NameToCommandMap.FindChecked("BrushSet_Circle"), NAME_None, LOCTEXT("Brush.Circle", "Circle"), LOCTEXT("Brush.Circle.Brushtip", "Simple circular brush"));
		}
		if (LandscapeEdMode->CurrentToolSet->ValidBrushes.Contains("BrushSet_Alpha"))
		{
			MenuBuilder.AddToolButton(NameToCommandMap.FindChecked("BrushSet_Alpha"), NAME_None, LOCTEXT("Brush.Alpha.Alpha", "Alpha"), LOCTEXT("Brush.Alpha.Alpha.Tooltip", "Alpha brush, orients a mask image with the brush stroke"));
		}
		if (LandscapeEdMode->CurrentToolSet->ValidBrushes.Contains("BrushSet_Pattern"))
		{
			MenuBuilder.AddToolButton(NameToCommandMap.FindChecked("BrushSet_Pattern"), NAME_None, LOCTEXT("Brush.Alpha.Pattern", "Pattern"), LOCTEXT("Brush.Alpha.Pattern.Tooltip", "Pattern brush, tiles a mask image across the landscape"));
		}
		if (LandscapeEdMode->CurrentToolSet->ValidBrushes.Contains("BrushSet_Component"))
		{
			MenuBuilder.AddToolButton(NameToCommandMap.FindChecked("BrushSet_Component"), NAME_None, LOCTEXT("Brush.Component", "Component"), LOCTEXT("Brush.Component.Brushtip", "Work with entire landscape components"));
		}
		if (LandscapeEdMode->CurrentToolSet->ValidBrushes.Contains("BrushSet_Gizmo"))
		{
			MenuBuilder.AddToolButton(NameToCommandMap.FindChecked("BrushSet_Gizmo"), NAME_None, LOCTEXT("Brush.Gizmo", "Gizmo"), LOCTEXT("Brush.Gizmo.Brushtip", "Work with the landscape gizmo, used for copy/pasting landscape"));
		}
		//if (LandscapeEdMode->CurrentToolSet->ValidBrushes.Contains("BrushSet_Splines"))
		//{
		//	MenuBuilder.AddToolButton(NameToCommandMap.FindChecked("BrushSet_Splines"), NAME_None, LOCTEXT("Brush.Splines", "Splines"), LOCTEXT("Brush.Splines.Brushtip", "Edit Splines"));
		//}
		MenuBuilder.EndSection();

		return MenuBuilder.MakeWidget();
	}

	return SNullWidget::NullWidget;
}

bool FLandscapeEditorDetails::GetBrushSelectorIsVisible() const
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode && LandscapeEdMode->CurrentToolSet)
	{
		if (LandscapeEdMode->CurrentToolSet->ValidBrushes.Num() >= 2)
		{
			return true;
		}
	}

	return false;
}

void FLandscapeEditorDetails::OnChangeBrushSet(FName BrushSetName)
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode != NULL)
	{
		LandscapeEdMode->SetCurrentBrushSet(BrushSetName);
	}
}

bool FLandscapeEditorDetails::IsBrushSetEnabled(FName BrushSetName) const
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode && LandscapeEdMode->CurrentToolSet)
	{
		return LandscapeEdMode->CurrentToolSet->ValidBrushes.Contains(BrushSetName);
	}

	return false;
}

FText FLandscapeEditorDetails::GetCurrentBrushFalloffName() const
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode != NULL && LandscapeEdMode->CurrentBrush != NULL)
	{
		const FName CurrentBrushName = LandscapeEdMode->CurrentBrush->GetBrushName();
		return GetLocalizedName(CurrentBrushName);
	}

	return LOCTEXT("Unknown", "Unknown");
}

FSlateIcon FLandscapeEditorDetails::GetCurrentBrushFalloffIcon() const
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode != NULL && LandscapeEdMode->CurrentBrush != NULL)
	{
		const FName CurrentBrushName = LandscapeEdMode->CurrentBrush->GetBrushName();
		TSharedPtr<FUICommandInfo> Command = FLandscapeEditorCommands::Get().NameToCommandMap.FindRef(CurrentBrushName);
		if (Command.IsValid())
		{
			return Command->GetIcon();
		}
	}

	return FSlateIcon(FEditorStyle::GetStyleSetName(), "Default");
}

TSharedRef<SWidget> FLandscapeEditorDetails::GetBrushFalloffSelector()
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode && LandscapeEdMode->CurrentToolSet)
	{
		auto NameToCommandMap = FLandscapeEditorCommands::Get().NameToCommandMap;

		FToolMenuBuilder MenuBuilder(true, CommandList);
		MenuBuilder.BeginSection(NAME_None, LOCTEXT("FalloffTitle", "Falloff"));
		MenuBuilder.AddToolButton(FLandscapeEditorCommands::Get().CircleBrush_Smooth,    NAME_None, LOCTEXT("Brush.Circle.Smooth", "Smooth"),       LOCTEXT("Brush.Circle.Smooth.Tooltip", "Smooth falloff"));
		MenuBuilder.AddToolButton(FLandscapeEditorCommands::Get().CircleBrush_Linear,    NAME_None, LOCTEXT("Brush.Circle.Linear", "Linear"),       LOCTEXT("Brush.Circle.Linear.Tooltip", "Sharp, linear falloff"));
		MenuBuilder.AddToolButton(FLandscapeEditorCommands::Get().CircleBrush_Spherical, NAME_None, LOCTEXT("Brush.Circle.Spherical", "Spherical"), LOCTEXT("Brush.Circle.Spherical.Tooltip", "Spherical falloff, smooth at the center and sharp at the edge"));
		MenuBuilder.AddToolButton(FLandscapeEditorCommands::Get().CircleBrush_Tip,       NAME_None, LOCTEXT("Brush.Circle.Tip", "Tip"),             LOCTEXT("Brush.Circle.Tip.Tooltip", "Tip falloff, sharp at the center and smooth at the edge"));
		MenuBuilder.EndSection();

		return MenuBuilder.MakeWidget();
	}

	return SNullWidget::NullWidget;
}

bool FLandscapeEditorDetails::GetBrushFalloffSelectorIsVisible() const
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode && LandscapeEdMode->CurrentBrush != NULL)
	{
		const FLandscapeBrushSet& CurrentBrushSet = LandscapeEdMode->LandscapeBrushSets[LandscapeEdMode->CurrentBrushSetIndex];

		if (CurrentBrushSet.Brushes.Num() >= 2)
		{
			return true;
		}
	}

	return false;
}

void FLandscapeEditorDetails::OnChangeBrush(FName BrushName)
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode != NULL)
	{
		LandscapeEdMode->SetCurrentBrush(BrushName);
	}
}

bool FLandscapeEditorDetails::IsBrushActive(FName BrushName) const
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode && LandscapeEdMode->CurrentBrush)
	{
		const FName CurrentBrushName = LandscapeEdMode->CurrentBrush->GetBrushName();
		return CurrentBrushName == BrushName;
	}

	return false;
}

#undef LOCTEXT_NAMESPACE
