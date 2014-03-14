// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BehaviorTreeEditorPrivatePCH.h"
#include "BlueprintGraphDefinitions.h"
#include "GraphEditorActions.h"
#include "BehaviorTreeConnectionDrawingPolicy.h"
#include "ScopedTransaction.h"
#include "SGraphEditorImpl.h"
#include "Toolkits/ToolkitManager.h"

#define LOCTEXT_NAMESPACE "BehaviorTreeSchema"
#define SNAP_GRID (16) // @todo ensure this is the same as SNodePanel::GetSnapGridSize()

namespace 
{
	// Maximum distance a drag can be off a node edge to require 'push off' from node
	const int32 NodeDistance = 60;
}

struct FBBNode
{
	FVector2D SubGraphBBox;
	TArray<FBBNode*> Children;

	FBBNode()
	{
		SubGraphBBox = FVector2D(0,0);
	}
};

void FillTreeSizeInfo(UBehaviorTreeGraphNode* ParentNode, FBBNode& BBoxTree)
{
	BBoxTree.SubGraphBBox = ParentNode->NodeWidget.Pin()->GetDesiredSize();
	float LevelWidth = 0;
	float LevelHeight = 0;
	for (int32 i = 0; i < ParentNode->Pins.Num(); i++)
	{
		if (ParentNode->Pins[i]->Direction == EGPD_Output)
		{
			UEdGraphPin* Pin =  ParentNode->Pins[i];
			// sort connections so that they're organized the same as user can see in the editor
			Pin->LinkedTo.Sort(FCompareNodeXLocation());
			for (int32 Index=0; Index < Pin->LinkedTo.Num(); ++Index)
			{
				UBehaviorTreeGraphNode* GraphNode = Cast<UBehaviorTreeGraphNode>(Pin->LinkedTo[Index]->GetOwningNode());
				if (GraphNode != NULL)
				{
					FBBNode* ChildBBNode = new FBBNode();
					BBoxTree.Children.Add(ChildBBNode);
					FillTreeSizeInfo(GraphNode, *ChildBBNode);
					LevelWidth += ChildBBNode->SubGraphBBox.X + 20;
					if (ChildBBNode->SubGraphBBox.Y > LevelHeight)
					{
						LevelHeight = ChildBBNode->SubGraphBBox.Y;
					}			
				}
			}
			if (LevelWidth > BBoxTree.SubGraphBBox.X )
			{
				BBoxTree.SubGraphBBox.X = LevelWidth;
			}
			BBoxTree.SubGraphBBox.Y += LevelHeight;
		}
	}
}

void AutoArrangeNodes(UBehaviorTreeGraphNode* ParentNode, FBBNode& BBoxTree, float PosX, float PosY )
{
	int32 BBoxIndex = 0;
	for (int32 i = 0; i < ParentNode->Pins.Num(); i++)
	{
		if ( ParentNode->Pins[i]->Direction == EGPD_Output)
		{
			UEdGraphPin* Pin =  ParentNode->Pins[i];
			for (int32 Index=0; Index < Pin->LinkedTo.Num(); ++Index)
			{
				UBehaviorTreeGraphNode* GraphNode = Cast<UBehaviorTreeGraphNode>(Pin->LinkedTo[Index]->GetOwningNode());
				if (GraphNode != NULL && BBoxTree.Children.Num() > 0)
				{
					AutoArrangeNodes(GraphNode, *BBoxTree.Children[BBoxIndex], PosX, PosY + GraphNode->NodeWidget.Pin()->GetDesiredSize().Y * 2.5f);
					GraphNode->NodeWidget.Pin()->MoveTo(FVector2D(BBoxTree.Children[BBoxIndex]->SubGraphBBox.X /2 - GraphNode->NodeWidget.Pin()->GetDesiredSize().X /2 + PosX, PosY));
					PosX += BBoxTree.Children[BBoxIndex]->SubGraphBBox.X + 20;
				}
				BBoxIndex++;
			}
		}
	}
}


UEdGraphNode* FBehaviorTreeSchemaAction_AutoArrange::PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
{
	FBBNode BBoxTree;
	UBehaviorTreeGraphNode* RootNode = NULL;
	for (int32 i = 0; i < ParentGraph->Nodes.Num(); i++)
	{
		if (Cast<UBehaviorTreeGraphNode_Root>(ParentGraph->Nodes[i]) != NULL)
		{
			RootNode = Cast<UBehaviorTreeGraphNode_Root>(ParentGraph->Nodes[i]);
		}
	}

	FillTreeSizeInfo(RootNode, BBoxTree);
	AutoArrangeNodes(RootNode, BBoxTree, 0, RootNode->NodeWidget.Pin()->GetDesiredSize().Y * 2.5f);
	
	RootNode->NodePosX = BBoxTree.SubGraphBBox.X / 2 - RootNode->NodeWidget.Pin()->GetDesiredSize().X / 2;
	RootNode->NodePosY = 0;

	RootNode->NodeWidget.Pin()->GetOwnerPanel()->ZoomToFit(/*bOnlySelection=*/ false);
	return NULL;
}

UEdGraphNode* FBehaviorTreeSchemaAction_NewNode::PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
{
	UEdGraphNode* ResultNode = NULL;

	// If there is a template, we actually use it
	if (NodeTemplate != NULL)
	{
		const FScopedTransaction Transaction(LOCTEXT("AddNode", "Add Node"));
		ParentGraph->Modify();
		if (FromPin)
		{
			FromPin->Modify();
		}

		NodeTemplate->SetFlags(RF_Transactional);

		// set outer to be the graph so it doesn't go away
		NodeTemplate->Rename(NULL, ParentGraph, REN_NonTransactional);
		ParentGraph->AddNode(NodeTemplate, true);

		NodeTemplate->CreateNewGuid();
		NodeTemplate->PostPlacedNewNode();
		NodeTemplate->AllocateDefaultPins();
		NodeTemplate->AutowireNewNode(FromPin);

		// For input pins, new node will generally overlap node being dragged off
		// Work out if we want to visually push away from connected node
		int32 XLocation = Location.X;
		if (FromPin && FromPin->Direction == EGPD_Input)
		{
			UEdGraphNode* PinNode = FromPin->GetOwningNode();
			const float XDelta = FMath::Abs(PinNode->NodePosX - Location.X);

			if (XDelta < NodeDistance)
			{
				// Set location to edge of current node minus the max move distance
				// to force node to push off from connect node enough to give selection handle
				XLocation = PinNode->NodePosX - NodeDistance;
			}
		}

		NodeTemplate->NodePosX = XLocation;
		NodeTemplate->NodePosY = Location.Y;
		NodeTemplate->SnapToGrid(SNAP_GRID);

		ResultNode = NodeTemplate;
	}

	return ResultNode;
}

UEdGraphNode* FBehaviorTreeSchemaAction_NewNode::PerformAction(class UEdGraph* ParentGraph, TArray<UEdGraphPin*>& FromPins, const FVector2D Location, bool bSelectNewNode) 
{
	UEdGraphNode* ResultNode = NULL;
	if (FromPins.Num() > 0)
	{
		ResultNode = PerformAction(ParentGraph, FromPins[0], Location);

		// Try autowiring the rest of the pins
		for (int32 Index = 1; Index < FromPins.Num(); ++Index)
		{
			ResultNode->AutowireNewNode(FromPins[Index]);
		}
	}
	else
	{
		ResultNode = PerformAction(ParentGraph, NULL, Location, bSelectNewNode);
	}

	return ResultNode;
}

void FBehaviorTreeSchemaAction_NewNode::AddReferencedObjects( FReferenceCollector& Collector )
{
	FEdGraphSchemaAction::AddReferencedObjects( Collector );

	// These don't get saved to disk, but we want to make sure the objects don't get GC'd while the action array is around
	Collector.AddReferencedObject( NodeTemplate );
}




UEdGraphNode* FBehaviorTreeSchemaAction_NewSubNode::PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
{
	ParentNode->AddSubNode(NodeTemplate,ParentGraph);
	return NULL;
}

UEdGraphNode* FBehaviorTreeSchemaAction_NewSubNode::PerformAction(class UEdGraph* ParentGraph, TArray<UEdGraphPin*>& FromPins, const FVector2D Location, bool bSelectNewNode) 
{
	return PerformAction(ParentGraph, NULL, Location, bSelectNewNode);
}

void FBehaviorTreeSchemaAction_NewSubNode::AddReferencedObjects( FReferenceCollector& Collector )
{
	FEdGraphSchemaAction::AddReferencedObjects( Collector );

	// These don't get saved to disk, but we want to make sure the objects don't get GC'd while the action array is around
	Collector.AddReferencedObject( NodeTemplate );
}
//////////////////////////////////////////////////////////////////////////

UEdGraphSchema_BehaviorTree::UEdGraphSchema_BehaviorTree(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
}

TSharedPtr<FBehaviorTreeSchemaAction_NewNode> AddNewNodeAction(FGraphContextMenuBuilder& ContextMenuBuilder, const FString& Category, const FString& MenuDesc, const FString& Tooltip)
{
	TSharedPtr<FBehaviorTreeSchemaAction_NewNode> NewAction = TSharedPtr<FBehaviorTreeSchemaAction_NewNode>(new FBehaviorTreeSchemaAction_NewNode(Category, MenuDesc, Tooltip, 0));

	ContextMenuBuilder.AddAction( NewAction );

	return NewAction;
}

TSharedPtr<FBehaviorTreeSchemaAction_NewSubNode> AddNewSubNodeAction(FGraphContextMenuBuilder& ContextMenuBuilder, const FString& Category, const FString& MenuDesc, const FString& Tooltip)
{
	TSharedPtr<FBehaviorTreeSchemaAction_NewSubNode> NewAction = TSharedPtr<FBehaviorTreeSchemaAction_NewSubNode>(new FBehaviorTreeSchemaAction_NewSubNode(Category, MenuDesc, Tooltip, 0));
	ContextMenuBuilder.AddAction( NewAction );
	return NewAction;
}

void UEdGraphSchema_BehaviorTree::CreateDefaultNodesForGraph(UEdGraph& Graph) const
{
	FGraphNodeCreator<UBehaviorTreeGraphNode_Root> NodeCreator(Graph);
	UBehaviorTreeGraphNode_Root* MyNode = NodeCreator.CreateNode();
	NodeCreator.Finalize();
}

FString UEdGraphSchema_BehaviorTree::GetShortTypeName(const UObject* Ob) const
{
	if (Ob->GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
	{
		return Ob->GetClass()->GetName().LeftChop(2);
	}

	FString TypeDesc = Ob->GetClass()->GetName();
	const int32 ShortNameIdx = TypeDesc.Find(TEXT("_"));
	if (ShortNameIdx != INDEX_NONE)
	{
		TypeDesc = TypeDesc.Mid(ShortNameIdx + 1);
	}

	return TypeDesc;
}

void UEdGraphSchema_BehaviorTree::GetGraphNodeContextActions(FGraphContextMenuBuilder& ContextMenuBuilder, ESubNode::Type SubNodeType) const
{
	TArray<FClassData> NodeClasses;
	UEdGraph* Graph = (UEdGraph*)ContextMenuBuilder.CurrentGraph;

	if (SubNodeType == ESubNode::Decorator)
	{
		FClassBrowseHelper::GatherClasses(UBTDecorator::StaticClass(), NodeClasses);

		{
			UBehaviorTreeGraphNode_CompositeDecorator* OpNode = NewObject<UBehaviorTreeGraphNode_CompositeDecorator>(Graph);
			TSharedPtr<FBehaviorTreeSchemaAction_NewSubNode> AddOpAction = AddNewSubNodeAction(ContextMenuBuilder, TEXT(""), OpNode->GetNodeTypeDescription(), "");
			AddOpAction->ParentNode = Cast<UBehaviorTreeGraphNode>(ContextMenuBuilder.SelectedObjects[0]);
			AddOpAction->NodeTemplate = OpNode;
		}

		for (int32 i = 0; i < NodeClasses.Num(); i++)
		{
			const FString NodeTypeName = EngineUtils::SanitizeDisplayName(NodeClasses[i].ToString(), false);

			UBehaviorTreeGraphNode* OpNode = NewObject<UBehaviorTreeGraphNode_Decorator>(Graph);
			OpNode->ClassData = NodeClasses[i];

			TSharedPtr<FBehaviorTreeSchemaAction_NewSubNode> AddOpAction = AddNewSubNodeAction(ContextMenuBuilder, TEXT(""), NodeTypeName, "");
			AddOpAction->ParentNode = Cast<UBehaviorTreeGraphNode>(ContextMenuBuilder.SelectedObjects[0]);
			AddOpAction->NodeTemplate = OpNode;
		}
	}
	else if (SubNodeType == ESubNode::Service)
	{
		FClassBrowseHelper::GatherClasses(UBTService::StaticClass(), NodeClasses);
		for (int32 i = 0; i < NodeClasses.Num(); i++)
		{
			const FString NodeTypeName = EngineUtils::SanitizeDisplayName(NodeClasses[i].ToString(), false);

			UBehaviorTreeGraphNode* OpNode = NewObject<UBehaviorTreeGraphNode_Service>(Graph);
			OpNode->ClassData = NodeClasses[i];

			TSharedPtr<FBehaviorTreeSchemaAction_NewSubNode> AddOpAction = AddNewSubNodeAction(ContextMenuBuilder, TEXT(""), NodeTypeName, "");
			AddOpAction->ParentNode = Cast<UBehaviorTreeGraphNode>(ContextMenuBuilder.SelectedObjects[0]);
			AddOpAction->NodeTemplate = OpNode;
		}
	}
}

void UEdGraphSchema_BehaviorTree::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	const FString PinCategory = ContextMenuBuilder.FromPin ?
		ContextMenuBuilder.FromPin->PinType.PinCategory : 
		UBehaviorTreeEditorTypes::PinCategory_MultipleNodes;

	const bool bNoParent = (ContextMenuBuilder.FromPin == NULL);
	const bool bOnlyTasks = (PinCategory == UBehaviorTreeEditorTypes::PinCategory_SingleTask);
	const bool bOnlyComposites = (PinCategory == UBehaviorTreeEditorTypes::PinCategory_SingleComposite);
	const bool bAllowComposites = bNoParent || !bOnlyTasks || bOnlyComposites;
	const bool bAllowTasks = bNoParent || !bOnlyComposites || bOnlyTasks;

	if (bAllowComposites)
	{
		TArray<FClassData> NodeClasses;
		FClassBrowseHelper::GatherClasses(UBTCompositeNode::StaticClass(), NodeClasses);

		const FString ParallelClassName = UBTComposite_SimpleParallel::StaticClass()->GetName();

		for (int32 i = 0; i < NodeClasses.Num(); i++)
	{
			const FString NodeTypeName = EngineUtils::SanitizeDisplayName(NodeClasses[i].ToString(), false);

			TSharedPtr<FBehaviorTreeSchemaAction_NewNode> AddOpAction = AddNewNodeAction(ContextMenuBuilder, TEXT("Composites"), NodeTypeName, "");

			UBehaviorTreeGraphNode* OpNode = (NodeClasses[i].GetClassName() == ParallelClassName) ?
				NewObject<UBehaviorTreeGraphNode_SimpleParallel>(ContextMenuBuilder.OwnerOfTemporaries) :				
				NewObject<UBehaviorTreeGraphNode_Composite>(ContextMenuBuilder.OwnerOfTemporaries);

			OpNode->ClassData = NodeClasses[i];
			AddOpAction->NodeTemplate = OpNode;
		}
		}

	if (bAllowTasks)
		{
		TArray<FClassData> NodeClasses;
		FClassBrowseHelper::GatherClasses(UBTTaskNode::StaticClass(), NodeClasses);

		for (int32 i = 0; i < NodeClasses.Num(); i++)
		{
			const FString NodeTypeName = EngineUtils::SanitizeDisplayName(NodeClasses[i].ToString(), false);

			TSharedPtr<FBehaviorTreeSchemaAction_NewNode> AddOpAction = AddNewNodeAction(ContextMenuBuilder, TEXT("Tasks"), NodeTypeName, "");

			UBehaviorTreeGraphNode* OpNode = NewObject<UBehaviorTreeGraphNode_Task>(ContextMenuBuilder.OwnerOfTemporaries);
			OpNode->ClassData = NodeClasses[i];
			AddOpAction->NodeTemplate = OpNode;
		}
	}
	
	if (bNoParent)
		{
		TSharedPtr<FBehaviorTreeSchemaAction_AutoArrange> Action = TSharedPtr<FBehaviorTreeSchemaAction_AutoArrange>(new FBehaviorTreeSchemaAction_AutoArrange(FString(),FString(TEXT("Auto Arrange")),FString(),0));
		ContextMenuBuilder.AddAction(Action);
	}
}

void UEdGraphSchema_BehaviorTree::GetContextMenuActions(const UEdGraph* CurrentGraph, const UEdGraphNode* InGraphNode, const UEdGraphPin* InGraphPin, class FMenuBuilder* MenuBuilder, bool bIsDebugging) const
{
	if (InGraphPin)
	{
		MenuBuilder->BeginSection("BehaviorTreeGraphSchemaPinActions", LOCTEXT("PinActionsMenuHeader", "Pin Actions"));
		{
			// Only display the 'Break Links' option if there is a link to break!
			if (InGraphPin->LinkedTo.Num() > 0)
			{
				MenuBuilder->AddMenuEntry( FGraphEditorCommands::Get().BreakPinLinks );

				// add sub menu for break link to
				if(InGraphPin->LinkedTo.Num() > 1)
				{
					MenuBuilder->AddSubMenu(
						LOCTEXT("BreakLinkTo", "Break Link To..." ),
						LOCTEXT("BreakSpecificLinks", "Break a specific link..." ),
						FNewMenuDelegate::CreateUObject( (UEdGraphSchema_BehaviorTree*const)this, &UEdGraphSchema_BehaviorTree::GetBreakLinkToSubMenuActions, const_cast<UEdGraphPin*>(InGraphPin)));
				}
				else
				{
					((UEdGraphSchema_BehaviorTree*const)this)->GetBreakLinkToSubMenuActions(*MenuBuilder, const_cast<UEdGraphPin*>(InGraphPin));
				}
			}
		}
		MenuBuilder->EndSection();
	}
	else if (InGraphNode)
	{
		const UBehaviorTreeGraphNode* BTGraphNode = Cast<const UBehaviorTreeGraphNode>(InGraphNode);
		if (BTGraphNode && BTGraphNode->CanPlaceBreakpoints())
		{
			MenuBuilder->BeginSection("EdGraphSchemaBreakpoints", LOCTEXT("BreakpointsHeader", "Breakpoints"));
			{
				MenuBuilder->AddMenuEntry( FGraphEditorCommands::Get().ToggleBreakpoint );
				MenuBuilder->AddMenuEntry( FGraphEditorCommands::Get().AddBreakpoint );
				MenuBuilder->AddMenuEntry( FGraphEditorCommands::Get().RemoveBreakpoint );
				MenuBuilder->AddMenuEntry( FGraphEditorCommands::Get().EnableBreakpoint );
				MenuBuilder->AddMenuEntry( FGraphEditorCommands::Get().DisableBreakpoint );
			}
			MenuBuilder->EndSection();
		}

		MenuBuilder->BeginSection("BehaviorTreeGraphSchemaNodeActions", LOCTEXT("ClassActionsMenuHeader", "Node Actions"));
		{
			MenuBuilder->AddMenuEntry( FGenericCommands::Get().Delete );
			MenuBuilder->AddMenuEntry( FGenericCommands::Get().Cut );
			MenuBuilder->AddMenuEntry( FGenericCommands::Get().Copy );
			MenuBuilder->AddMenuEntry( FGenericCommands::Get().Duplicate );

			MenuBuilder->AddMenuEntry(FGraphEditorCommands::Get().BreakNodeLinks);
		}
		MenuBuilder->EndSection();
	}

	Super::GetContextMenuActions(CurrentGraph, InGraphNode, InGraphPin, MenuBuilder, bIsDebugging);
}

void UEdGraphSchema_BehaviorTree::GetBreakLinkToSubMenuActions( class FMenuBuilder& MenuBuilder, UEdGraphPin* InGraphPin )
{
	// Make sure we have a unique name for every entry in the list
	TMap< FString, uint32 > LinkTitleCount;

	// Add all the links we could break from
	for(TArray<class UEdGraphPin*>::TConstIterator Links(InGraphPin->LinkedTo); Links; ++Links)
	{
		UEdGraphPin* Pin = *Links;
		FString TitleString = Pin->GetOwningNode()->GetNodeTitle(ENodeTitleType::ListView);
		FText Title = FText::FromString( TitleString );
		if ( Pin->PinName != TEXT("") )
		{
			TitleString = FString::Printf(TEXT("%s (%s)"), *TitleString, *Pin->PinName);

			// Add name of connection if possible
			FFormatNamedArguments Args;
			Args.Add( TEXT("NodeTitle"), Title );
			Args.Add( TEXT("PinName"), FText::FromString( Pin->PinName ) );
			Title = FText::Format( LOCTEXT("BreakDescPin", "{NodeTitle} ({PinName})"), Args );
		}

		uint32 &Count = LinkTitleCount.FindOrAdd( TitleString );

		FText Description;
		FFormatNamedArguments Args;
		Args.Add( TEXT("NodeTitle"), Title );
		Args.Add( TEXT("NumberOfNodes"), Count );

		if(Count == 0)
		{
			Description = FText::Format( LOCTEXT("BreakDesc", "Break link to {NodeTitle}"), Args );
		}
		else
		{
			Description = FText::Format( LOCTEXT("BreakDescMulti", "Break link to {NodeTitle} ({NumberOfNodes})"), Args );
		}
		++Count;

		MenuBuilder.AddMenuEntry( Description, Description, FSlateIcon(), FUIAction(
			FExecuteAction::CreateUObject((USoundClassGraphSchema*const)this, &USoundClassGraphSchema::BreakSinglePinLink, const_cast< UEdGraphPin* >(InGraphPin), *Links) ) );
	}
}


const FPinConnectionResponse UEdGraphSchema_BehaviorTree::CanCreateConnection(const UEdGraphPin* PinA, const UEdGraphPin* PinB) const
{
	// Make sure the pins are not on the same node
	if (PinA->GetOwningNode() == PinB->GetOwningNode())
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinErrorSameNode","Both are on the same node"));
	}

	const bool bPinAIsSingleComposite = (PinA->PinType.PinCategory == UBehaviorTreeEditorTypes::PinCategory_SingleComposite);
	const bool bPinAIsSingleTask = (PinA->PinType.PinCategory == UBehaviorTreeEditorTypes::PinCategory_SingleTask);
	const bool bPinAIsSingleNode = (PinA->PinType.PinCategory == UBehaviorTreeEditorTypes::PinCategory_SingleNode);

	const bool bPinBIsSingleComposite = (PinB->PinType.PinCategory == UBehaviorTreeEditorTypes::PinCategory_SingleComposite);
	const bool bPinBIsSingleTask = (PinB->PinType.PinCategory == UBehaviorTreeEditorTypes::PinCategory_SingleTask);
	const bool bPinBIsSingleNode = (PinB->PinType.PinCategory == UBehaviorTreeEditorTypes::PinCategory_SingleNode);

	const bool bPinAIsTask = PinA->GetOwningNode()->IsA(UBehaviorTreeGraphNode_Task::StaticClass());
	const bool bPinAIsComposite = PinA->GetOwningNode()->IsA(UBehaviorTreeGraphNode_Composite::StaticClass());
	
	const bool bPinBIsTask = PinB->GetOwningNode()->IsA(UBehaviorTreeGraphNode_Task::StaticClass());
	const bool bPinBIsComposite = PinB->GetOwningNode()->IsA(UBehaviorTreeGraphNode_Composite::StaticClass());

	if ((bPinAIsSingleComposite && !bPinBIsComposite) || (bPinBIsSingleComposite && !bPinAIsComposite))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinErrorOnlyComposite","Only composite nodes are allowed"));
	}

	if ((bPinAIsSingleTask && !bPinBIsTask) || (bPinBIsSingleTask && !bPinAIsTask))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinErrorOnlyTask","Only task nodes are allowed"));
	}

	if (((bPinAIsSingleNode || bPinAIsSingleTask || bPinAIsSingleComposite) && PinA->LinkedTo.Num() > 0) ||
		((bPinBIsSingleNode || bPinBIsSingleTask || bPinBIsSingleComposite) && PinB->LinkedTo.Num() > 0))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinErrorSingleNode","Can't connect multiple nodes"));
	}

	// Compare the directions
	bool bDirectionsOK = false;

	if ((PinA->Direction == EGPD_Input) && (PinB->Direction == EGPD_Output))
	{
		bDirectionsOK = true;
	}
	else if ((PinB->Direction == EGPD_Input) && (PinA->Direction == EGPD_Output))
	{
		bDirectionsOK = true;
	}

	if (bDirectionsOK)
	{
		if ( (PinA->Direction == EGPD_Input && PinA->LinkedTo.Num()>0) || (PinB->Direction == EGPD_Input && PinB->LinkedTo.Num()>0))
		{
			return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinErrorAlreadyConnected","Already connected with other"));
		}
	}
	else
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT(""));
	}

	return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, TEXT(""));
}

const FPinConnectionResponse UEdGraphSchema_BehaviorTree::CanMergeNodes(const UEdGraphNode* NodeA, const UEdGraphNode* NodeB) const
{
	// Make sure the nodes are not the same 
	if (NodeA == NodeB)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Both are the same node"));
	}

	const bool bNodeAIsDecorator = NodeA->IsA(UBehaviorTreeGraphNode_Decorator::StaticClass()) || NodeA->IsA(UBehaviorTreeGraphNode_CompositeDecorator::StaticClass());
	const bool bNodeAIsService = NodeA->IsA(UBehaviorTreeGraphNode_Service::StaticClass());
	const bool bNodeBIsComposite = NodeB->IsA(UBehaviorTreeGraphNode_Composite::StaticClass());
	const bool bNodeBIsTask = NodeB->IsA(UBehaviorTreeGraphNode_Task::StaticClass());
	const bool bNodeBIsDecorator = NodeB->IsA(UBehaviorTreeGraphNode_Decorator::StaticClass()) || NodeB->IsA(UBehaviorTreeGraphNode_CompositeDecorator::StaticClass());
	const bool bNodeBIsService = NodeB->IsA(UBehaviorTreeGraphNode_Service::StaticClass());

	if (FBehaviorTreeDebugger::IsPIENotSimulating())
	{
	if ((bNodeAIsDecorator && (bNodeBIsComposite || bNodeBIsTask || bNodeBIsDecorator))
		|| (bNodeAIsService && (bNodeBIsComposite || bNodeBIsService)))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, TEXT(""));
	}
	}

	return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT(""));
}

FLinearColor UEdGraphSchema_BehaviorTree::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
	return FColor::White;
}

bool UEdGraphSchema_BehaviorTree::ShouldHidePinDefaultValue(UEdGraphPin* Pin) const
{
	check(Pin != NULL);

	if (Pin->bDefaultValueIsIgnored)
	{
		return true;
	}

	return false;
}

class FConnectionDrawingPolicy* UEdGraphSchema_BehaviorTree::CreateConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, class FSlateWindowElementList& InDrawElements, class UEdGraph* InGraphObj) const
{
	return new FBehaviorTreeConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect, InDrawElements, InGraphObj);
}

int32 UEdGraphSchema_BehaviorTree::GetNodeSelectionCount(const UEdGraph* Graph) const
{
	if (Graph)
	{
		TSharedPtr<IBehaviorTreeEditor> BTEditor;
		if (UBehaviorTree* BTAsset = Cast<UBehaviorTree>(Graph->GetOuter()))
		{
			TSharedPtr< IToolkit > BTAssetEditor = FToolkitManager::Get().FindEditorForAsset(BTAsset);
			if (BTAssetEditor.IsValid())
			{
				BTEditor = StaticCastSharedPtr<IBehaviorTreeEditor>(BTAssetEditor);
			}
		}
		if(BTEditor.IsValid())
		{
			return BTEditor->GetSelectedNodesCount();
		}
	}

	return 0;
}

#undef LOCTEXT_NAMESPACE
