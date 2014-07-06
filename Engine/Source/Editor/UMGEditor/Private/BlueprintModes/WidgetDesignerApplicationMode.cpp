// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
#include "Editor/Kismet/Public/BlueprintEditorTabs.h"
#include "Editor/Kismet/Public/SBlueprintEditorToolbar.h"
#include "Editor/Kismet/Public/BlueprintEditorModes.h"
#include "SKismetInspector.h"

#include "WidgetBlueprintEditor.h"
#include "WidgetDesignerApplicationMode.h"
#include "WidgetBlueprintEditorToolbar.h"

#include "IDetailsView.h"

#include "BlueprintEditorTabs.h"
#include "PaletteTabSummoner.h"
#include "HierarchyTabSummoner.h"
#include "DesignerTabSummoner.h"
#include "SequencerTabSummoner.h"

/////////////////////////////////////////////////////
// FWidgetDesignerApplicationMode

FWidgetDesignerApplicationMode::FWidgetDesignerApplicationMode(TSharedPtr<FWidgetBlueprintEditor> InWidgetEditor)
	: FWidgetBlueprintApplicationMode(InWidgetEditor, FWidgetBlueprintApplicationModes::DesignerMode)
{
	TabLayout = FTabManager::NewLayout( "WidgetBlueprintEditor_Designer_Layout_v1" )
	->AddArea
	(
		FTabManager::NewPrimaryArea()
		->SetOrientation(Orient_Vertical)
		->Split
		(
			FTabManager::NewStack()
			->SetSizeCoefficient( 0.2f )
			->SetHideTabWell(true)
			->AddTab( InWidgetEditor->GetToolbarTabId(), ETabState::OpenedTab )
		)
		->Split
		(
			FTabManager::NewSplitter()
			->SetOrientation(Orient_Horizontal)
			->Split
			(
				FTabManager::NewSplitter()
				->SetSizeCoefficient( 0.15f )
				->SetOrientation(Orient_Vertical)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient( 0.5f )
					->AddTab( FPaletteTabSummoner::TabID, ETabState::OpenedTab )
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient( 0.5f )
					->AddTab( FHierarchyTabSummoner::TabID, ETabState::OpenedTab )
				)
			)
			->Split
			(
				FTabManager::NewSplitter()
				->SetSizeCoefficient( 0.85f )
				->SetOrientation(Orient_Horizontal)
				->Split
				(
					FTabManager::NewStack()
					->SetHideTabWell(true)
					->AddTab( FDesignerTabSummoner::TabID, ETabState::OpenedTab )
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient( 0.5f )
					->AddTab( FBlueprintEditorTabs::DetailsID, ETabState::OpenedTab )
				)
			)
		)
		->Split
		(
			FTabManager::NewStack()
			->SetSizeCoefficient(0.2f)
			->SetHideTabWell(true)
			->AddTab( FSequencerTabSummoner::TabID, ETabState::OpenedTab )
		)
	);


	// Add Tab Spawners
	TabFactories.RegisterFactory(MakeShareable(new FSelectionDetailsSummoner(InWidgetEditor)));
	TabFactories.RegisterFactory(MakeShareable(new FDesignerTabSummoner(InWidgetEditor)));
	TabFactories.RegisterFactory(MakeShareable(new FHierarchyTabSummoner(InWidgetEditor)));
	TabFactories.RegisterFactory(MakeShareable(new FPaletteTabSummoner(InWidgetEditor)));
	TabFactories.RegisterFactory(MakeShareable(new FSequencerTabSummoner(InWidgetEditor)));

	// setup toolbar - clear existing toolbar extender from the BP mode
	//@TODO: Keep this in sync with BlueprintEditorModes.cpp
	ToolbarExtender = MakeShareable(new FExtender);
	InWidgetEditor->GetWidgetToolbarBuilder()->AddWidgetBlueprintEditorModesToolbar(ToolbarExtender);
	InWidgetEditor->GetToolbarBuilder()->AddCompileToolbar(ToolbarExtender);
	//InPersona->GetToolbarBuilder()->AddScriptingToolbar(ToolbarExtender);
	//InPersona->GetToolbarBuilder()->AddBlueprintGlobalOptionsToolbar(ToolbarExtender);
	//InPersona->GetToolbarBuilder()->AddDebuggingToolbar(ToolbarExtender);
}

void FWidgetDesignerApplicationMode::RegisterTabFactories(TSharedPtr<FTabManager> InTabManager)
{
	TSharedPtr<FWidgetBlueprintEditor> BP = GetBlueprintEditor();
	
	BP->RegisterToolbarTab(InTabManager.ToSharedRef());
	BP->PushTabFactories(TabFactories);
}

void FWidgetDesignerApplicationMode::PreDeactivateMode()
{
	//FWidgetBlueprintApplicationMode::PreDeactivateMode();

	GetBlueprintEditor()->GetInspector()->EnableComponentDetailsCustomization(false);

	static FName PropertyEditor("PropertyEditor");
	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(PropertyEditor);
	PropertyModule.UnregisterCustomPropertyTypeLayout(TEXT("CanvasPanelSlot"));
}

void FWidgetDesignerApplicationMode::PostActivateMode()
{
	//FWidgetBlueprintApplicationMode::PostActivateMode();

	GetBlueprintEditor()->GetInspector()->EnableComponentDetailsCustomization(false);

	TSharedRef<class SKismetInspector> Inspector = MyBlueprintEditor.Pin()->GetInspector();
	FOnGetDetailCustomizationInstance LayoutDelegateDetails = FOnGetDetailCustomizationInstance::CreateStatic(&FBlueprintWidgetCustomization::MakeInstance, MyBlueprintEditor.Pin()->GetBlueprintObj());
	Inspector->GetPropertyView()->RegisterInstancedCustomPropertyLayout(UWidget::StaticClass(), LayoutDelegateDetails);

	FOnGetPropertyTypeCustomizationInstance CanvasSlotCustomization = FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FCanvasSlotCustomization::MakeInstance, MyBlueprintEditor.Pin()->GetBlueprintObj());

	static FName PropertyEditor("PropertyEditor");
	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(PropertyEditor);
	PropertyModule.RegisterCustomPropertyTypeLayout(TEXT("PanelSlot"), CanvasSlotCustomization);
}
