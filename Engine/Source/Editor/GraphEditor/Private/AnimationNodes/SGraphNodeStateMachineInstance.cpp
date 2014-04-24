// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "GraphEditorCommon.h"
#include "../KismetNodes/SGraphNodeK2Base.h"
#include "../KismetNodes/SGraphNodeK2Composite.h"
#include "SGraphNodeStateMachineInstance.h"
#include "SGraphPreviewer.h"

/////////////////////////////////////////////////////
// SGraphNodeStateMachineInstance

void SGraphNodeStateMachineInstance::Construct(const FArguments& InArgs, UAnimGraphNode_StateMachineBase* InNode)
{
	GraphNode = InNode;

	SetCursor(EMouseCursor::CardinalCross);

	UpdateGraphNode();
}

UEdGraph* SGraphNodeStateMachineInstance::GetInnerGraph() const
{
	UAnimGraphNode_StateMachineBase* StateMachineInstance = CastChecked<UAnimGraphNode_StateMachineBase>(GraphNode);

	return StateMachineInstance->EditorStateMachineGraph;
}
