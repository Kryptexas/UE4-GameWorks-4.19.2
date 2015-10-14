// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class IKeyArea;
class ISequencerSection;
class SSequencerTreeViewRow;
class FGroupedKeyArea;

/** Structure used to define padding for a particular node */
struct FNodePadding
{
	FNodePadding(float InUniform) : Top(InUniform), Bottom(InUniform) {}
	FNodePadding(float InTop, float InBottom) : Top(InTop), Bottom(InBottom) {}
	
	/** @return The sum total of the separate padding values */
	float Combined() const { return Top + Bottom; }

	/** Padding to be applied to the top of the node */
	float Top;
	/** Padding to be applied to the bottom of the node */
	float Bottom;
};

namespace ESequencerNode
{
	enum Type
	{
		/* Top level object binding node */
		Object,
		/* Area for tracks */
		Track,
		/* Area for keys inside of a section */
		KeyArea,
		/* Displays a category */
		Category,
	};
}

/**
 * Base Sequencer layout node
 */
class FSequencerDisplayNode : public TSharedFromThis<FSequencerDisplayNode>
{
public:
	virtual ~FSequencerDisplayNode(){}
	/**
	 * Constructor
	 * 
	 * @param InNodeName	The name identifier of then node
	 * @param InParentNode	The parent of this node or NULL if this is a root node
	 * @param InParentTree	The tree this node is in
	 */
	FSequencerDisplayNode( FName InNodeName, TSharedPtr<FSequencerDisplayNode> InParentNode, FSequencerNodeTree& InParentTree );

	/**
	* Adds an object binding node to this node.
	*
	* @param ObjectBindingNode The node to add
	*/
	void AddObjectBindingNode( TSharedRef<FObjectBindingNode> ObjectBindingNode );

	/**
	 * Adds a category to this node
	 * 
	 * @param CategoryName	Name of the category
	 * @param DisplayName	Localized label to display in the animation outliner
	 */
	TSharedRef<class FSectionCategoryNode> AddCategoryNode( FName CategoryName, const FText& DisplayLabel );

	/**
	 * Adds a new section area for this node.
	 * 
	 * @param SectionName		Name of the section area
	 * @param AssociatedTrack	The track associated with sections in this node
	 * @param AssociatedEditor	The track editor for the associated track
	 */
	TSharedRef<class FTrackNode> AddSectionAreaNode( FName SectionName, UMovieSceneTrack& AssociatedTrack, ISequencerTrackEditor& AssociatedEditor );

	/**
	 * Adds a key area to this node
	 *
	 * @param KeyAreaName	Name of the key area
	 * @param DisplayLabel	Localized label to display in the animation outliner
	 * @param KeyArea		Key area interface for drawing and interaction with keys
	 */
	void AddKeyAreaNode( FName KeyAreaName, const FText& DisplayLabel, TSharedRef<IKeyArea> KeyArea );

	/**
	 * @return The type of node this is
	 */
	virtual ESequencerNode::Type GetType() const = 0;

	/** @return Whether or not this node can be selected */
	virtual bool IsSelectable() const { return true; }

	/**
	 * @return The desired height of the node when displayed
	 */
	virtual float GetNodeHeight() const = 0;
	
	/**
	 * @return The desired padding of the node when displayed
	 */
	virtual FNodePadding GetNodePadding() const = 0;

	/**
	 * @return The localized display name of this node
	 */
	virtual FText GetDisplayName() const = 0;

	/**
	 * Generates a container widget for tree display in the animation outliner portion of the track area
	 * 
	 * @return Generated outliner container widget
	 */
	virtual TSharedRef<SWidget> GenerateContainerWidgetForOutliner(const TSharedRef<SSequencerTreeViewRow>& InRow);

	/**
	* Generates a widget editing a row in the animation outliner portion of the track area
	*
	* @return Generated outliner edit widget
	*/
	virtual TSharedRef<SWidget> GenerateEditWidgetForOutliner();

	/**
	 * Generates a widget for display in the section area portion of the track area
	 * 
	 * @param ViewRange	The range of time in the sequencer that we are displaying
	 * @return Generated outliner widget
	 */
	virtual TSharedRef<SWidget> GenerateWidgetForSectionArea( const TAttribute< TRange<float> >& ViewRange );

	/**
	 * Get the display node that is ultimately responsible for constructing a section area widget for this node.
	 * Could return this node itself, or a parent node
	 */
	TSharedPtr<FSequencerDisplayNode> GetSectionAreaAuthority() const;

	/**
	 * @return the path to this node starting with the outermost parent
	 */
	FString GetPathName() const;

	/** Summon context menu */
	TSharedPtr<SWidget> OnSummonContextMenu(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);

	/** What sort of context menu this node summons */
	virtual void BuildContextMenu(FMenuBuilder& MenuBuilder);

	/**
	 * @return The name of the node (for identification purposes)
	 */
	FName GetNodeName() const { return NodeName; }

	/**
	 * @return The number of child nodes belonging to this node
	 */
	uint32 GetNumChildren() const { return ChildNodes.Num(); }

	/**
	 * @return A List of all Child nodes belonging to this node
	 */
	const TArray< TSharedRef<FSequencerDisplayNode> >& GetChildNodes() const { return ChildNodes; }

	/**
	 * Iterate this entire node tree, child first.
	 * @param 	InPredicate			Predicate to call for each node, returning whether to continue iteration or not
	 * @param 	bIncludeThisNode	Whether to include this node in the iteration, or just children
	 * @return  true where the client prematurely exited the iteration, false otherwise
	 */
	bool Traverse_ChildFirst(const TFunctionRef<bool(FSequencerDisplayNode&)>& InPredicate, bool bIncludeThisNode = true);

	/**
	 * Iterate this entire node tree, parent first.
	 * @param 	InPredicate			Predicate to call for each node, returning whether to continue iteration or not
	 * @param 	bIncludeThisNode	Whether to include this node in the iteration, or just children
	 * @return  true where the client prematurely exited the iteration, false otherwise
	 */
	bool Traverse_ParentFirst(const TFunctionRef<bool(FSequencerDisplayNode&)>& InPredicate, bool bIncludeThisNode = true);

	/**
	 * Iterate any visible portions of this node's sub-tree, child first.
	 * @param 	InPredicate			Predicate to call for each node, returning whether to continue iteration or not
	 * @param 	bIncludeThisNode	Whether to include this node in the iteration, or just children
	 * @return  true where the client prematurely exited the iteration, false otherwise
	 */
	bool TraverseVisible_ChildFirst(const TFunctionRef<bool(FSequencerDisplayNode&)>& InPredicate, bool bIncludeThisNode = true);

	/**
	 * Iterate any visible portions of this node's sub-tree, parent first.
	 * @param 	InPredicate			Predicate to call for each node, returning whether to continue iteration or not
	 * @param 	bIncludeThisNode	Whether to include this node in the iteration, or just children
	 * @return  true where the client prematurely exited the iteration, false otherwise
	 */
	bool TraverseVisible_ParentFirst(const TFunctionRef<bool(FSequencerDisplayNode&)>& InPredicate, bool bIncludeThisNode = true);

	/**
	 * Sorts the child nodes with the supplied predicate
	 */
	template <class PREDICATE_CLASS>
	void SortChildNodes(const PREDICATE_CLASS& Predicate)
	{
		ChildNodes.StableSort(Predicate);
		for (TSharedRef<FSequencerDisplayNode> ChildNode : ChildNodes)
		{
			ChildNode->SortChildNodes(Predicate);
		}
	}

	/**
	 * @return The parent of this node                                                              
	 */
	TSharedPtr<FSequencerDisplayNode> GetParent() const { return ParentNode.Pin(); }
	
	/** Gets the sequencer that owns this node */
	FSequencer& GetSequencer() const { return ParentTree.GetSequencer(); }
	
	/** Gets the parent tree that this node is in */
	FSequencerNodeTree& GetParentTree() const { return ParentTree; }

	/** Gets all the key area nodes recursively, including this node if applicable */
	virtual void GetChildKeyAreaNodesRecursively(TArray< TSharedRef<class FSectionKeyAreaNode> >& OutNodes) const;

	/**
	 * Set whether this node is expanded or not
	 */
	void SetExpansionState(bool bInExpanded);

	/**
	 * @return Whether or not this node is expanded
	 */
	bool IsExpanded() const;

	/**
	 * @return Whether this node is explicitly hidden from the view or not
	 */
	bool IsHidden() const;

	/** Initialize this node with expansion states and virtual offsets */
	void Initialize(float InVirtualTop, float InVirtualBottom);

	/** @return this node's virtual offset from the top of the tree, irrespective of expansion states */
	float GetVirtualTop() const { return VirtualTop; }
	
	/** @return this node's virtual offset plus its virtual height, irrespective of expansion states */
	float GetVirtualBottom() const { return VirtualBottom; }

	/** Get the key grouping for the specified section index, ensuring it is fully up to date */
	TSharedRef<FGroupedKeyArea> UpdateKeyGrouping(int32 InSectionIndex);

	/** Get the key grouping for the specified section index */
	TSharedRef<FGroupedKeyArea> GetKeyGrouping(int32 InSectionIndex);

protected:
	/** The virtual offset of this item from the top of the tree, irrespective of expansion states */
	float VirtualTop;
	/** The virtual offset + virtual heightof this item, irrespective of expansion states */
	float VirtualBottom;

protected:
	/** The parent of this node*/
	TWeakPtr< FSequencerDisplayNode > ParentNode;
	/** List of children belonging to this node */
	TArray< TSharedRef<FSequencerDisplayNode > > ChildNodes;
	/** Parent tree that this node is in */
	FSequencerNodeTree& ParentTree;
	/** The name identifier of this node */
	FName NodeName;
	/** Whether or not the node is expanded */
	bool bExpanded;
	/** Transient grouped keys for this node */
	TArray< TSharedPtr<FGroupedKeyArea> > KeyGroupings;
};

/**
 * Represents an area inside a section where keys are displayed
 * There is one key area per section that defines that key area
 */
class FSectionKeyAreaNode : public FSequencerDisplayNode
{
public:
	/**
	 * Constructor
	 * 
	 * @param InNodeName	The name identifier of then node
	 * @param InDisplayName	Display name of the category
	 * @param InParentNode	The parent of this node or NULL if this is a root node
	 * @param InParentTree	The tree this node is in
	 * @param bInTopLevel	If true the node is part of the section itself instead of taking up extra height in the section
	 */
	FSectionKeyAreaNode( FName NodeName, const FText& InDisplayName, TSharedPtr<FSequencerDisplayNode> InParentNode, FSequencerNodeTree& InParentTree, bool bInTopLevel = false )
		: FSequencerDisplayNode( NodeName, InParentNode, InParentTree )
		, DisplayName( InDisplayName )
		, bTopLevel( bInTopLevel )
	{}

	/** FSequencerDisplayNodeInterface */
	virtual ESequencerNode::Type GetType() const override { return ESequencerNode::KeyArea; }
	virtual float GetNodeHeight() const override;
	virtual FNodePadding GetNodePadding() const override;
	virtual FText GetDisplayName() const override { return DisplayName; }
	virtual TSharedRef<SWidget> GenerateEditWidgetForOutliner() override;

	/**
	 * Adds a key area to this node
	 *
	 * @param KeyArea	The key area interface to add
	 */
	void AddKeyArea( TSharedRef<IKeyArea> KeyArea );

	/**
	 * Returns a key area at the given index
	 * 
	 * @param Index	The index of the key area to get
	 * @return the key area at the index
	 */
	TSharedRef<IKeyArea> GetKeyArea( uint32 Index ) const { return KeyAreas[Index]; }

	/**
	 * Returns all key area for this node
	 * 
	 * @return All key areas
	 */
	TArray< TSharedRef<IKeyArea> > GetAllKeyAreas() const { return KeyAreas; }

	/** @return Whether the node is top level.  (I.E., is part of the section itself instead of taking up extra height in the section) */
	bool IsTopLevel() const { return bTopLevel; }
private:
	/** The display name of the key area */
	FText DisplayName;
	/** All key areas on this node (one per section) */
	TArray< TSharedRef<IKeyArea> > KeyAreas;
	/** If true the node is part of the section itself instead of taking up extra height in the section */
	bool bTopLevel;
};


/**
 * Represents an area to display Sequencer sections (possibly on multiple lines)
 */
class FTrackNode : public FSequencerDisplayNode
{
public:
	/**
	 * Constructor
	 * 
	 * @param InNodeName The name identifier of then node
	 * @param InAssociatedType The track that this node represents.
	 * @param InAssociatedEditor The track editor for the track that this node represents.
	 * @param InParentNode The parent of this node or NULL if this is a root node
	 * @param InParentTree The tree this node is in
	 */
	FTrackNode( FName NodeName, UMovieSceneTrack& InAssociatedType, ISequencerTrackEditor& InAssociatedEditor, TSharedPtr<FSequencerDisplayNode> InParentNode, FSequencerNodeTree& InParentTree );

	/** FSequencerDisplayNodeInterface */
	virtual ESequencerNode::Type GetType() const override { return ESequencerNode::Track; }
	virtual float GetNodeHeight() const override;
	virtual FNodePadding GetNodePadding() const override;
	virtual FText GetDisplayName() const override;
	virtual void GetChildKeyAreaNodesRecursively(TArray< TSharedRef<class FSectionKeyAreaNode> >& OutNodes) const override;
	virtual TSharedRef<SWidget> GenerateEditWidgetForOutliner() override;

	/**
	 * Adds a section to this node
	 *
	 * @param SequencerSection	The section to add
	 */
	void AddSection( TSharedRef<ISequencerSection>& SequencerSection )
	{
		Sections.Add( SequencerSection );
	}

	/**
	 * Makes the section itself a key area without taking up extra space
	 *
	 * @param KeyArea	Interface for the key area
	 */
	void SetSectionAsKeyArea( TSharedRef<IKeyArea>& KeyArea );
	
	/**
	 * @return All sections in this node
	 */
	const TArray< TSharedRef<ISequencerSection> >& GetSections() const { return Sections; }
	TArray< TSharedRef<ISequencerSection> >& GetSections() { return Sections; }

	/** @return Returns the top level key node for the section area if it exists */
	TSharedPtr< FSectionKeyAreaNode > GetTopLevelKeyNode() { return TopLevelKeyNode; }

	/** @return the track associated with this section */
	UMovieSceneTrack* GetTrack() const { return AssociatedType.Get(); }

	/** Gets the greatest row index of all the sections we have */
	int32 GetMaxRowIndex() const;

	/** Ensures all row indices which have no sections are gone */
	void FixRowIndices();

private:
	/** All of the sequencer sections in this node */
	TArray< TSharedRef<ISequencerSection> > Sections;
	/** If the section area is a key area itself, this represents the node for the keys */
	TSharedPtr< FSectionKeyAreaNode > TopLevelKeyNode;
	/** The type associated with the sections in this node */
	TWeakObjectPtr<UMovieSceneTrack> AssociatedType;
	/** The track editor for the track associated with this node. */
	ISequencerTrackEditor& AssociatedEditor;
};

/**
 * A node for displaying an object binding
 */
class FObjectBindingNode : public FSequencerDisplayNode
{
public:
	/**
	 * Constructor
	 * 
	 * @param InNodeName		The name identifier of then node
	 * @param InObjectName		The name of the object we're binding to
	 * @param InObjectBinding	Object binding guid for associating with live objects
	 * @param InParentNode		The parent of this node or NULL if this is a root node
	 * @param InParentTree		The tree this node is in
	 */
	FObjectBindingNode( FName NodeName, const FString& InObjectName, const FGuid& InObjectBinding, TSharedPtr<FSequencerDisplayNode> InParentNode, FSequencerNodeTree& InParentTree )
		: FSequencerDisplayNode( NodeName, InParentNode, InParentTree )
		, ObjectBinding( InObjectBinding )
		, DefaultDisplayName( FText::FromString(InObjectName) )
	{}

	/** FSequencerDisplayNodeInterface */
	virtual ESequencerNode::Type GetType() const override { return ESequencerNode::Object; }
	virtual FText GetDisplayName() const override;
	virtual float GetNodeHeight() const override;
	virtual FNodePadding GetNodePadding() const override;
	TSharedRef<SWidget> GenerateEditWidgetForOutliner() override;
	
	/** @return The object binding on this node */
	const FGuid& GetObjectBinding() const { return ObjectBinding; }
	
	/** What sort of context menu this node summons */
	virtual void BuildContextMenu(FMenuBuilder& MenuBuilder) override;

private:
	TSharedRef<SWidget> OnGetAddTrackMenuContent();

	void AddPropertyMenuItemsWithCategories(FMenuBuilder& AddTrackMenuBuilder, TArray<TArray<UProperty*> > KeyablePropertyPath);
	void AddPropertyMenuItems(FMenuBuilder& AddTrackMenuBuilder, TArray<TArray<UProperty*> > KeyableProperties, int32 PropertyNameIndexStart = 0, int32 PropertyNameIndexEnd = -1);

	void AddTrackForProperty(TArray<UProperty*> PropertyPath);

	/** Get class for object binding */
	const UClass* GetClassForObjectBinding();

	/** The binding to live objects */
	FGuid ObjectBinding;
	/** The default display name of the object which is used if the binding manager doesn't provide one. */
	FText DefaultDisplayName;
};

/**
 * A node that displays a category for other nodes
 */
class FSectionCategoryNode : public FSequencerDisplayNode
{
public:
	/**
	 * Constructor
	 * 
	 * @param InNodeName	The name identifier of then node
	 * @param InDisplayName	Display name of the category
	 * @param InParentNode	The parent of this node or NULL if this is a root node
	 * @param InParentTree	The tree this node is in
	 */
	FSectionCategoryNode( FName NodeName, const FText& InDisplayName, TSharedPtr<FSequencerDisplayNode> InParentNode, FSequencerNodeTree& InParentTree )
		: FSequencerDisplayNode( NodeName, InParentNode, InParentTree )
		, DisplayName( InDisplayName )
	{}

	/** FSequencerDisplayNodeInterface */
	virtual ESequencerNode::Type GetType() const override { return ESequencerNode::Category; }
	virtual float GetNodeHeight() const override;
	virtual FNodePadding GetNodePadding() const override;
	virtual FText GetDisplayName() const override { return DisplayName; }
private:
	/** The display name of the category */
	FText DisplayName;
};
