// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#define LOCTEXT_NAMESPACE "BlueprintEditor"

#include "SUserDefinedStructureEditor.h"

/////////////////////////////////////////////////////
// FLocalKismetCallbacks

struct FLocalKismetCallbacks
{
	static FText GetObjectName(UObject* Object)
	{
		return (Object != NULL) ? FText::FromString( Object->GetName() ) : LOCTEXT("UnknownObjectName", "UNKNOWN");
	}

	static FText GetGraphDisplayName(UEdGraph* Graph)
	{
		FGraphDisplayInfo Info;
		Graph->GetSchema()->GetGraphDisplayInformation(*Graph, /*out*/ Info);

		return Info.DisplayName;
	}

	static void RecompileGraphEditor_OnClicked()
	{
		FAssetEditorManager::Get().CloseAllAssetEditors();
		GEngine->DeferredCommands.Add( TEXT( "Module Recompile GraphEditor" ) );
	}

	static void RecompileKismetCompiler_OnClicked()
	{
		GEngine->DeferredCommands.Add( TEXT( "Module Recompile KismetCompiler" ) );
	}

	static void RecompileBlueprintEditor_OnClicked()
	{
		FAssetEditorManager::Get().CloseAllAssetEditors();
		GEngine->DeferredCommands.Add( TEXT( "Module Recompile Kismet" ) );
	}

	static void RecompilePersona_OnClicked()
	{
		FAssetEditorManager::Get().CloseAllAssetEditors();
		GEngine->DeferredCommands.Add( TEXT( "Module Recompile Persona" ) );
	}

	static bool CanRecompileModules()
	{
		// We're not able to recompile if a compile is already in progress!
		return !FModuleManager::Get().IsCurrentlyCompiling();
	}
};

/////////////////////////////////////////////////////
// FDebugInfoSummoner

struct FDebugInfoSummoner : public FWorkflowTabFactory
{
public:
	FDebugInfoSummoner(TSharedPtr<class FAssetEditorToolkit> InHostingApp)
		: FWorkflowTabFactory(FBlueprintEditorTabs::DebugID, InHostingApp)
	{
		TabLabel = LOCTEXT("DebugTabTitle", "Debug");

		EnableTabPadding();
		bIsSingleton = true;

		ViewMenuDescription = LOCTEXT("DebugView", "Debug");
		ViewMenuTooltip = LOCTEXT("DebugView_ToolTip", "Shows the debugging view");
	}

	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const OVERRIDE
	{
		TSharedPtr<FBlueprintEditor> BlueprintEditorPtr = StaticCastSharedPtr<FBlueprintEditor>(HostingApp.Pin());

		return BlueprintEditorPtr->GetDebuggingView();
	}
};

/////////////////////////////////////////////////////
// FDefaultsEditorSummoner

struct FDefaultsEditorSummoner : public FWorkflowTabFactory
{
public:
	FDefaultsEditorSummoner(TSharedPtr<class FAssetEditorToolkit> InHostingApp)
		: FWorkflowTabFactory(FBlueprintEditorTabs::DefaultEditorID, InHostingApp)
	{
		TabLabel = LOCTEXT("BlueprintDefaultsTabTitle", "Blueprint Defaults"); //@TODO: ANIMATION: GetDefaultEditorTitle(); !!!
		TabIcon = FEditorStyle::GetBrush("LevelEditor.Tabs.Details");

		bIsSingleton = true;

		ViewMenuDescription = LOCTEXT("DefaultEditorView", "Defaults");
		ViewMenuTooltip = LOCTEXT("DefaultEditorView_ToolTip", "Shows the default editor view");
	}

	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const OVERRIDE
	{
		TSharedPtr<FBlueprintEditor> BlueprintEditorPtr = StaticCastSharedPtr<FBlueprintEditor>(HostingApp.Pin());

		return BlueprintEditorPtr->GetDefaultEditor();
	}

	virtual FText GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const OVERRIDE
	{
		return LOCTEXT("DefaultsEditorTooltip", "The defaults editor lets you set the default value for all variables in your Blueprint.");
	}
};

/////////////////////////////////////////////////////
// FUserDefinedStructureEditorSummoner

struct FUserDefinedStructureEditorSummoner : public FWorkflowTabFactory
{
public:
	FUserDefinedStructureEditorSummoner(TSharedPtr<class FAssetEditorToolkit> InHostingApp)
		: FWorkflowTabFactory(FBlueprintEditorTabs::UserDefinedStructureID, InHostingApp)
	{
		TabLabel = LOCTEXT("BlueprintUserDefinedStructureTabTitle", "User Defined Structure");
		TabIcon = FEditorStyle::GetBrush("LevelEditor.Tabs.Details");

		bIsSingleton = true;

		ViewMenuDescription = LOCTEXT("UserDefinedStructureEditorView", "Structures");
		ViewMenuTooltip = LOCTEXT("UserDefinedStructureEditorView_ToolTip", "Shows the user defined structures editor view");
	}

	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const OVERRIDE
	{
		TSharedPtr<FBlueprintEditor> BlueprintEditorPtr = StaticCastSharedPtr<FBlueprintEditor>(HostingApp.Pin());

		return BlueprintEditorPtr->GetUserDefinedStructureEditor();
	}
};

/////////////////////////////////////////////////////
// FGraphTabHistory

struct FGraphTabHistory : public FGenericTabHistory
{
public:
	/**
	 * @param InFactory		The factory used to regenerate the content
	 * @param InPayload		The payload object used to regenerate the content
	 */
	FGraphTabHistory(TSharedPtr<FDocumentTabFactory> InFactory, TSharedPtr<FTabPayload> InPayload)
		: FGenericTabHistory(InFactory, InPayload)
		, SavedZoomAmount(INDEX_NONE)
		, SavedLocation(FVector2D::ZeroVector)
	{

	}

	virtual void EvokeHistory(TSharedPtr<FTabInfo> InTabInfo) OVERRIDE
	{
		FWorkflowTabSpawnInfo SpawnInfo;
		SpawnInfo.Payload = Payload;
		SpawnInfo.TabInfo = InTabInfo;

		TSharedRef< SGraphEditor > GraphEditorRef = StaticCastSharedRef< SGraphEditor >(FactoryPtr.Pin()->CreateTabBody(SpawnInfo));
		GraphEditor = GraphEditorRef;

		FactoryPtr.Pin()->UpdateTab(InTabInfo->GetTab().Pin(), SpawnInfo, GraphEditorRef);
	}

	virtual void SaveHistory() OVERRIDE
	{
		if(IsHistoryValid())
		{
			check(GraphEditor.IsValid());
			GraphEditor.Pin()->GetViewLocation(SavedLocation, SavedZoomAmount);
		}
	}

	virtual void RestoreHistory() OVERRIDE
	{
		if(IsHistoryValid())
		{
			check(GraphEditor.IsValid());
			GraphEditor.Pin()->SetViewLocation(SavedLocation, SavedZoomAmount);
		}
	}

private:
	/** The graph editor represented by this history node. While this node is inactive, the graph editor is invalid */
	TWeakPtr< class SGraphEditor > GraphEditor;
	/** Saved location the graph editor was at when this history node was last visited */
	FVector2D SavedLocation;
	/** Saved zoom the graph editor was at when this history node was last visited */
	float SavedZoomAmount;
};

/////////////////////////////////////////////////////
// FGraphEditorSummoner

struct FGraphEditorSummoner : public FDocumentTabFactoryForObjects<UEdGraph>
{
public:
	DECLARE_DELEGATE_RetVal_TwoParams(TSharedRef<SGraphEditor>, FOnCreateGraphEditorWidget, TSharedRef<FTabInfo>, UEdGraph*);
public:
	FGraphEditorSummoner(TSharedPtr<class FBlueprintEditor> InBlueprintEditorPtr, FOnCreateGraphEditorWidget CreateGraphEditorWidgetCallback)
		: FDocumentTabFactoryForObjects<UEdGraph>(FBlueprintEditorTabs::GraphEditorID, InBlueprintEditorPtr)
		, BlueprintEditorPtr(InBlueprintEditorPtr)
		, OnCreateGraphEditorWidget(CreateGraphEditorWidgetCallback)
	{
	}

	virtual void OnTabActivated(TSharedPtr<SDockTab> Tab) const OVERRIDE
	{
		TSharedRef<SGraphEditor> GraphEditor = StaticCastSharedRef<SGraphEditor>(Tab->GetContent());
		BlueprintEditorPtr.Pin()->OnGraphEditorFocused(GraphEditor);
	}

	virtual void OnTabRefreshed(TSharedPtr<SDockTab> Tab) const OVERRIDE
	{
		TSharedRef<SGraphEditor> GraphEditor = StaticCastSharedRef<SGraphEditor>(Tab->GetContent());
		GraphEditor->NotifyGraphChanged();
	}

	virtual void SaveState(TSharedPtr<SDockTab> Tab, TSharedPtr<FTabPayload> Payload) const OVERRIDE
	{
		TSharedRef<SGraphEditor> GraphEditor = StaticCastSharedRef<SGraphEditor>(Tab->GetContent());

		FVector2D ViewLocation;
		float ZoomAmount;
		GraphEditor->GetViewLocation(ViewLocation, ZoomAmount);

		UEdGraph* Graph = FTabPayload_UObject::CastChecked<UEdGraph>(Payload);
		BlueprintEditorPtr.Pin()->GetBlueprintObj()->LastEditedDocuments.Add(FEditedDocumentInfo(Graph, ViewLocation, ZoomAmount));
	}

//	virtual void OnTabClosed(TSharedRef<SDockTab> Tab, TSharedPtr<FTabPayload> Payload) const OVERRIDE
//	{
//		BlueprintEditorPtr.Pin()->RequestSaveEditedObjectState();
//	}
protected:
	virtual TAttribute<FText> ConstructTabNameForObject(UEdGraph* DocumentID) const OVERRIDE
	{
		return TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateStatic<UEdGraph*>(&FLocalKismetCallbacks::GetGraphDisplayName, DocumentID));
	}

	virtual TSharedRef<SWidget> CreateTabBodyForObject(const FWorkflowTabSpawnInfo& Info, UEdGraph* DocumentID) const OVERRIDE
	{
		check(Info.TabInfo.IsValid());
		return OnCreateGraphEditorWidget.Execute(Info.TabInfo.ToSharedRef(), DocumentID);
	}

	virtual const FSlateBrush* GetTabIconForObject(const FWorkflowTabSpawnInfo& Info, UEdGraph* DocumentID) const OVERRIDE
	{
		return FBlueprintEditor::GetGlyphForGraph(DocumentID, false);
	}

	virtual TSharedRef<FGenericTabHistory> CreateTabHistoryNode(TSharedPtr<FTabPayload> Payload) OVERRIDE
	{
		return MakeShareable(new FGraphTabHistory(SharedThis(this), Payload));
	}

protected:
	TWeakPtr<class FBlueprintEditor> BlueprintEditorPtr;
	FOnCreateGraphEditorWidget OnCreateGraphEditorWidget;
};

/////////////////////////////////////////////////////
// FTimelineEditorSummoner

struct FTimelineEditorSummoner : public FDocumentTabFactoryForObjects<UTimelineTemplate>
{
public:
	FTimelineEditorSummoner(TSharedPtr<class FBlueprintEditor> InBlueprintEditorPtr)
		: FDocumentTabFactoryForObjects<UTimelineTemplate>(FBlueprintEditorTabs::TimelineEditorID, InBlueprintEditorPtr)
		, BlueprintEditorPtr(InBlueprintEditorPtr)
	{}
protected:
	virtual TAttribute<FText> ConstructTabNameForObject(UTimelineTemplate* DocumentID) const OVERRIDE
	{
		return TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateStatic<UObject*>(&FLocalKismetCallbacks::GetObjectName, DocumentID));
	}

	virtual TSharedRef<SWidget> CreateTabBodyForObject(const FWorkflowTabSpawnInfo& Info, UTimelineTemplate* DocumentID) const OVERRIDE
	{
		return SNew(STimelineEditor, BlueprintEditorPtr.Pin(), DocumentID);
	}

	virtual const FSlateBrush* GetTabIconForObject(const FWorkflowTabSpawnInfo& Info, UTimelineTemplate* DocumentID) const OVERRIDE
	{
		return FEditorStyle::GetBrush("GraphEditor.Timeline_16x");
	}

	virtual void SaveState(TSharedPtr<SDockTab> Tab, TSharedPtr<FTabPayload> Payload) const OVERRIDE
	{
		UTimelineTemplate* Timeline = FTabPayload_UObject::CastChecked<UTimelineTemplate>(Payload);
		BlueprintEditorPtr.Pin()->GetBlueprintObj()->LastEditedDocuments.Add(FEditedDocumentInfo(Timeline));
	}

// 	virtual void OnTabClosed(TSharedRef<SDockTab> Tab, TSharedPtr<FTabPayload> Payload) const OVERRIDE
// 	{
// 		BlueprintEditorPtr.Pin()->RequestSaveEditedObjectState();
// 	}

	virtual void OnTabRefreshed(TSharedPtr<SDockTab> Tab) const OVERRIDE
	{
		TSharedRef<STimelineEditor> TimelineEditor = StaticCastSharedRef<STimelineEditor>(Tab->GetContent());
		TimelineEditor->OnTimelineChanged();
	}

	virtual FText GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const OVERRIDE
	{
		return LOCTEXT("TimelineEditorTooltip", "The Timeline editor lets you add tracks and visually keyframe values over time, to be used inside a Blueprint graph.");
	}

protected:
	TWeakPtr<class FBlueprintEditor> BlueprintEditorPtr;
};

/////////////////////////////////////////////////////
// FConstructionScriptEditorSummoner

struct FConstructionScriptEditorSummoner : public FWorkflowTabFactory
{
public:
	FConstructionScriptEditorSummoner(TSharedPtr<class FAssetEditorToolkit> InHostingApp)
		: FWorkflowTabFactory(FBlueprintEditorTabs::ConstructionScriptEditorID, InHostingApp)
	{
		TabLabel = LOCTEXT("ComponentsTabLabel", "Components");
		TabIcon = FEditorStyle::GetBrush("Kismet.Tabs.Components");

		bIsSingleton = true;

		ViewMenuDescription = LOCTEXT("ComponentsView", "Components");
		ViewMenuTooltip = LOCTEXT("ComponentsView_ToolTip", "Show the components view");
	}

	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const OVERRIDE
	{
		TSharedPtr<FBlueprintEditor> BlueprintEditorPtr = StaticCastSharedPtr<FBlueprintEditor>(HostingApp.Pin());

		return BlueprintEditorPtr->GetSCSEditor();
	}

	virtual FText GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const OVERRIDE
	{
		return LOCTEXT("ComponentsTabTooltip", "The Components tab is for easily adding, selecting and attaching Components within your Blueprint.");
	}
};

/////////////////////////////////////////////////////
// FConstructionScriptEditorSummoner

struct FSCSViewportSummoner : public FWorkflowTabFactory
{
public:
	FSCSViewportSummoner(TSharedPtr<class FAssetEditorToolkit> InHostingApp)
		: FWorkflowTabFactory(FBlueprintEditorTabs::SCSViewportID, InHostingApp)
	{
		TabLabel = LOCTEXT("SCSViewportTabLabel", "Viewport");

		bIsSingleton = true;

		ViewMenuDescription = LOCTEXT("SCSViewportView", "Viewport");
		ViewMenuTooltip = LOCTEXT("SCSViewportView_ToolTip", "Show the viewport view");
	}

	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const OVERRIDE
	{
		TSharedPtr<FBlueprintEditor> BlueprintEditorPtr = StaticCastSharedPtr<FBlueprintEditor>(HostingApp.Pin());

		TSharedPtr<SWidget> Result;
		if (BlueprintEditorPtr->CanAccessComponentsMode())
		{
			Result = BlueprintEditorPtr->GetSCSViewport();
		}

		if (Result.IsValid())
		{
			return Result.ToSharedRef();
		}
		else
		{
			return SNew(SErrorText)
					.BackgroundColor(FLinearColor::Transparent)
					.ErrorText(LOCTEXT("SCSViewportView_Unavailable", "Viewport is not available for this Blueprint.").ToString());
		}
	}
};

//////////////////////////////////////////////////////////////////////////
// FPaletteSummoner

struct FPaletteSummoner : public FWorkflowTabFactory
{
public:
	FPaletteSummoner(TSharedPtr<class FAssetEditorToolkit> InHostingApp)
		: FWorkflowTabFactory(FBlueprintEditorTabs::PaletteID, InHostingApp)
	{
		TabLabel = LOCTEXT("PaletteTabTitle", "Palette");
		TabIcon = FEditorStyle::GetBrush("Kismet.Tabs.Palette");

		bIsSingleton = true;

		ViewMenuDescription = LOCTEXT("PaletteView", "Palette");
		ViewMenuTooltip = LOCTEXT("PaletteView_ToolTip", "Show palette of all functions and variables");
	}

	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const OVERRIDE
	{
		TSharedPtr<FBlueprintEditor> BlueprintEditorPtr = StaticCastSharedPtr<FBlueprintEditor>(HostingApp.Pin());

		return BlueprintEditorPtr->GetPalette();
	}

	virtual FText GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const OVERRIDE
	{
		return LOCTEXT("PaletteTooltip", "The Palette tab provides access to _all_ nodes that can be placed (functions, variables, events etc).");
	}
};

/////////////////////////////////////////////////////
// FMyBlueprintSummoner

struct FMyBlueprintSummoner : public FWorkflowTabFactory
{
public:
	FMyBlueprintSummoner(TSharedPtr<class FAssetEditorToolkit> InHostingApp)
		: FWorkflowTabFactory(FBlueprintEditorTabs::MyBlueprintID, InHostingApp)
	{
		TabLabel = LOCTEXT("MyBlueprintTabLabel", "My Blueprint");
		TabIcon = FEditorStyle::GetBrush("Kismet.Tabs.Palette");

		bIsSingleton = true;

		ViewMenuDescription = LOCTEXT("MyBlueprintTabView", "My Blueprint");
		ViewMenuTooltip = LOCTEXT("MyBlueprintTabView_ToolTip", "Show the my blueprint view");
	}

	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const OVERRIDE
	{
		TSharedPtr<FBlueprintEditor> BlueprintEditorPtr = StaticCastSharedPtr<FBlueprintEditor>(HostingApp.Pin());

		return BlueprintEditorPtr->GetMyBlueprintWidget().ToSharedRef();
	}

	virtual FText GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const OVERRIDE
	{
		return LOCTEXT("MyBlueprintTooltip", "The MyBlueprint tab shows you elements that belong to _this_ Blueprint (variables, graphs etc.)");
	}
};

//////////////////////////////////////////////////////////////////////////
// FCompilerResultsSummoner

struct FCompilerResultsSummoner : public FWorkflowTabFactory
{
public:
	FCompilerResultsSummoner(TSharedPtr<class FAssetEditorToolkit> InHostingApp)
		: FWorkflowTabFactory(FBlueprintEditorTabs::CompilerResultsID, InHostingApp)
	{
		TabLabel = LOCTEXT("CompilerResultsTabTitle", "Compiler Results");
		TabIcon = FEditorStyle::GetBrush("Kismet.Tabs.CompilerResults");

		bIsSingleton = true;

		ViewMenuDescription = LOCTEXT("CompilerResultsView", "Compiler Results");
		ViewMenuTooltip = LOCTEXT("CompilerResultsView_ToolTip", "Show compiler results of all functions and variables");
	}

	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const OVERRIDE
	{
		TSharedPtr<FBlueprintEditor> BlueprintEditorPtr = StaticCastSharedPtr<FBlueprintEditor>(HostingApp.Pin());

		return BlueprintEditorPtr->GetCompilerResults();
	}

	virtual FText GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const OVERRIDE
	{
		return LOCTEXT("CompilerResultsTooltip", "The compiler results tab shows any errors or warnings generated when compiling this Blueprint.");
	}
};

//////////////////////////////////////////////////////////////////////////
// FFindResultsSummoner

struct FFindResultsSummoner : public FWorkflowTabFactory
{
public:
	FFindResultsSummoner(TSharedPtr<class FAssetEditorToolkit> InHostingApp)
		: FWorkflowTabFactory(FBlueprintEditorTabs::FindResultsID, InHostingApp)
	{
		TabLabel = LOCTEXT("FindResultsTabTitle", "Find Results");
		TabIcon = FEditorStyle::GetBrush("Kismet.Tabs.FindResults");

		bIsSingleton = true;

		ViewMenuDescription = LOCTEXT("FindResultsView", "Find Results");
		ViewMenuTooltip = LOCTEXT("FindResultsView_ToolTip", "Show find results for searching in this blueprint or all blueprints");
	}

	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const OVERRIDE
	{
		TSharedPtr<FBlueprintEditor> BlueprintEditorPtr = StaticCastSharedPtr<FBlueprintEditor>(HostingApp.Pin());

		return BlueprintEditorPtr->GetFindResults();
	}

	virtual FText GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const OVERRIDE
	{
		return LOCTEXT("FindResultsTooltip", "The find results tab shows results of searching this Blueprint (or all Blueprints).");
	}
};

#undef LOCTEXT_NAMESPACE
