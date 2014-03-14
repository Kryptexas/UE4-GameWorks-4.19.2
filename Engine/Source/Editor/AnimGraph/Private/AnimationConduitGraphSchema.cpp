// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"

/////////////////////////////////////////////////////
// UAnimationConduitGraphSchema

UAnimationConduitGraphSchema::UAnimationConduitGraphSchema(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UAnimationConduitGraphSchema::CreateDefaultNodesForGraph(UEdGraph& Graph) const
{
	FGraphNodeCreator<UAnimGraphNode_TransitionResult> NodeCreator(Graph);
	UAnimGraphNode_TransitionResult* ResultSinkNode = NodeCreator.CreateNode();

	UAnimationTransitionGraph* TypedGraph = CastChecked<UAnimationTransitionGraph>(&Graph);
	TypedGraph->MyResultNode = ResultSinkNode;

	NodeCreator.Finalize();
}

void UAnimationConduitGraphSchema::GetGraphDisplayInformation(const UEdGraph& Graph, /*out*/ FGraphDisplayInfo& DisplayInfo) const
{
	DisplayInfo.DisplayName = FText::FromString( Graph.GetName() );
	
	if (const UAnimStateConduitNode* ConduitNode = Cast<const UAnimStateConduitNode>(Graph.GetOuter()))
	{
		DisplayInfo.DisplayName = FText::Format( NSLOCTEXT("Animation", "ConduitRuleGraphTitle", "{0} (conduit rule)"), FText::FromString( ConduitNode->GetNodeTitle(ENodeTitleType::FullTitle) ) );
	}
}
