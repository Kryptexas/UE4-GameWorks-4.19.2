// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IBehaviorTreeEditor.h"
#include "Toolkits/AssetEditorToolkit.h"

class FBehaviorTreeEditor : public IBehaviorTreeEditor, public FEditorUndoClient
{
public:
	FBehaviorTreeEditor();
	/** Destructor */
	virtual ~FBehaviorTreeEditor();

	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) OVERRIDE;
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) OVERRIDE;

	void InitBehaviorTreeEditor( const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UBehaviorTree* Script );

	// Begin IToolkit interface
	virtual FName GetToolkitFName() const OVERRIDE;
	virtual FText GetBaseToolkitName() const OVERRIDE;
	virtual FString GetWorldCentricTabPrefix() const OVERRIDE;
	virtual FLinearColor GetWorldCentricTabColorScale() const OVERRIDE;
	// End IToolkit interface

	// Begin IBehaviorTreeEditor interface
	virtual uint32 GetSelectedNodesCount() const OVERRIDE { return SelectedNodesCount; }
	virtual void InitializeDebuggerState(class FBehaviorTreeDebugger* ParentDebugger) const OVERRIDE;
	// End IBehaviorTreeEditor interface

	// Begin FEditorUndoClient Interface
	virtual void	PostUndo(bool bSuccess) OVERRIDE;
	virtual void	PostRedo(bool bSuccess) OVERRIDE;
	// End of FEditorUndoClient

	// Delegates
	void OnNodeDoubleClicked(class UEdGraphNode* Node);
	void OnGraphEditorFocused(const TSharedRef<SGraphEditor>& InGraphEditor);
	FGraphPanelSelectionSet GetSelectedNodes() const;

	void OnAddInputPin();
	bool CanAddInputPin() const;
	void OnRemoveInputPin();
	bool CanRemoveInputPin() const;

	void OnEnableBreakpoint();
	bool CanEnableBreakpoint() const;
	void OnToggleBreakpoint();
	bool CanToggleBreakpoint() const;
	void OnDisableBreakpoint();
	bool CanDisableBreakpoint() const;
	void OnAddBreakpoint();
	bool CanAddBreakpoint() const;
	void OnRemoveBreakpoint();
	bool CanRemoveBreakpoint() const;
	
	void SelectAllNodes();
	bool CanSelectAllNodes() const;
	void DeleteSelectedNodes();
	bool CanDeleteNodes() const;
	void DeleteSelectedDuplicatableNodes();
	void CutSelectedNodes();
	bool CanCutNodes() const;
	void CopySelectedNodes();
	bool CanCopyNodes() const;
	void PasteNodes();
	void PasteNodesHere(const FVector2D& Location);
	bool CanPasteNodes() const;
	void DuplicateNodes();
	bool CanDuplicateNodes() const;

	void SearchTree();
	bool CanSearchTree() const;

	void JumpToNode(const UEdGraphNode* Node);

	bool IsPropertyVisible(UProperty const * const InProperty) const;
	void OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent);

	void UpdateToolbar();
	bool IsDebuggerReady() const;

	TSharedRef<class SWidget> OnGetDebuggerActorsMenu();
	void OnDebuggerActorSelected(TWeakObjectPtr<UBehaviorTreeComponent> InstanceToDebug);
	FString GetDebuggerActorDesc() const;
	FGraphAppearanceInfo GetGraphAppearance() const;
	bool InEditingMode() const;

	void DebuggerSwitchAsset(UBehaviorTree* NewAsset);

	EVisibility GetDebuggerDetailsVisibility() const;
	EVisibility GetRangeLowerVisibility() const;
	EVisibility GetRangeSelfVisibility() const;

	TWeakPtr<SGraphEditor> GetFocusedGraphPtr() const;

protected:
	/** Called when "Save" is clicked for this asset */
	virtual void SaveAsset_Execute() OVERRIDE;

private:
	/** Create widget for graph editing */
	TSharedRef<class SGraphEditor> CreateGraphEditorWidget(UEdGraph* InGraph);

	/** Creates all internal widgets for the tabs to point at */
	void CreateInternalWidgets();

	/** Add custom menu options */
	void ExtendMenu();

	/** Setup common commands */
	void BindCommonCommands();

	/** Add custom widgets to toolbar */
	void ExtentToolbar();

	/** Setup commands */
	void BindDebuggerToolbarCommands();

	/** Called when the selection changes in the GraphEditor */
	void OnSelectedNodesChanged(const TSet<class UObject*>& NewSelection);

	/** prepare range of nodes that can be aborted by this decorator */
	void GetAbortModePreview(const class UBehaviorTreeGraphNode_CompositeDecorator* Node, struct FAbortDrawHelper& Mode0, struct FAbortDrawHelper& Mode1);

	/** prepare range of nodes that can be aborted by this decorator */
	void GetAbortModePreview(const class UBTDecorator* DecoratorOb, struct FAbortDrawHelper& Mode0, struct FAbortDrawHelper& Mode1);

	/** Spawns the tab with the update graph inside */
	TSharedRef<SDockTab> SpawnTab_Properties(const FSpawnTabArgs& Args);

	/** Spawns the search tab */
	TSharedRef<SDockTab> SpawnTab_Search(const FSpawnTabArgs& Args);

	FDocumentTracker DocumentManager;
	TWeakPtr<FDocumentTabFactory> GraphEditorTabFactoryPtr;

	/* The Script being edited */
	UBehaviorTree* Script;

	TWeakPtr<SGraphEditor> FocusedGraphEdPtr;
	TWeakObjectPtr<class UBehaviorTreeGraphNode_CompositeDecorator> FocusedGraphOwner;

	/** Property View */
	TSharedPtr<class IDetailsView> DetailsView;
	TSharedPtr<class SBehaviorTreeDebuggerView> DebuggerView;

	/** The command list for this editor */
	TSharedPtr<FUICommandList> GraphEditorCommands;

	TSharedPtr<class FBehaviorTreeDebugger> Debugger;

	/** Find results log as well as the search filter */
	TSharedPtr<class SFindInBT> FindResults;

	bool bShowDecoratorRangeLower;
	bool bShowDecoratorRangeSelf;
	uint32 SelectedNodesCount;

public:
	/**	Graph editor tab */
	static const FName BTGraphTabId;
	static const FName BTPropertiesTabId;
	static const FName BTSearchTabId;
};
