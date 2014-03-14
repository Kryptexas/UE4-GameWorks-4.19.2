// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


/** Dynamic N-way splitter that provides the resizing functionality in the docking framework. */
class SLATE_API SDockingSplitter : public SDockingNode
{
public:
	SLATE_BEGIN_ARGS(SDockingSplitter){}
	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs, const TSharedRef<FTabManager::FSplitter>& PersistentNode );

	virtual Type GetNodeType() const
	{
		return SDockingNode::DockSplitter;
	}

	/**
	 * Add a new child dock node at the desired location.
	 * Assumes this DockNode is a splitter.
	 *
	 * @param InChild      The DockNode child to add.
	 * @param InLocation   Index at which to add; INDEX_NONE (default) adds to the end of the list
	 */
	void AddChildNode( const TSharedRef<SDockingNode>& InChild, int32 InLocation = INDEX_NONE);

	/**
	 * Replace the child InChildToReplace with Replacement
	 *
	 * @param InChildToReplace     The child of this node to replace
	 * @param Replacement          What to replace with
	 */
	void ReplaceChild( const TSharedRef<SDockingNode>& InChildToReplace, const TSharedRef<SDockingNode>& Replacement );

	/**
	 * Remove a child node from the splitter
	 * 
	 * @param ChildToRemove    The child node to remove from this splitter
	 */
	void RemoveChild( const TSharedRef<SDockingNode>& ChildToRemove );
	
	/**
	 * Remove a child at this index
	 *
	 * @param IndexOfChildToRemove    Remove a child at this index
	 */
	void RemoveChildAt( int32 IndexOfChildToRemove );

	/**
	 * Inserts NodeToPlace next to RelativeToMe.
	 * Next to means above, below, left or right of RelativeToMe.
	 *
	 * @param NodeToPlace        The node to insert.
	 * @param Direction          Where to insert relative to the other node
	 * @param RelativeToMove     Insert relative to this node.
	 */
	void PlaceNode( const TSharedRef<SDockingNode>& NodeToPlace, SDockingNode::RelativeDirection Direction, const TSharedRef<SDockingNode>& RelativeToMe );

	/** Sets the orientation of this splitter */
	void SetOrientation(EOrientation NewOrientation);

	/** Gets an array of all child dock nodes */
	const TArray< TSharedRef<SDockingNode> >& GetChildNodes() const;

	/** Gets an array of all child dock nodes and all of their child dock nodes and so on */
	TArray< TSharedRef<SDockingNode> > GetChildNodesRecursively() const;
	
	/** Recursively searches through all children looking for child tabs */
	virtual TArray< TSharedRef<SDockTab> > GetAllChildTabs() const OVERRIDE;

	/** Gets the size coefficient of a given child dock node */
	float GetSizeCoefficientForSlot(int32 Index) const;

	/** Gets the orientation of this splitter */
	EOrientation GetOrientation() const;

	virtual TSharedPtr<FTabManager::FLayoutNode> GatherPersistentLayout() const OVERRIDE;

protected:

	/** Is the docking direction (left, right, above, below) match the orientation (horizontal vs. vertical) */
	static bool DoesDirectionMatchOrientation( SDockingNode::RelativeDirection InDirection, EOrientation InOrientation );

	static SDockingNode::ECleanupRetVal MostResponsibility( SDockingNode::ECleanupRetVal A, SDockingNode::ECleanupRetVal B );

	virtual SDockingNode::ECleanupRetVal CleanUpNodes() OVERRIDE;

	float ComputeChildCoefficientTotal() const;

	/**
	 * @param ParentNode    DockSplitter node whose children might be redundant
	 *
	 * @return true if we found some redundant splitters and need to do another pass on this list; false if we found no redundancies.
	 */
	static bool ClearRedundantNodes( const TSharedRef<SDockingSplitter>& ParentNode );

	/** The SSplitter widget that SDockingSplitter wraps. */
	TSharedPtr<SSplitter> Splitter;
	
	/** The DockNode children. All these children are kept in sync with the SSplitter's children via the use of the public interface for adding, removing and replacing children. */
	TArray< TSharedRef<class SDockingNode> > Children;
};


