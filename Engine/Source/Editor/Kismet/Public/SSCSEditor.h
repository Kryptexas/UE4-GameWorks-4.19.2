// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BlueprintEditor.h"
#include "SComponentClassCombo.h"

class SSCSEditor;
class USCS_Node;
class USimpleConstructionScript;

// SCS tree node pointer type
typedef TSharedPtr<class FSCSEditorTreeNode> FSCSEditorTreeNodePtrType;

/**
 * FSCSEditorTreeNodeBase
 *
 * Wrapper class for component template nodes displayed in the SCS editor tree widget.
 */
class KISMET_API FSCSEditorTreeNode : public TSharedFromThis<FSCSEditorTreeNode>
{
public:

	enum ENodeType
	{
		ComponentNode,
		RootActorNode,
		SeparatorNode,
	};

	/** 
	 * Constructs an empty tree node.
	 */
	FSCSEditorTreeNode(FSCSEditorTreeNode::ENodeType InNodeType);

	/**
	 * Constructs a wrapper around a component template contained within an SCS tree node.
	 *
	 * @param InSCSNode The SCS tree node represented by this object.
	 * @param bInIsInherited Whether or not the SCS tree node is inherited from a parent Blueprint class.
	 */
	FSCSEditorTreeNode(class USCS_Node* InSCSNode, bool bInIsInherited = false);

	/**
	 * Constructs a wrapper around a component template not contained within an SCS tree node (e.g. "native" components).
	 *
	 * @param InComponentTemplate The component template represented by this object.
	 */
	FSCSEditorTreeNode(UActorComponent* InComponentTemplate);

	/** 
	 * @return The name of the variable represented by this node.
	 */
	FName GetVariableName() const;
	/**
	 * @return The string to be used in the tree display.
	 */
	FString GetDisplayString() const;
	/**
	* @return The name of this node in text.
	*/
	FText GetDisplayName() const;
	/** 
     * @return The SCS node that is represented by this object, or NULL if there is no SCS node associated with the component template.
	 */
	class USCS_Node* GetSCSNode() const { return SCSNodePtr.Get(); }
	/**
	 * @return The component template or instance represented by this object.
	 */
	UActorComponent* GetComponentTemplate() const;
	/**
	 * @param ActualEditedBlueprint currently edited blueprint
	 *
	 * @return The component template that can be editable for actual class.
	 */
	UActorComponent* GetEditableComponentTemplate(UBlueprint* ActualEditedBlueprint);
	/**
	 * Finds the component instance represented by this node contained within a given Actor instance.
	 *
	 * @param InActor The Actor instance to use as the container object for finding the component instance.
	 * @return The component instance represented by this node and contained within the given Actor instance, or NULL if not found.
	 */
	UActorComponent* FindComponentInstanceInActor(const AActor* InActor) const;
	/** 
	 * @return This object's parent node (or an invalid reference if no parent is assigned).
	 */
	FSCSEditorTreeNodePtrType GetParent() const { return ParentNodePtr; }
	/**
	 * @return The set of nodes which are parented to this node (read-only).
	 */
	const TArray<FSCSEditorTreeNodePtrType>& GetChildren() const { return Children; }
	/**
	 * @return The Blueprint to which this node belongs.
	 */
	UBlueprint* GetBlueprint() const;
	/**
	 * @return Type of node
	 */
	ENodeType GetNodeType() const;
	/**
	 * @return Whether or not this object represents a root component.
	 */
	bool IsRootComponent() const;
	/**
	 * @return Whether or not this node is a direct child of the given node.
	 */
	bool IsDirectlyAttachedTo(FSCSEditorTreeNodePtrType InNodePtr) const { return ParentNodePtr == InNodePtr; } 
	/**
	 * @return Whether or not this node is a child (direct or indirect) of the given node.
	 */
	bool IsAttachedTo(FSCSEditorTreeNodePtrType InNodePtr) const;
	/**
	 * @return Whether or not this object represents a "native" component template (i.e. one that is not found in the SCS tree).
	 */
	bool IsNative() const { return GetSCSNode() == NULL && GetComponentTemplate() != NULL; }
	/** 
	 * @return Whether or not this object represents a component template that was inherited from a parent Blueprint class SCS.
	 */
	bool IsInherited() const { return bIsInherited; }
	/**
	 * @return Whether or not this object represents a component instance rather than a template.
	 */
	bool IsInstanced() const { return bIsInstanced; }
	/**
	 * @return Whether or not this object represents a component instance that was created by the user and not by a native or Blueprint-generated class.
	 */
	bool IsUserInstanced() const;
	/** 
	 * @return Whether or not this object represents the default SCS scene root component.
	 */
	bool IsDefaultSceneRoot() const;
	/** 
	 * @return Whether or not this object represents a node that can be deleted from the SCS tree.
	 */
	bool CanDelete() const { return (IsUserInstanced() || (!IsNative() && !IsInherited())) && !IsDefaultSceneRoot(); }
	/** 
	 * @return Whether or not this object represents a node that can be renamed from the SCS tree.
	 */
	bool CanRename() const { return (IsUserInstanced() || (!IsNative() && !IsInherited())) && !IsDefaultSceneRoot(); }
	/** 
	 * @return Whether or not this object represents a node that can be reparented to other nodes based on its context.
	 */
	bool CanReparent() const { return (IsUserInstanced() || (!IsNative() && !IsInherited())) && !IsDefaultSceneRoot() && Cast<USceneComponent>(GetComponentTemplate()) != NULL; }
	/** 
	 * @return Whether or not we can edit default properties for the component template represented by this object.
	 */
	bool CanEditDefaults() const;

	/**
	 * Finds the closest ancestor node in the given node set.
	 * 
	 * @param InNodes The given node set.
	 * @return One of the nodes from the set, or an invalid node reference if the set does not contain any ancestor nodes.
	 */
	FSCSEditorTreeNodePtrType FindClosestParent(TArray<FSCSEditorTreeNodePtrType> InNodes);

	/** 
	 * Adds the given node as a child node.
	 *
	 * @param InChildNodePtr The node to add as a child node.
	 */
	void AddChild(FSCSEditorTreeNodePtrType InChildNodePtr);

	/** 
	 * Adds a child node for the given SCS node.
	 *
	 * @param InSCSNode The SCS node to for which to create a child node.
	 * @param bIsInherited Whether or not the given SCS node is inherited from a parent.
	 * @return A reference to the new child node or the existing child node if a match is found.
	 */
	FSCSEditorTreeNodePtrType AddChild(USCS_Node* InSCSNode, bool bIsInherited);

	/** 
	 * Adds a child node for the given component template.
	 *
	 * @param InComponentTemplate The component template for which to create a child node.
	 * @return A reference to the new child node or the existing child node if a match is found.
	 */
	FSCSEditorTreeNodePtrType AddChild(UActorComponent* InComponentTemplate);

	/** 
	 * Attempts to find a reference to the child node that matches the given SCS node.
	 *
	 * @param InSCSNode The SCS node to match.
	 * @param bRecursiveSearch Whether or not to recursively search child nodes (default == false).
	 * @param OutDepth If non-NULL, the depth of the child node will be returned in this parameter on success (default == NULL).
	 * @return The child node that matches the given SCS node, or an invalid node reference if no match was found.
	 */
	FSCSEditorTreeNodePtrType FindChild(const USCS_Node* InSCSNode, bool bRecursiveSearch = false, uint32* OutDepth = NULL) const;

	/** 
	 * Attempts to find a reference to the child node that matches the given component template.
	 *
	 * @param InComponentTemplate The component template instance to match.
	 * @param bRecursiveSearch Whether or not to recursively search child nodes (default == false).
	 * @param OutDepth If non-NULL, the depth of the child node will be returned in this parameter on success (default == NULL).
	 * @return The child node with a component template that matches the given component template instance, or an invalid node reference if no match was found.
	 */
	FSCSEditorTreeNodePtrType FindChild(const UActorComponent* InComponentTemplate, bool bRecursiveSearch = false, uint32* OutDepth = NULL) const;

	/** 
	 * Attempts to find a reference to the child node that matches the given component variable or instance name.
	 *
	 * @param InVariableOrInstanceName The component variable or instance name to match.
	 * @param bRecursiveSearch Whether or not to recursively search child nodes (default == false).
	 * @param OutDepth If non-NULL, the depth of the child node will be returned in this parameter on success (default == NULL).
	 * @return The child node with a component variable or instance name that matches the given name, or an invalid node reference if no match was found.
	 */
	FSCSEditorTreeNodePtrType FindChild(const FName& InVariableOrInstanceName, bool bRecursiveSearch = false, uint32* OutDepth = NULL) const;

	/** 
	 * Removes the given node from the list of child nodes.
	 *
	 * @param InChildNodePtr The child node to remove.
	 */
	void RemoveChild(FSCSEditorTreeNodePtrType InChildNodePtr);

	/** Delegate for when the context menu requests a rename */
	DECLARE_DELEGATE(FOnRenameRequested);

	/** Requests a rename on the component */
	void OnRequestRename(bool bTransactional);

	/** Renames the component */
	void OnCompleteRename(const FText& InNewName);

	/** Sets up the delegate for renaming a component */
	void SetRenameRequestedDelegate(FOnRenameRequested InRenameRequested) { RenameRequestedDelegate = InRenameRequested; }

	/** Get overridden template component, specialized in given blueprint */
	UActorComponent* GetOverridenComponentTemplate(UBlueprint* Blueprint, bool bCreateIfNecessary) const;

private:
	ENodeType NodeType;
	bool bIsInherited;
	bool bIsInstanced;
	TWeakObjectPtr<class USCS_Node> SCSNodePtr;
	TWeakObjectPtr<UActorComponent> ComponentTemplatePtr;
	FSCSEditorTreeNodePtrType ParentNodePtr;
	TArray<FSCSEditorTreeNodePtrType> Children;

	/** Handles rename requests */
	bool bNonTransactionalRename;
	FOnRenameRequested RenameRequestedDelegate;

	bool bWasInstancedFromNativeClass;
	FName InstancedComponentName;
	TWeakObjectPtr<AActor> InstancedComponentOwnerPtr;
};

//////////////////////////////////////////////////////////////////////////
// SSCS_RowWidget

class SSCS_RowWidget : public SMultiColumnTableRow<FSCSEditorTreeNodePtrType>
{
public:
	SLATE_BEGIN_ARGS( SSCS_RowWidget ){}
	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs, TSharedPtr<SSCSEditor> InSCSEditor, FSCSEditorTreeNodePtrType InNodePtr, TSharedPtr<STableViewBase> InOwnerTableView  );
	virtual ~SSCS_RowWidget();

	// SMultiColumnTableRow<T> interface
	virtual TSharedRef<SWidget> GenerateWidgetForColumn( const FName& ColumnName ) override;
	// End of SMultiColumnTableRow<T>

	// SWidget interface
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnDragDetected( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;
	virtual void OnDragEnter( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent ) override;
	virtual void OnDragLeave( const FDragDropEvent& DragDropEvent ) override;
	virtual FReply OnDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent ) override;
	// End of SWidget interface

	/** Get the blueprint we are editing */
	UBlueprint* GetBlueprint() const;

	FText GetNameLabel() const;
	FText GetTooltipText() const;
	FSlateColor GetColorTint() const;
	FString GetDocumentationLink() const;
	FString GetDocumentationExcerptName() const;
	
	FText GetAssetName() const;
	FText GetAssetPath() const;
	EVisibility GetAssetVisibility() const;

	EVisibility GetRootLabelVisibility() const;

	/* Get the node used by the row Widget */
	virtual FSCSEditorTreeNodePtrType GetNode() const { return TreeNodePtr; };

private:
	/** Verifies the name of the component when changing it */
	bool OnNameTextVerifyChanged(const FText& InNewText, FText& OutErrorMessage);

	/** Commits the new name of the component */
	void OnNameTextCommit(const FText& InNewName, ETextCommit::Type InTextCommit);

	/** Builds a context menu popup for dropping a child node onto the scene root node */
	TSharedPtr<SWidget> BuildSceneRootDropActionMenu(FSCSEditorTreeNodePtrType DroppedNodePtr);

	/** Creates a tooltip for this row */
	TSharedRef<SToolTip> CreateToolTipWidget() const;

	/** Handler for attaching a single node to this node */
	void OnAttachToDropAction(FSCSEditorTreeNodePtrType DroppedNodePtr)
	{
		TArray<FSCSEditorTreeNodePtrType> DroppedNodePtrs;
		DroppedNodePtrs.Add(DroppedNodePtr);
		OnAttachToDropAction(DroppedNodePtrs);
	}

	/** Handler for attaching one or more nodes to this node */
	void OnAttachToDropAction(const TArray<FSCSEditorTreeNodePtrType>& DroppedNodePtrs);

	/** Handler for detaching one or more nodes from the current parent and reattaching to the existing scene root node */
	void OnDetachFromDropAction(const TArray<FSCSEditorTreeNodePtrType>& DroppedNodePtrs);

	/** Handler for making the given node the new scene root node */
	void OnMakeNewRootDropAction(FSCSEditorTreeNodePtrType DroppedNodePtr);
	
	/** Tasks to perform after handling a drop action */
	void PostDragDropAction(bool bRegenerateTreeNodes);

	/**
	 * Retrieves an image brush signifying the specified component's mobility (could sometimes be NULL).
	 * 
	 * @param   SCSNode		A weak-pointer to the component node that you want a mobility icon for
	 * @returns A pointer to the FSlateBrush to use (NULL for Static and Non-SceneComponents)
	 */
	FSlateBrush const* GetMobilityIconImage(TWeakObjectPtr<USCS_Node> SCSNode) const;

	/**
	 * Retrieves tooltip text describing the specified component's mobility.
	 * 
	 * @param   SCSNode		A weak-pointer to the component node that you want tooltip text for
	 * @returns An FText object containing a description of the component's mobility
	 */
	FText GetMobilityToolTipText(TWeakObjectPtr<USCS_Node> SCSNode) const;
public:
	/** Pointer back to owning SCSEditor 2 tool */
	TWeakPtr<SSCSEditor> SCSEditor;

private:
	/** Pointer to node we represent */
	FSCSEditorTreeNodePtrType TreeNodePtr;
};

class SSCS_RowWidget_ActorRoot : public SSCS_RowWidget
{
public:

	// SMultiColumnTableRow<T> interface
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;
	// End of SMultiColumnTableRow<T>

	/* Get the node used by the row Widget */
	virtual FSCSEditorTreeNodePtrType GetNode() const override;

private:
	/** Creates a tooltip for this row */
	TSharedRef<SToolTip> CreateToolTipWidget() const;

	/** Data accessors */
	const FSlateBrush* GetActorIcon() const;
	FText GetActorDisplayText() const;
	FText GetActorClassNameText() const;
	FText GetActorSuperClassNameText() const;
	FText GetActorMobilityText() const;
};

class SSCS_RowWidget_Separator : public SSCS_RowWidget
{
public:

	// SMultiColumnTableRow<T> interface
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;
	// End of SMultiColumnTableRow<T>

private:

};

//////////////////////////////////////////////////////////////////////////
// SSCSEditorDragDropTree - implements STreeView for our specific node type and adds drag/drop functionality
class SSCSEditorDragDropTree : public STreeView<FSCSEditorTreeNodePtrType>
{
public:
	SLATE_BEGIN_ARGS( SSCSEditorDragDropTree )
		: _SCSEditor( NULL )
		, _OnGenerateRow()
		, _OnGetChildren()
		, _TreeItemsSource( static_cast< TArray<FSCSEditorTreeNodePtrType>* >(NULL) ) //@todo Slate Syntax: Initializing from NULL without a cast
		, _ItemHeight(16)
		, _OnContextMenuOpening()
		, _OnMouseButtonDoubleClick()
		, _OnSelectionChanged()
		, _OnExpansionChanged()
		, _SelectionMode(ESelectionMode::Multi)
		, _ClearSelectionOnClick(true)
		, _ExternalScrollbar()
		{}

		SLATE_ARGUMENT( SSCSEditor*, SCSEditor )

		SLATE_EVENT( FOnGenerateRow, OnGenerateRow )

		SLATE_EVENT( FOnItemScrolledIntoView, OnItemScrolledIntoView )

		SLATE_EVENT( FOnGetChildren, OnGetChildren )

		SLATE_ARGUMENT( TArray<FSCSEditorTreeNodePtrType>* , TreeItemsSource )

		SLATE_ATTRIBUTE( float, ItemHeight )

		SLATE_EVENT( FOnContextMenuOpening, OnContextMenuOpening )

		SLATE_EVENT( FOnMouseButtonDoubleClick, OnMouseButtonDoubleClick )

		SLATE_EVENT( FOnSelectionChanged, OnSelectionChanged )

		SLATE_EVENT( FOnExpansionChanged, OnExpansionChanged )

		SLATE_ATTRIBUTE( ESelectionMode::Type, SelectionMode )

		SLATE_ARGUMENT( TSharedPtr<SHeaderRow>, HeaderRow )

		SLATE_ARGUMENT ( bool, ClearSelectionOnClick )

		SLATE_ARGUMENT( TSharedPtr<SScrollBar>, ExternalScrollbar )

	SLATE_END_ARGS()
	/** Object construction - mostly defers to the base STreeView */
	void Construct( const FArguments& InArgs );

	/** Handler for drag over operations */
	FReply OnDragOver( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent );

	/** Handler for drop operations */
	FReply OnDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent );

private:
	/** Pointer to the SSCSEditor that owns this widget */
	SSCSEditor* SCSEditor;
};

//////////////////////////////////////////////////////////////////////////
// SSCSEditor

typedef SSCSEditorDragDropTree SSCSTreeType;

class KISMET_API SSCSEditor : public SCompoundWidget
{
public:
	/* Editor mode */
	enum EEditorMode
	{
		/* View/edit the SCS in a BGPC */
		BlueprintSCS,
		/* View/edit the Actor instance */
		ActorInstance
	};

	DECLARE_DELEGATE_RetVal_OneParam(class USCS_Node*, FOnAddNewComponent, class UClass*);
	DECLARE_DELEGATE_RetVal_OneParam(class USCS_Node*, FOnAddExistingComponent, class UActorComponent*);
	DECLARE_DELEGATE_OneParam(FOnSelectionUpdated, const TArray<FSCSEditorTreeNodePtrType>&);
	DECLARE_DELEGATE_OneParam(FOnItemDoubleClicked, const FSCSEditorTreeNodePtrType);
	DECLARE_DELEGATE_OneParam(FOnHighlightPropertyInDetailsView, const class FPropertyPath&);

	SLATE_BEGIN_ARGS( SSCSEditor )
		:_EditorMode(BlueprintSCS)
		,_ActorContext(nullptr)
		,_PreviewActor(nullptr)
		,_AllowEditing(true)
		,_HideComponentClassCombo(false)
		,_OnSelectionUpdated()
		,_OnHighlightPropertyInDetailsView()
		{}

		SLATE_ARGUMENT(EEditorMode, EditorMode)
		SLATE_ATTRIBUTE(class AActor*, ActorContext)
		SLATE_ATTRIBUTE(class AActor*, PreviewActor)
		SLATE_ATTRIBUTE(bool, AllowEditing)
		SLATE_ATTRIBUTE(bool, HideComponentClassCombo)
		SLATE_EVENT(FOnSelectionUpdated, OnSelectionUpdated)
		SLATE_EVENT(FOnItemDoubleClicked, OnItemDoubleClicked)
		SLATE_EVENT(FOnHighlightPropertyInDetailsView, OnHighlightPropertyInDetailsView)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** SWidget interface */
	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime );
	virtual FReply OnKeyDown( const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent );

	/** Used by tree control - make a widget for a table row from a node */
	TSharedRef<ITableRow> MakeTableRowWidget(FSCSEditorTreeNodePtrType InNodePtr, const TSharedRef<STableViewBase>& OwnerTable);

	/** Used by tree control - get children for a specified node */
	void OnGetChildrenForTree(FSCSEditorTreeNodePtrType InNodePtr, TArray<FSCSEditorTreeNodePtrType>& OutChildren);

	/** Returns true if editing is allowed */
	bool IsEditingAllowed() const;

	/** Adds a component to the SCS Table
	   @param NewComponentClass	(In) The class to add
	   @param Asset				(In) Optional asset to assign to the component
	   @return The reference of the newly created ActorComponent */
	UActorComponent* AddNewComponent(UClass* NewComponentClass, UObject* Asset);

	/** Adds a new SCS Node to the component Table
	   @param NewNode	(In) The SCS node to add
	   @param Asset		(In) Optional asset to assign to the component
	   @param bMarkBlueprintModified (In) Whether or not to mark the Blueprint as structurally modified
	   @param bSetFocusToNewItem (In) Select the new item and activate the inline rename widget (default is true)
	   @return The reference of the newly created ActorComponent */
	UActorComponent* AddNewNode(USCS_Node* NewNode, UObject* Asset, bool bMarkBlueprintModified, bool bSetFocusToNewItem = true);

	/** Adds a new component instance node to the component Table
		@param NewInstanceComponent	(In) The component being added to the actor instance
		@param Asset	(In) Optional asset to assign to the component
		@param bSetFocusToNewItem (In) Select the new item and activate the inline rename widget (default is true)
		@return The reference of the newly created ActorComponent */
	UActorComponent* AddNewNode(UActorComponent* NewInstanceComponent, UObject* Asset, bool bSetFocusToNewItem = true);
	
	/** Returns true if the specified component is currently selected */
	bool IsComponentSelected(const UPrimitiveComponent* PrimComponent) const;

	/** Assigns a selection override delegate to the specified component */
	void SetSelectionOverride(UPrimitiveComponent* PrimComponent) const;

	/** Cut selected node(s) */
	void CutSelectedNodes();
	bool CanCutNodes() const;

	/** Copy selected node(s) */
	void CopySelectedNodes();
	bool CanCopyNodes() const;

	/** Pastes previously copied node(s) */
	void PasteNodes();
	bool CanPasteNodes() const;

	/** Removes existing selected component nodes from the SCS */
	void OnDeleteNodes();
	bool CanDeleteNodes() const;

	/** Removes an existing component node from the tree */
	void RemoveComponentNode(FSCSEditorTreeNodePtrType InNodePtr);

	/** Called when selection in the tree changes */
	void OnTreeSelectionChanged(FSCSEditorTreeNodePtrType InSelectedNodePtr, ESelectInfo::Type SelectInfo);

	/** Called when the Actor is selected. */
	void OnActorSelected(const ECheckBoxState NewCheckedState);

	/** Called to determine if actor is selected. */
	ECheckBoxState OnIsActorSelected() const;

	/** Update any associated selection (e.g. details view) from the passed in nodes */
	void UpdateSelectionFromNodes(const TArray<FSCSEditorTreeNodePtrType> &SelectedNodes );

	/** Refresh the tree control to reflect changes in the SCS */
	void UpdateTree(bool bRegenerateTreeNodes = true);

	/** Forces the details panel to refresh on the same objects */
	void RefreshSelectionDetails();

	/** Clears the current selection */
	void ClearSelection();

	/** Get the currently selected tree nodes */
	TArray<FSCSEditorTreeNodePtrType> GetSelectedNodes() const;

	/**
	 * Fills out an events section in ui.
	 * @param Menu								the menu to add the events section into
	 * @param Blueprint							the active blueprint context being edited
	 * @param SelectedClass						the common component class to build the events list from
	 * @param CanExecuteActionDelegate			the delegate to query whether or not to execute the UI action
	 * @param GetSelectedObjectsDelegate		the delegate to fill the currently select variables / components
	 */
	static void BuildMenuEventsSection( FMenuBuilder& Menu, UBlueprint* Blueprint, UClass* SelectedClass, FCanExecuteAction CanExecuteActionDelegate, FGetSelectedObjectsDelegate GetSelectedObjectsDelegate );

	/**
	 * Given an actor component, attempts to find an associated tree node.
	 *
	 * @param ActorComponent The component associated with the node.
	 * @param bIncludeAttachedComponents Whether or not to include components attached to each node in the search (default is true).
	 * @return A shared pointer to a tree node. The pointer will be invalid if no match could be found.
	 */
	FSCSEditorTreeNodePtrType GetNodeFromActorComponent(const UActorComponent* ActorComponent, bool bIncludeAttachedComponents = true) const;

	/** Select the root of the tree */
	void SelectRoot();

	/** Select the given tree node */
	void SelectNode(FSCSEditorTreeNodePtrType InNodeToSelect, bool IsCntrlDown);

	/**
	 * Highlight a tree node and, optionally, a property with in it
	 *
	 * @param TreeNodeName		Name of the treenode to be highlighted
	 * @param Property	The name of the property to be highlighted in the details view
	 * @return True if the node was found in this Editor, otherwise false
	 */
	void HighlightTreeNode( FName TreeNodeName, const class FPropertyPath& Property );
	/**
	 * Highlight a tree node and, optionally, a property with in it
	 *
	 * @param Node		 A Reference to the Node SCS_Node to be highlighted
	 * @param Property	The name of the property to be highlighted in the details view
	 */
	void HighlightTreeNode( const USCS_Node* Node, FName Property );

	/**
	 * Function to save current state of SimpleConstructionScript and nodes associated with it.
	 *
	 * @param: SimpleContructionScript object reference.
	 */
	static void SaveSCSCurrentState( USimpleConstructionScript* SCSObj );

	/**
	 * Function to save the current state of SCS_Node and its children
	 *
	 * @param: Reference of the SCS_Node to be saved
	 */
	static void SaveSCSNode(USCS_Node* Node);

	/** Is this node still used by the Simple Construction Script */
	bool IsNodeInSimpleConstructionScript(USCS_Node* Node) const;

	/**
	 * Fills the supplied array with the currently selected objects
	 * @param OutSelectedItems The array to fill.
	 */
	void GetSelectedItemsForContextMenu(TArray<FComponentEventConstructionData>& OutSelectedItems) const;

	/** Provides access to the Blueprint context that's being edited */
	class UBlueprint* GetBlueprint() const;

	/** Returns the set of root nodes */
	const TArray<FSCSEditorTreeNodePtrType>& GetRootComponentNodes();

	/** @return The current editor mode (editing live actors or editing blueprints) */
	EEditorMode GetEditorMode() const { return EditorMode; }
protected:
	FString GetSelectedClassText() const;

	/** Add a component from the selection in the combo box */
	void PerformComboAddClass(TSubclassOf<UActorComponent> ComponentClass, EComponentCreateAction::Type ComponentCreateAction );

	/** Called to display context menu when right clicking on the widget */
	TSharedPtr< SWidget > CreateContextMenu();

	/**
	 * Requests a rename on the selected component
	 * @param bTransactional Whether or not the rename should be transactional (i.e. undoable)
	 */
	void OnRenameComponent(bool bTransactional);

	/** Checks to see if renaming is allowed on the selected component */
	bool CanRenameComponent() const;

	/**
	 * Function to create events for the current selection
	 * @param Blueprint						the active blueprint context
	 * @param EventName						the event to add
	 * @param GetSelectedObjectsDelegate	the delegate to gather information about current selection
	 * @param NodeIndex						an index to a specified node to add event for or < 0 for all selected nodes.
	 */
	static void CreateEventsForSelection(UBlueprint* Blueprint, FName EventName, FGetSelectedObjectsDelegate GetSelectedObjectsDelegate);

	/**
	 * Function to construct an event for a node
	 * @param Blueprint						the nodes blueprint
	 * @param EventName						the event to add
	 * @param EventData						the event data structure describing the node
	 */
	static void ConstructEvent(UBlueprint* Blueprint, const FName EventName, const FComponentEventConstructionData EventData);

	/**
	 * Function to view an event for a node
	 * @param Blueprint						the nodes blueprint
	 * @param EventName						the event to view
	 * @param EventData						the event data structure describing the node
	 */
	static void ViewEvent(UBlueprint* Blueprint, const FName EventName, const FComponentEventConstructionData EventData);

	/** Callbacks to duplicate the selected component */
	bool CanDuplicateComponent() const;
	void OnDuplicateComponent();

	/** Helper method to add a tree node for the given SCS node */
	FSCSEditorTreeNodePtrType AddTreeNode(USCS_Node* InSCSNode, FSCSEditorTreeNodePtrType InParentNodePtr, const bool bIsInherited);

	/** Helper method to add a tree node for the given scene component */
	FSCSEditorTreeNodePtrType AddTreeNode(USceneComponent* InSceneComponent);

	/** Helper method to recursively find a tree node for the given SCS node starting at the given tree node */
	FSCSEditorTreeNodePtrType FindTreeNode(const USCS_Node* InSCSNode, FSCSEditorTreeNodePtrType InStartNodePtr = FSCSEditorTreeNodePtrType()) const;

	/** Helper method to recursively find a tree node for the given scene component starting at the given tree node */
	FSCSEditorTreeNodePtrType FindTreeNode(const UActorComponent* InComponent, FSCSEditorTreeNodePtrType InStartNodePtr = FSCSEditorTreeNodePtrType()) const;

	/** Helper method to recursively find a tree node for the given variable or instance name starting at the given tree node */
	FSCSEditorTreeNodePtrType FindTreeNode(const FName& InVariableOrInstanceName, FSCSEditorTreeNodePtrType InStartNodePtr = FSCSEditorTreeNodePtrType()) const;

	/** Callback when a component item is scrolled into view */
	void OnItemScrolledIntoView( FSCSEditorTreeNodePtrType InItem, const TSharedPtr<ITableRow>& InWidget);

	/** Callback when a component item is double clicked. */
	void HandleItemDoubleClicked(FSCSEditorTreeNodePtrType InItem);

	/** Returns the set of expandable nodes that are currently collapsed in the UI */
	void GetCollapsedNodes(const FSCSEditorTreeNodePtrType& InNodePtr, TSet<FSCSEditorTreeNodePtrType>& OutCollapsedNodes) const;

	/** @return The visibility of the promote to blueprint button (only visible with an actor instance that is not created from a blueprint)*/
	EVisibility GetPromoteToBlueprintButtonVisibility() const;

	/** @return The visibility of the Edit Blueprint button (only visible with an actor instance that is created from a blueprint)*/
	EVisibility GetEditBlueprintButtonVisibility() const;

	/** @return the tooltip describing how many properties will be applied to the blueprint */
	FText OnGetApplyChangesToBlueprintTooltip() const;

	/** @return the tooltip describing how many properties will be reset to the blueprint default*/
	FText OnGetResetToBlueprintDefaultsTooltip() const;

	/** Opens the blueprint editor for the blueprint being viewed by the scseditor */
	void OnOpenBlueprintEditor() const;

	/** Propagates instance changes to the blueprint */
	void OnApplyChangesToBlueprint() const;

	/** Resets instance changes to the blueprint default */
	void OnResetToBlueprintDefaults() const;

	/** Converts the current actor instance to a blueprint */
	void PromoteToBlueprint() const;

	/** Called when the promote to blueprint button is clicked */
	FReply OnPromoteToBlueprintClicked();

	/** gets a root nodes of the tree */
	const TArray<FSCSEditorTreeNodePtrType>& GetRootNodes() const;

	/** Adds a root component tree node */
	TSharedPtr<FSCSEditorTreeNode> AddRootComponentTreeNode(UActorComponent* ActorComp);

	/**
	 * Creates a new C++ component from the specified class type
	 * The user will be prompted to pick a new subclass name and code will be recompiled
	 *
	 * @return The new class that was created
	 */
	UClass* CreateNewCPPComponent(TSubclassOf<UActorComponent> ComponentClass);

	
	/**
	 * Creates a new Blueprint component from the specified class type
	 * The user will be prompted to pick a new subclass name and a blueprint asset will be created
	 *
	 * @return The new class that was created
	 */
	UClass* CreateNewBPComponent(TSubclassOf<UActorComponent> ComponentClass);

public:
	/** Tree widget */
	TSharedPtr<SSCSTreeType> SCSTreeWidget;

	/** The node that represents the root component in the scene hierarchy */
	FSCSEditorTreeNodePtrType SceneRootNodePtr;

	/** Command list for handling actions in the SSCSEditor */
	TSharedPtr< FUICommandList > CommandList;

	/** Name of a node that has been requested to be renamed */
	FName DeferredRenameRequest;

	/** Whether or not the deferred rename request was flagged as transactional */
	bool bIsDeferredRenameRequestTransactional;

	/** Attribute that provides access to the Actor context for which we are viewing/editing the SCS. */
	TAttribute<class AActor*> ActorContext;

	/** Attribute that provides access to a "preview" Actor context (may not be same as the Actor context that's being edited. */
	TAttribute<class AActor*> PreviewActor;

	/** Attribute to indicate whether or not editing is allowed. */
	TAttribute<bool> AllowEditing;

	/** Delegate to invoke on selection update. */
	FOnSelectionUpdated OnSelectionUpdated;

	/** Delegate to invoke when an item in the tree is double clicked. */
	FOnItemDoubleClicked OnItemDoubleClicked;

	/** Delegate to invoke when the given property should be highlighted in the details view (e.g. diff). */
	FOnHighlightPropertyInDetailsView OnHighlightPropertyInDetailsView;
private:
	/** Indicates which editor mode we're in. */
	EEditorMode EditorMode;

	/** Root set of tree */
	TArray<FSCSEditorTreeNodePtrType> RootNodes;

	/** Root set of components*/
	TArray<FSCSEditorTreeNodePtrType> RootComponentNodes;

	/* Root Tree Node*/
	TSharedPtr<FSCSEditorTreeNode> RootTreeNode;

	/** Flag to enable/disable component editing */
	bool bEnableComponentEditing;

	/** Gate to prevent changing the selection while selection change is being broadcast. */
	bool bUpdatingSelection;

	/** Flag to track if we have the Actor selected curently */
	bool bIsActorSelected;
};
