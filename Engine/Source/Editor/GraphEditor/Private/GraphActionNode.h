// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

// Utility class for building menus of graph actions

#ifndef __GraphActionNode_h__
#define __GraphActionNode_h__

struct FGraphActionNode : TSharedFromThis<FGraphActionNode>
{
	/** The section this node belongs to (if any) */
	int32 SectionID;

	/** The category this node belongs in. */
	FString Category;

	int32 Grouping;
	TArray< TSharedPtr<FEdGraphSchemaAction> > Actions;

	/** When the item is first created, a rename request may occur before everything is setup for it. This toggles to true in those cases */
	bool bIsRenameRequestBeforeReady;

	/** Broadcasts whenever renaming a layer is requested */
	FOnRenameRequestActionNode& OnRenameRequest() { return RenameRequestEvent; }
	void BroadcastRenameRequest() { RenameRequestEvent.ExecuteIfBound(); }

	/** Broadcasts whenever a rename is requested */
	FOnRenameRequestActionNode RenameRequestEvent;

	TArray< TSharedPtr<FGraphActionNode> > Children;
public:
	static TSharedPtr<FGraphActionNode> NewAction( TSharedPtr<FEdGraphSchemaAction> InAction )
	{
		return MakeShareable( new FGraphActionNode( TEXT(""), (InAction.IsValid() ? InAction->Grouping : 0), InAction) );
	}

	static TSharedPtr<FGraphActionNode> NewAction( TArray< TSharedPtr<FEdGraphSchemaAction> >& InActionList )
	{
		int32 Grouping = 0;
		int32 SectionID = 0;
		for ( int32 ActionIndex = 0; ActionIndex < InActionList.Num(); ActionIndex++ )
		{
			TSharedPtr<FEdGraphSchemaAction> CurrentAction = InActionList[ActionIndex];
			if ( CurrentAction.IsValid() )
			{
				Grouping = FMath::Max( CurrentAction->Grouping, Grouping );
				// Copy the section ID if we don't already have one
				if( SectionID == 0 )
				{
					SectionID = CurrentAction->SectionID;
				}
			}
		}

		return MakeShareable( new FGraphActionNode( TEXT(""), Grouping, InActionList, SectionID ) );
	}

	static TSharedPtr<FGraphActionNode> NewCategory( const FString& InCategory, int32 InGrouping = 0 )
	{
		return MakeShareable( new FGraphActionNode(InCategory, InGrouping, NULL) );
	}

	static TSharedPtr<FGraphActionNode> NewSeparator( int32 InGrouping = 0 )
	{
		return MakeShareable( new FGraphActionNode(TEXT(""), InGrouping, NULL) );
	}

	static TSharedPtr<FGraphActionNode> NewSeparatorWithTitle( int32 InGrouping, int32 InNodeSection )
	{
		return MakeShareable( new FGraphActionNode( InGrouping, InNodeSection) );
	}

	bool IsActionNode() const
	{
		return (Actions.Num() > 0 && Actions[0].IsValid());
	}

	bool IsCategoryNode() const
	{
		return (Actions.Num()==0 || !Actions[0].IsValid()) && !Category.IsEmpty();
	}

	bool IsSeparator() const
	{
		return (Actions.Num()==0 || !Actions[0].IsValid()) && Category.IsEmpty();
	}

	void AddChild(TSharedPtr<FGraphActionNode> NodeToAdd, TArray<FString>& CategoryStack, bool bAlphaSort)
	{
		// If we still have some categories in the chain to process...
		if( CategoryStack.Num() )
		{
			// ...check to see if we have any 
			const FString NextCategory = CategoryStack[0];
			CategoryStack.RemoveAt(0,1);
			bool bFoundMatch = false;
			for( int32 i = 0; i < Children.Num(); i++ )
			{
				TSharedPtr<FGraphActionNode>& CurrentChild = Children[i];
				if(CurrentChild->IsCategoryNode())
				{
					bFoundMatch = NextCategory == CurrentChild->Category;
				}
				else if(CurrentChild->IsActionNode())
				{
					if ( NodeToAdd->IsActionNode() )
					{
						TSharedPtr<FEdGraphSchemaAction> CurrentAction = NodeToAdd->Actions[0];

						bFoundMatch = CurrentAction->IsParentable()
							&& NextCategory == CurrentChild->Actions[0]->MenuDescription;
					}
				}
				
				if( bFoundMatch )
				{
					CurrentChild->AddChild(NodeToAdd, CategoryStack, bAlphaSort);
					break;
				}
			}

			if( !bFoundMatch )
			{
				TSharedPtr<FGraphActionNode> NewCategoryNode = NewCategory(NextCategory, NodeToAdd->Grouping);
				// Copy the section ID also 
				NewCategoryNode->SectionID = NodeToAdd->SectionID;
				AddToChildrenSorted(NewCategoryNode, bAlphaSort);
				NewCategoryNode->AddChild(NodeToAdd, CategoryStack, bAlphaSort);
			}
		}
		else
		{
			// End of the category chain, add it to this level!
			AddToChildrenSorted(NodeToAdd, bAlphaSort);
		}
	}

	/**
	* Iterates through all this node's children, and tells the tree view to expand them
	*/
	void ExpandAllChildren( TSharedPtr< STreeView< TSharedPtr<FGraphActionNode> > > TreeView )
	{
		if( Children.Num() )
		{
			TreeView->SetItemExpansion(this->AsShared(), true);
			for( int32 i = 0; i < Children.Num(); i++ )
			{
				Children[i]->ExpandAllChildren(TreeView);
			}
		}
	}

	void ClearChildren()
	{
		Children.Empty();
	}

	void AddToChildrenSorted(TSharedPtr<FGraphActionNode> NodeToAdd, bool bAlphaSort)
	{
 		if( NodeToAdd->SectionID != 0 )
 		{
 			return AddToChildrenSortedWithSection( NodeToAdd, bAlphaSort );
 		}
		// Run over the children list, and break when we figure out where we belong
		int32 i;
		for(i = 0; i < Children.Num(); i++)
		{
			if( NodeCompare(NodeToAdd, Children[i], bAlphaSort) )
			{
				break;
			}
		}

		// Check to see if this is the first node in a boundary between groupings, and add a separator if so
		const int32 CurrentChildren = Children.Num();
		if( i < CurrentChildren - 1 )
		{
			TSharedPtr<FGraphActionNode> PreviousEntry = Children[i];
			if( NodeToAdd->Grouping != PreviousEntry->Grouping )
			{
				Children.Insert( NewSeparator(NodeToAdd->Grouping), i );
			}
		}

		// Finally, add the item
		Children.Insert(NodeToAdd, i);
	}

	
	void AddToChildrenSortedWithSection(TSharedPtr<FGraphActionNode> NodeToAdd, bool bAlphaSort)
	{
		// Run over the children list, and break when we figure out where we belong
		int32 i;
		for(i = 0; i < Children.Num(); i++)
		{
			if( NodeCompareWithSection(NodeToAdd, Children[i], bAlphaSort) )
			{
				break;
			}
		}

		const int32 CurrentChildren = Children.Num();
		bool AddGroup = false;		
		if ( CurrentChildren != 0)
		{
			// If we have any children check if the previous section matches, if not, insert a new one
			int32 PreviousIndex = FMath::Clamp( i-1, 0, CurrentChildren - 1 );
			TSharedPtr<FGraphActionNode> PreviousEntry = Children[PreviousIndex];
			if( NodeToAdd->SectionID != PreviousEntry->SectionID )
			{
				AddGroup = true;
			}
		}
		else
		{
			// There are no children so we need to create a section
			AddGroup = true;
		}
		// Override the group requirement if this node is in a category (only variables are shown in categories anyway - no point in adding separators)
		if( (NodeToAdd->Actions.Num() != 0 ) && ( NodeToAdd->Actions[0].IsValid() == true ) && (!NodeToAdd->Actions[0]->Category.IsEmpty()) )
		{
			AddGroup = false;
		}
		// If we want a group - add it now
		if( AddGroup )
		{
			Children.Insert( NewSeparatorWithTitle(NodeToAdd->Grouping, NodeToAdd->SectionID), i );
			i++;
		}		

		// Finally, add the item
		Children.Insert(NodeToAdd, i);
	}

	/** Comparison function for sorted insertion:  returns true when NodeA belongs before NodeB, false otherwise */
	bool NodeCompareWithSection(TSharedPtr<FGraphActionNode> NodeA, TSharedPtr<FGraphActionNode> NodeB, bool bAlphaSort)
	{
		const bool bNodeAIsSeparator = NodeA->IsSeparator();
		const bool bNodeBIsSeparator = NodeB->IsSeparator();

		const bool bNodeAIsCategory = NodeA->IsCategoryNode();
		const bool bNodeBIsCategory = NodeB->IsCategoryNode();

		if( ( NodeA->SectionID == NodeB->SectionID) && ( bNodeAIsSeparator == true ) )
		{
			return false;
		}
		if( NodeA->SectionID != NodeB->SectionID )
		{
			return NodeA->SectionID < NodeB->SectionID;
		}
		else if ( bNodeBIsCategory == true ) 
		{
			// If next node is a category and we are not, we go first
			if( bNodeAIsCategory == false )
			{
				return true;
			}

			// Both nodes must categories - insert me here if this is my category
			return (NodeA->Category == NodeB->Category);		
		}		
		else
		{
			if(bAlphaSort)
			{
				// Finally, alphabetize
				if( bNodeAIsCategory )
				{
					return (NodeA->Category <= NodeB->Category);
				}
				else
				{
					return (NodeA->Actions[0]->MenuDescription <= NodeB->Actions[0]->MenuDescription);
				}
			}
			else
			{
				return false;
			}
		}
	}
	/** Comparison function for sorted insertion:  returns true when NodeA belongs before NodeB, false otherwise */
	bool NodeCompare(TSharedPtr<FGraphActionNode> NodeA, TSharedPtr<FGraphActionNode> NodeB, bool bAlphaSort)
	{
		const bool bNodeAIsSeparator = NodeA->IsSeparator();
		const bool bNodeBIsSeparator = NodeB->IsSeparator();

		const bool bNodeAIsCategory = NodeA->IsCategoryNode();
		const bool bNodeBIsCategory = NodeB->IsCategoryNode();

		// Grouping trumps all
		if( NodeA->Grouping != NodeB->Grouping )
		{
			return NodeA->Grouping >= NodeB->Grouping;
		}
		// Next, make sure separators are preserved
		else if( bNodeAIsSeparator != bNodeBIsSeparator )
		{
			return bNodeBIsSeparator;
		}
		// Next, categories come before terminals
		else if( bNodeAIsCategory != bNodeBIsCategory )
		{
			return bNodeAIsCategory;
		}
		else
		{
			if(bAlphaSort)
			{
				// Finally, alphabetize
				if( bNodeAIsCategory )
				{
					return (NodeA->Category <= NodeB->Category);
				}
				else
				{
					return (NodeA->Actions[0]->MenuDescription <= NodeB->Actions[0]->MenuDescription);
				}
			}
			else
			{
				return false;
			}
		}
	}

	void GetLeafNodes(TArray< TSharedPtr<FGraphActionNode> >& LeafArray)
	{
		for (int32 ChildIndex = 0; ChildIndex < Children.Num(); ++ChildIndex)
		{
			TSharedPtr<FGraphActionNode>& Child = Children[ChildIndex];

			if (Child->IsCategoryNode())
			{
				Child->GetLeafNodes(LeafArray);
			}
			else
			{
				LeafArray.Add(Child);
			}
		}
	}

	void GetAllNodes(TArray< TSharedPtr<FGraphActionNode> >& OutNodes, bool bLeavesOnly)
	{
		for (int32 ChildIndex = 0; ChildIndex < Children.Num(); ++ChildIndex)
		{
			TSharedPtr<FGraphActionNode>& Child = Children[ChildIndex];

			// Add us if a leaf, or we want all nodes (not just leaves)
			if(!Child->IsCategoryNode() || !bLeavesOnly)
			{
				OutNodes.Add(Child);
			}

			// If a category, recurse into childen
			if (Child->IsCategoryNode())
			{
				Child->GetAllNodes(OutNodes, bLeavesOnly);
			}
		}
	}

protected:

	FGraphActionNode( FString InCategory, int32 InGrouping, TSharedPtr<FEdGraphSchemaAction> InAction )
		: Category( InCategory )
		, Grouping( InGrouping )
		, bIsRenameRequestBeforeReady( false )
		, SectionID ( 0 )
	{
		Actions.Add( InAction );
	}

	FGraphActionNode( FString InCategory, int32 InGrouping, TArray< TSharedPtr<FEdGraphSchemaAction> >& InActionList, int32 InSectionID = 0 )
	: Category( InCategory )
	, Grouping( InGrouping )
	, bIsRenameRequestBeforeReady( false )
	, SectionID ( InSectionID )
	{
		Actions.Append( InActionList );
	}

	FGraphActionNode( int32 InGrouping, int32 InNodeSection )		
		: Grouping( InGrouping )
		, bIsRenameRequestBeforeReady( false )
		, SectionID ( InNodeSection )
	{
		Actions.Add( NULL );		
	}
};

#endif // __GraphActionNode_h__
