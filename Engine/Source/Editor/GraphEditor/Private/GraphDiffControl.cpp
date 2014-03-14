// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "GraphEditorCommon.h"
#include "GraphDiffControl.h"
#include "BlueprintEditorUtils.h"

#define LOCTEXT_NAMESPACE "GraphDiffControl"

//If we are collecting all Diff results, keep going. If we just want to know if there *is* any diffs, we can early out
#define KEEP_GOING_IF_RESULTS() 	bHasResult = true;			\
if(!Results)					\
{break;	}					


/*******************************************************************************
* Static helper functions
*******************************************************************************/

/** Determine if two Nodes are the same */
static bool IsNodeMatch(class UEdGraphNode* Node1, class UEdGraphNode* Node2, TArray<FGraphDiffControl::FNodeMatch> const* Exclusions = NULL)
{
	if(Node2->GetClass() != Node1->GetClass()) 
	{
		return false;
	}

	if(Node1->NodeGuid == Node2->NodeGuid)
	{
		return true;
	}

	// we could be diffing two completely separate assets, this makes sure both 
	// nodes historically belong to the same graph
	bool bIsIntraAssetDiff = (Node1->GetGraph()->GraphGuid == Node2->GetGraph()->GraphGuid);

	// if both nodes are from the same graph
	if (bIsIntraAssetDiff)
	{
		return (Node1->GetFName() == Node2->GetFName());
	}

	if (Exclusions != NULL)
	{
		// have to see if this node has already been matched with another
		for (auto MatchIt(Exclusions->CreateConstIterator()); MatchIt; ++MatchIt)
		{
			FGraphDiffControl::FNodeMatch const& PriorMatch = *MatchIt;
			if (!PriorMatch.IsValid())
			{
				continue;
			}

			// if one of these nodes has already been matched to a different node
			if (((PriorMatch.OldNode == Node1) && (PriorMatch.NewNode != Node2)) ||
				((PriorMatch.OldNode == Node2) && (PriorMatch.NewNode != Node1)) ||
				((PriorMatch.NewNode == Node1) && (PriorMatch.OldNode != Node2)) ||
				((PriorMatch.NewNode == Node2) && (PriorMatch.OldNode != Node1)))
			{
				return false;
			}
		}
	}

	// the name hashes won't match for nodes from separate graph assets, so we 
	// need to look for some kind of semblance between the two... title? (what's displayed to the user)
	FString Title1 = Node1->GetNodeTitle(ENodeTitleType::FullTitle);
	FString Title2 = Node2->GetNodeTitle(ENodeTitleType::FullTitle);

	return (Title1 == Title2);
}

/** Diff result when a node was added to the graph */
static void DiffR_NodeAdded( FDiffResults& Results, UEdGraphNode* Node )
{
	if(Results)
	{
		FDiffSingleResult Diff;
		Diff.Diff = EDiffType::NODE_ADDED;
		Diff.Node1 = Node;
		Diff.ToolTip =  FString::Printf(*LOCTEXT("DIF_AddNode", "Added Node '%s'").ToString(), *Node->GetNodeTitle(ENodeTitleType::ListView));
		Diff.DisplayString = Diff.ToolTip;
		Diff.DisplayColor = FLinearColor(0.3f,1.0f,0.4f);
		Results.Add(Diff);
	}
}

/** Diff result when a node comment was changed */
static void DiffR_NodeCommentChanged(FDiffResults& Results, class UEdGraphNode* Node, class UEdGraphNode* Node2)
{
	if(Results)
	{
		FDiffSingleResult Diff;
		Diff.Diff = EDiffType::NODE_COMMENT;
		Diff.Node1 = Node;
		Diff.Node2 = Node2;
		Diff.ToolTip =  FString::Printf(*LOCTEXT("DIF_CommentModified", "Comment Modified Node '%s'").ToString(), *Node->GetNodeTitle(ENodeTitleType::ListView));
		Diff.DisplayString = Diff.ToolTip;
		Diff.DisplayColor = FLinearColor(0.25f,0.4f,0.5f);
		Results.Add(Diff);
	}
}

/** Diff result when a node was moved on the graph */
static void DiffR_NodeMoved(FDiffResults& Results, UEdGraphNode* Node, class UEdGraphNode* OtherNode)
{
	if(Results)
	{
		FDiffSingleResult Diff;
		Diff.Diff = EDiffType::NODE_MOVED;
		Diff.Node1 = Node;
		Diff.Node2 = OtherNode;
		Diff.ToolTip = FString::Printf(*LOCTEXT("DIF_MoveNode", "Moved Node '%s'").ToString(), *Node->GetNodeTitle(ENodeTitleType::ListView));
		Diff.DisplayString = Diff.ToolTip;
		Diff.DisplayColor = FLinearColor(0.9f, 0.84f, 0.43f);
		Results.Add(Diff);
	}
}

/** Diff result when a pin type was changed */
static void DiffR_PinTypeChanged(FDiffResults& Results, class UEdGraphPin* Pin2, class UEdGraphPin* Pin1)
{
	if(Results)
	{
		FEdGraphPinType Type1 = Pin1->PinType;
		FEdGraphPinType Type2 = Pin2->PinType;

		FDiffSingleResult Diff;

		const UObject* T1Obj = Type1.PinSubCategoryObject.Get();
		const UObject* T2Obj = Type2.PinSubCategoryObject.Get();

		if(Type1.PinCategory != Type2.PinCategory)
		{
			Diff.Diff = EDiffType::PIN_TYPE_CATEGORY;
			Diff.ToolTip =  FString::Printf(*LOCTEXT("DIF_PinCategoryToolTip", "Pin '%s' Category was '%s', but is now '%s").ToString(), *Pin2->PinName, *Pin1->PinType.PinCategory, *Pin2->PinType.PinCategory);
			Diff.DisplayColor = FLinearColor(0.15f,0.53f,0.15f);
			Diff.DisplayString =  FString::Printf(*LOCTEXT("DIF_PinCategory", "Pin Category '%s' ['%s' -> '%s']").ToString(), *Pin2->PinName, *Pin1->PinType.PinCategory, *Pin2->PinType.PinCategory);
		}
		else if(Type1.PinSubCategory != Type2.PinSubCategory)
		{
			Diff.Diff = EDiffType::PIN_TYPE_SUBCATEGORY;
			Diff.ToolTip =  FString::Printf(*LOCTEXT("DIF_PinSubCategoryToolTip", "Pin '%s' SubCategory was '%s', but is now '%s").ToString(), *Pin2->PinName, *Pin1->PinType.PinSubCategory,
				*Pin2->PinType.PinSubCategory);
			Diff.DisplayColor = FLinearColor(0.45f,0.53f,0.65f);
			Diff.DisplayString =   FString::Printf(*LOCTEXT("DIF_PinSubCategory", "Pin SubCategory '%s'  ['%s' -> '%s']").ToString(), *Pin2->PinName, *Pin1->PinType.PinSubCategory,
				*Pin2->PinType.PinSubCategory);
		}
		else if(T1Obj != T2Obj && (T1Obj && T2Obj ) &&
			(T1Obj->GetFName() != T2Obj->GetFName()))
		{
			FString Obj1 = T1Obj->GetFName().ToString();
			FString Obj2 = T2Obj->GetFName().ToString();

			Diff.Diff = EDiffType::PIN_TYPE_SUBCATEGORY_OBJECT;
			Diff.ToolTip =  FString::Printf(*LOCTEXT("DIF_PinSubCategorObjToolTip", "Pin '%s' was SubCategoryObject '%s', but is now '%s").ToString(), *Pin2->PinName, *Obj1, *Obj2);
			Diff.DisplayColor = FLinearColor(0.45f,0.13f,0.25f);
			Diff.DisplayString = FString::Printf(*LOCTEXT("DIF_PinSubCategoryObj", "Pin SubCategoryObject '%s' ['%s' -> '%s']").ToString(), *Pin2->PinName, *Obj1, *Obj2);
		}
		else if(Type1.bIsArray != Type2.bIsArray)
		{
			Diff.Diff = EDiffType::PIN_TYPE_IS_ARRAY;
			FString IsArray1 = Pin1->PinType.bIsArray ? TEXT("true") : TEXT("false");
			FString IsArray2 = Pin2->PinType.bIsArray ? TEXT("true") : TEXT("false");

			Diff.ToolTip =  FString::Printf(*LOCTEXT("DIF_PinIsArrayToolTip", "PinType IsArray for '%s' modified. Was '%s', but is now '%s").ToString(), *Pin2->PinName, *IsArray1, *IsArray2);
			Diff.DisplayString = FString::Printf(*LOCTEXT("DIF_PinIsArray", "Pin IsArray '%s' ['%s' -> '%s']").ToString(), *Pin2->PinName, *IsArray1, *IsArray2);
			Diff.DisplayColor = FLinearColor(0.45f,0.33f,0.35f);
		}
		else if(Type1.bIsReference != Type2.bIsReference)
		{
			Diff.Diff = EDiffType::PIN_TYPE_IS_REF;
			FString IsRef1 = Pin1->PinType.bIsReference ? TEXT("true") : TEXT("false");
			FString IsRef2 = Pin2->PinType.bIsReference ? TEXT("true") : TEXT("false");

			Diff.ToolTip =  FString::Printf(*LOCTEXT("DIF_PinIsRefToolTip", "PinType IsReference for '%s' modified. Was '%s', but is now '%s").ToString(), *Pin2->PinName, *IsRef1, *IsRef2);
			Diff.DisplayColor = FLinearColor(0.25f,0.43f,0.35f);
			Diff.DisplayString = FString::Printf(*LOCTEXT("DIF_PinIsRef", "Pin IsReference '%s' ['%s' -> '%s']").ToString(), *Pin2->PinName, *IsRef1, *IsRef2);
		}

		Diff.Pin1 = Pin1;
		Diff.Pin2 = Pin2;
		Results.Add(Diff);
	}
}

/** Diff result when the # of links to a pin was changed */
static void DiffR_PinLinkCountChanged(FDiffResults& Results, class UEdGraphPin* Pin2, class UEdGraphPin* Pin1)
{
	if(Results)
	{
		FDiffSingleResult Diff;
		Diff.Diff = Pin2->LinkedTo.Num() > Pin1->LinkedTo.Num()  ?  EDiffType::PIN_LINKEDTO_NUM_INC : EDiffType::PIN_LINKEDTO_NUM_DEC;
		Diff.Pin2 = Pin2;
		Diff.Pin1 = Pin1;
		if(Diff.Diff == EDiffType::PIN_LINKEDTO_NUM_INC)
		{
			Diff.ToolTip =  FString::Printf(*LOCTEXT("DIF_PinLinkCountIncToolTip", "Pin '%s' has more links(was %i now %i)").ToString(), *Pin1->PinName, Pin1->LinkedTo.Num() ,Pin2->LinkedTo.Num());
			Diff.DisplayColor = FLinearColor(0.5f,0.3f,0.85f);
			Diff.DisplayString =  FString::Printf(*LOCTEXT("DIF_PinLinkCountInc", "Added Link to '%s'").ToString(), *Pin1->PinName);
		}
		else
		{
			Diff.ToolTip =  FString::Printf(*LOCTEXT("DIF_PinLinkCountDecToolTip", "Pin '%s' has fewer links(was %i now %i)").ToString(), *Pin1->PinName, Pin1->LinkedTo.Num(), Pin2->LinkedTo.Num());
			Diff.DisplayColor = FLinearColor(0.5f,0.3f,0.85f);
			Diff.DisplayString =  FString::Printf(*LOCTEXT("DIF_PinLinkCountDec", "Removed Link to '%s'").ToString(), *Pin1->PinName);
		}

		Results.Add(Diff);
	}
}

/** Diff result when a pin to relinked to a different node */
static void DiffR_LinkedToNode(FDiffResults& Results, class UEdGraphPin* Pin1, class UEdGraphPin* Pin2,  class UEdGraphNode* Node1, class UEdGraphNode* Node2)
{
	if(Results)
	{
		FDiffSingleResult Diff;
		Diff.Diff = EDiffType::PIN_LINKEDTO_NODE;
		Diff.Pin2 = Pin2;
		Diff.Pin1 = Pin1;
		Diff.Node1 = Node1;
		Diff.Node2 = Node2;

		FString Node1Name = *Node1->GetNodeTitle(ENodeTitleType::ListView);
		FString Node2Name = *Node2->GetNodeTitle(ENodeTitleType::ListView);
		Diff.ToolTip =  FString::Printf(*LOCTEXT("DIF_PinLinkMovedToolTip", "Pin '%s' was linked to Node '%s', but is now linked to Node '%s'").ToString(), *Pin1->PinName,  *Node1Name, *Node2Name);
		Diff.DisplayColor = FLinearColor(0.85f,0.71f,0.25f);
		Diff.DisplayString =   FString::Printf(*LOCTEXT("DIF_PinLinkMoved", "Link Moved  '%s' ['%s' -> '%s']").ToString(), *Pin1->PinName,  *Node1Name, *Node2Name);
		Results.Add(Diff);
	}
}

/** Diff result when a pin default value was changed, and is in use*/
static void DiffR_PinDefaultValueChanged(FDiffResults& Results, class UEdGraphPin* Pin2, class UEdGraphPin* Pin1)
{
	if(Results)
	{
		FDiffSingleResult Diff;
		Diff.Diff = EDiffType::PIN_DEFAULT_VALUE;
		Diff.Pin1 = Pin1;
		Diff.Pin2 = Pin2;
		Diff.ToolTip =  FString::Printf(*LOCTEXT("DIF_PinDefaultValueToolTip", "Pin '%s' Default Value was '%s', but is now '%s").ToString(), *Pin2->PinName, 
			*Pin1->GetDefaultAsString(), *Pin2->GetDefaultAsString());
		Diff.DisplayColor = FLinearColor(0.665f,0.13f,0.455f);
		Diff.DisplayString =   FString::Printf(*LOCTEXT("DIF_PinDefaultValue", "Pin Default '%s' ['%s' -> '%s']").ToString(), *Pin2->PinName, *Pin1->GetDefaultAsString(), 
			*Pin2->GetDefaultAsString());
		Results.Add(Diff);
	}
}

/** 
 * Diff result when pin count is not the same.
 *
 * @param	Result	Struct to receive result.
 * @param	Node	Node 1 to which the pins relate.
 * @param	Node2	Node 2 to which the pins relate.
 * @param	Pins1	Array of relevant pins on Node 1.
 * @param	Pins2	Array of relevant pins on Node 2.
 */
static void DiffR_NodePinCount(FDiffResults& Results, class UEdGraphNode* Node, class UEdGraphNode* Node2, const TArray<class UEdGraphPin*> &Pins1, const TArray<class UEdGraphPin*> &Pins2)
{
	if(Results)
	{
		FString NodeName = Node->GetNodeTitle(ENodeTitleType::ListView);
		int32 OriginalCount = Pins2.Num();
		int32 NewCount = Pins1.Num();
		FDiffSingleResult Diff;
		Diff.Diff = EDiffType::NODE_PIN_COUNT;
		Diff.Node1 = Node;
		Diff.Node2 = Node2;
		Diff.ToolTip =  FString::Printf(*LOCTEXT("DIF_NodePinCountChangedToolTip", "Node '%s' had %i Pins, now has %i Pins").ToString(), *NodeName, OriginalCount, NewCount);
		Diff.DisplayColor = FLinearColor(0.45f,0.4f,0.4f);
		Diff.DisplayString =  (OriginalCount < NewCount) ?  (FString::Printf(*LOCTEXT("DIF_NodePinCountIncreased", "Add Pin '%s'").ToString(), *NodeName)) : 
			(FString::Printf(*LOCTEXT("DIF_NodePinCountDecreased", "Removed Pin '%s'").ToString(), *NodeName));
		Results.Add(Diff);
	}
}

/** 
 * Populate an array one of pins from another disregarding irrelevant ones (EG invisible). 
 * 
 * @param	InPins			Pins
 * @param	OutRelevanPins	Output of Relevant pins
 */
static void BuildArrayOfRelevantPins(const TArray<class UEdGraphPin*>& InPins, TArray< UEdGraphPin* >& OutRelevantPins)
{
	for(int32 i = 0;i<InPins.Num();++i)
	{
		UEdGraphPin* EachPin = InPins[i];
		if( EachPin != NULL )
		{
			if( EachPin->bHidden == false )
			{
				OutRelevantPins.Add( EachPin );
			}			
		}
	}
}

static bool IsPinTypeDifferent(const FEdGraphPinType& T1, const FEdGraphPinType& T2) 
{
	bool bIsDifferent =  (T1.PinCategory != T2.PinCategory) 
		|| (T1.PinSubCategory != T2.PinSubCategory) 
		|| (T1.bIsArray != T2.bIsArray) 
		|| (T1.bIsReference != T2.bIsReference);

	const UObject* T1Obj = T1.PinSubCategoryObject.Get();
	const UObject* T2Obj = T2.PinSubCategoryObject.Get();
	//TODO: fix, this code makes no sense
	if((T1Obj != T2Obj) && (T1Obj && T2Obj) && (T1Obj->GetFName() != T2Obj->GetFName())) 
	{
		bIsDifferent |= T1Obj->GetFName() == T2Obj->GetFName();
	}
	return bIsDifferent;
}

/** Find Pin in array that matches pin */
static class UEdGraphPin* FindOtherLink(TArray<class UEdGraphPin*>& Links2, int32 OriginalIndex, class UEdGraphPin* PinToFind)
{
	//sometimes the order of the pins is different between revisions, although the pins themselves are unchanged, so we have to look at all of them
	UEdGraphNode* Node1 = PinToFind->GetOwningNode();
	for(auto It(Links2.CreateIterator());It;++It)
	{
		UEdGraphPin* Other = *It;
		UEdGraphNode* Node2 = Other->GetOwningNode();
		if(IsNodeMatch(Node1, Node2))
		{
			return Other;
		}
	}
	return Links2[OriginalIndex];
}

/** Determine if the LinkedTo pins are the same */
static bool LinkedToDifferent(class UEdGraphPin* OriginalPin1, class UEdGraphPin* OriginalPin2, const TArray<class UEdGraphPin*>& Links1, TArray<class UEdGraphPin*>& Links2, FDiffResults& Results)
{
	const int32 Size = Links1.Num();
	bool bHasResult = false;
	for(int32 i = 0;i<Size;++i)
	{
		UEdGraphPin* Pin1 = Links1[i];
		UEdGraphPin* Pin2 = FindOtherLink(Links2, i, Pin1);

		UEdGraphNode* Node1 = Pin1->GetOwningNode();
		UEdGraphNode* Node2 = Pin2->GetOwningNode();
		if(!IsNodeMatch(Node1, Node2))
		{
			DiffR_LinkedToNode(Results, OriginalPin1, OriginalPin2, Node1, Node2);
			KEEP_GOING_IF_RESULTS()
		}
	}
	return bHasResult;
}

/** 
 * Determine of two Arrays of Pins are different.
 *
 * @param	Pins1	First set of pins to compare.
 * @param	Pins2	Second set of pins to compare.
 * @param	Results Difference results.
 * 
 * returns true if any pins are different and populates the Results array
 */
static bool ArePinsDifferent(const TArray<class UEdGraphPin*>& Pins1, TArray<class UEdGraphPin*>& Pins2, FDiffResults& Results)
{
	const int32 Size = Pins1.Num();
	bool bHasResult = false;
	for(int32 i = 0;i<Size;++i)
	{
		UEdGraphPin* Pin1 = Pins1[i];
		UEdGraphPin* Pin2 = Pins2[i];

		if(IsPinTypeDifferent(Pin1->PinType, Pin2->PinType))
		{
			DiffR_PinTypeChanged(Results, Pin2, Pin1);
			KEEP_GOING_IF_RESULTS()
		}
		if(Pin1->LinkedTo.Num() != Pin2->LinkedTo.Num())
		{
			DiffR_PinLinkCountChanged(Results, Pin2, Pin1);
			KEEP_GOING_IF_RESULTS()
		}
		else if(LinkedToDifferent(Pin1, Pin2,Pin1->LinkedTo, Pin2->LinkedTo, Results))
		{
			KEEP_GOING_IF_RESULTS()
		}

		if(Pin2->LinkedTo.Num() == 0 && (Pin2->GetDefaultAsString() != Pin1->GetDefaultAsString()))
		{
			DiffR_PinDefaultValueChanged(Results, Pin2, Pin1); //note: some issues with how floating point is stored as string format(0.0 vs 0.00) can cause false diffs
			KEEP_GOING_IF_RESULTS()
		}
	}
	return bHasResult;
}

/*******************************************************************************
* FGraphDiffControl::FNodeMatch
*******************************************************************************/

bool FGraphDiffControl::FNodeMatch::IsValid() const
{
	return ((NewNode != NULL) && (OldNode != NULL));
}

bool FGraphDiffControl::FNodeMatch::Diff(TArray<FDiffSingleResult>* OptionalDiffsArray /* = NULL*/) const
{
	FDiffResults DiffsOut(OptionalDiffsArray);
	bool bIsDifferent = false;

	if (IsValid())
	{
		//has comment changed?
		if(NewNode->NodeComment != OldNode->NodeComment)
		{
			DiffR_NodeCommentChanged(DiffsOut, NewNode, OldNode);
			bIsDifferent = true;
		}

		//has it moved?
		if( (NewNode->NodePosX != OldNode->NodePosX) || (NewNode->NodePosY != OldNode->NodePosY) )
		{
			//same node, different position--
			DiffR_NodeMoved(DiffsOut, NewNode, OldNode);
			bIsDifferent = true;
		}

		// Build arrays of pins that we care about
		TArray< UEdGraphPin* > OldRelevantPins;
		TArray< UEdGraphPin* > RelevantPins;
		BuildArrayOfRelevantPins(OldNode->Pins, OldRelevantPins);
		BuildArrayOfRelevantPins(NewNode->Pins, RelevantPins);

		if(OldRelevantPins.Num() == RelevantPins.Num())
		{
			//checks contents of pins
			bIsDifferent |= ArePinsDifferent(OldRelevantPins, RelevantPins, DiffsOut);
		}
		else//# of pins changed
		{
			DiffR_NodePinCount(DiffsOut, NewNode, OldNode, RelevantPins, OldRelevantPins);
			bIsDifferent = true;
		}

		//Find internal node diffs; skip this if we don't need the result data
		if(!bIsDifferent || DiffsOut)
		{
			OldNode->FindDiffs(NewNode, DiffsOut);
			bIsDifferent |= DiffsOut.HasFoundDiffs();
		}
	}
	else 
	// one of the nodes is NULL
	{
		bIsDifferent = true;
		DiffR_NodeAdded(DiffsOut, NewNode);
	}

	return bIsDifferent;
}

/*******************************************************************************
* FGraphDiffControl
*******************************************************************************/

FGraphDiffControl::FNodeMatch FGraphDiffControl::FindNodeMatch(class UEdGraph* Graph, class UEdGraphNode* Node, TArray<FNodeMatch> const& PriorMatches)
{
	FNodeMatch Match;
	Match.NewNode = Node;

	if (Graph != NULL)
	{
		// attempt to find a node matching 'Node'
		for (auto NodeIt(Graph->Nodes.CreateIterator()); NodeIt; ++NodeIt) 
		{
			UEdGraphNode* GraphNode = *NodeIt;
			if (GraphNode == NULL)
			{
				continue;
			}

			if (IsNodeMatch(Node, GraphNode, &PriorMatches))
			{
				Match.OldNode = GraphNode;
				break;
			}		
		}
	}	

	return Match;
}

bool FGraphDiffControl::DiffGraphs(UEdGraph* const LhsGraph, UEdGraph* const RhsGraph, TArray<FDiffSingleResult>& DiffsOut)
{
	bool bFoundDifferences = false;

	if ((LhsGraph != NULL) && (RhsGraph != NULL))
	{
		TArray<FGraphDiffControl::FNodeMatch> NodeMatches;
		TSet<UEdGraphNode const*> MatchedRhsNodes;

		// march through the all the nodes in the lhs graph and look for matches 
		for (auto NodeIt(LhsGraph->Nodes.CreateConstIterator()); NodeIt; ++NodeIt)
		{
			UEdGraphNode* const LhsNode = *NodeIt;
			if (LhsNode == NULL)
			{
				continue;
			}

			FGraphDiffControl::FNodeMatch NodeMatch = FGraphDiffControl::FindNodeMatch(RhsGraph, LhsNode, NodeMatches);
			// if we found a corresponding node in the rhs graph, track it (so we
			// can prevent future matches with the same nodes)
			if (NodeMatch.IsValid())
			{
				NodeMatches.Add(NodeMatch);
				MatchedRhsNodes.Add(NodeMatch.OldNode);
			}

			bFoundDifferences |= NodeMatch.Diff(&DiffsOut);
		}

		// go through the rhs nodes to catch ones that may have been missing from the lhs graph
		for (auto NodeIt(RhsGraph->Nodes.CreateConstIterator()); NodeIt; ++NodeIt)
		{
			UEdGraphNode* const RhsNode = *NodeIt;
			// if this node has already been matched, move on
			if ((RhsNode == NULL) || MatchedRhsNodes.Find(RhsNode))
			{
				continue;
			}

			FGraphDiffControl::FNodeMatch NodeMatch = FGraphDiffControl::FindNodeMatch(LhsGraph, RhsNode, NodeMatches);

			TArray<FDiffSingleResult> RhsDiffs;
			if (NodeMatch.Diff(&RhsDiffs))
			{
				for (auto DiffIt(RhsDiffs.CreateIterator()); DiffIt; ++DiffIt)
				{
					FDiffSingleResult& Difference = *DiffIt;

					// only looking for "adds" because all other diffs would have been caught earlier
					if (Difference.Diff == EDiffType::NODE_ADDED) 
					{
						// flip the "add" to a "removed" because all the diffs are
						// listed in a context of "what has changed from lhs to rhs?"
						Difference.Diff	         = EDiffType::NODE_REMOVED;
						Difference.ToolTip       = FString::Printf(*LOCTEXT("DIF_RemoveNode", "Removed Node '%s'").ToString(), *Difference.Node1->GetNodeTitle(ENodeTitleType::ListView));
						Difference.DisplayString = Difference.ToolTip;
						Difference.DisplayColor  = FLinearColor(1.f,0.4f,0.4f);

						bFoundDifferences = true;
						DiffsOut.Add(Difference);
					}
				}
			}
		}
	}

	return bFoundDifferences;
}

#undef LOCTEXT_NAMESPACE
