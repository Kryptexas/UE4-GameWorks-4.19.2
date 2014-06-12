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

	void InitBehaviorTreeEditor( const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UObject* InObject );

	// Begin IToolkit interface
	virtual FName GetToolkitFName() const OVERRIDE;
	virtual FText GetBaseToolkitName() const OVERRIDE;
	virtual FString GetWorldCentricTabPrefix() const OVERRIDE;
	virtual FLinearColor GetWorldCentricTabColorScale() const OVERRIDE;
	virtual FText GetToolkitName() const OVERRIDE;
	// End IToolkit interface

	// Begin IBehaviorTreeEditor interface
	virtual uint32 GetSelectedNodesCount() const OVERRIDE { return SelectedNodesCount; }
	virtual void InitializeDebuggerState(class FBehaviorTreeDebugger* ParentDebugger) const OVERRIDE;
	virtual UEdGraphNode* FindInjectedNode(int32 Index) const OVERRIDE;
	virtual void DoubleClickNode(class UEdGraphNode* Node) OVERRIDE;
	virtual void FocusWindow(UObject* ObjectToFocusOn = NULL) OVERRIDE;
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
	bool IsPropertyEditable() const;
	void OnPackageSaved(const FString& PackageFileName, UObject* Outer);
	void OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent);
	void OnClassListUpdated();

	void UpdateToolbar();
	bool IsDebuggerReady() const;

	TSharedRef<class SWidget> OnGetDebuggerActorsMenu();
	void OnDebuggerActorSelected(TWeakObjectPtr<UBehaviorTreeComponent> InstanceToDebug);
	FString GetDebuggerActorDesc() const;
	FGraphAppearanceInfo GetGraphAppearance() const;
	bool InEditingMode(bool bGraphIsEditable) const;

	void DebuggerSwitchAsset(UBehaviorTree* NewAsset);
	void DebuggerUpdateGraph();

	EVisibility GetDebuggerDetailsVisibility() const;
	EVisibility GetRangeLowerVisibility() const;
	EVisibility GetRangeSelfVisibility() const;
	EVisibility GetInjectedNodeVisibility() const;

	TWeakPtr<SGraphEditor> GetFocusedGraphPtr() const;

	/** Check whether the behavior tree mode can be accessed (i.e whether we have a valid tree to edit) */
	bool CanAccessBehaviorTreeMode() const;

	/** Check whether the blackboard mode can be accessed (i.e whether we have a valid blackboard to edit) */
	bool CanAccessBlackboardMode() const;

	/** 
	 * Get the localized text to display for the specified mode 
	 * @param	InMode	The mode to display
	 * @return the localized text representation of the mode
	 */
	static FText GetLocalizedMode(FName InMode);

	/** Access the toolbar builder for this editor */
	TSharedPtr<class FBehaviorTreeEditorToolbar> GetToolbarBuilder() { return ToolbarBuilder; }

	/** Get the behavior tree we are editing (if any) */
	class UBehaviorTree* GetBehaviorTree() const;

	/** Get the blackboard we are editing (if any) */
	class UBlackboardData* GetBlackboardData() const;

	/** Spawns the tab with the update graph inside */
	TSharedRef<SWidget> SpawnProperties();

	/** Spawns the search tab */
	TSharedRef<SWidget> SpawnSearch();

	// @todo This is a hack for now until we reconcile the default toolbar with application modes [duplicated from counterpart in Blueprint Editor]
	void RegisterToolbarTab(const TSharedRef<class FTabManager>& TabManager);

	/** Restores the behavior tree graph we were editing or creates a new one if none is available */
	void RestoreBehaviorTree();

	/** Save the graph state for later editing */
	void SaveEditedObjectState();

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

	/** Setup commands */
	void BindDebuggerToolbarCommands();

	/** Called when the selection changes in the GraphEditor */
	void OnSelectedNodesChanged(const TSet<class UObject*>& NewSelection);

	/** prepare range of nodes that can be aborted by this decorator */
	void GetAbortModePreview(const class UBehaviorTreeGraphNode_CompositeDecorator* Node, struct FAbortDrawHelper& Mode0, struct FAbortDrawHelper& Mode1);

	/** prepare range of nodes that can be aborted by this decorator */
	void GetAbortModePreview(const class UBTDecorator* DecoratorOb, struct FAbortDrawHelper& Mode0, struct FAbortDrawHelper& Mode1);

	/** Refresh the debugger's display */
	void RefreshDebugger();

	TSharedPtr<FDocumentTracker> DocumentManager;
	TWeakPtr<FDocumentTabFactory> GraphEditorTabFactoryPtr;

	/* The Behavior Tree being edited */
	UBehaviorTree* BehaviorTree;

	/* The Blackboard Data being edited */
	UBlackboardData* BlackboardData;

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

	uint32 bShowDecoratorRangeLower : 1;
	uint32 bShowDecoratorRangeSelf : 1;
	uint32 bForceDisablePropertyEdit : 1;
	uint32 bSelectedNodeIsInjected : 1;
	uint32 SelectedNodesCount;

	TSharedPtr<class FBehaviorTreeEditorToolbar> ToolbarBuilder;

public:
	/** Modes in mode switcher */
	static const FName BehaviorTreeMode;
	static const FName BlackboardMode;
};
