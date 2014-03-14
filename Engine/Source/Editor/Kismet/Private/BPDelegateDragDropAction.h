// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BlueprintEditor.h"
#include "BPVariableDragDropAction.h"

/** DragDropAction class for dropping a Variable onto a graph */
class FKismetDelegateDragDropAction : public FKismetVariableDragDropAction
{
public:
	// GetTypeId is the parent: FGraphEditorDragDropAction

	// FGraphEditorDragDropAction interface
	virtual void HoverTargetChanged() OVERRIDE { FGraphEditorDragDropAction::HoverTargetChanged(); }
	virtual FReply DroppedOnPin(FVector2D ScreenPosition, FVector2D GraphPosition) OVERRIDE { return FGraphEditorDragDropAction::DroppedOnPin(ScreenPosition, GraphPosition); }
	virtual FReply DroppedOnNode(FVector2D ScreenPosition, FVector2D GraphPosition) OVERRIDE { return FGraphEditorDragDropAction::DroppedOnNode(ScreenPosition, GraphPosition); }

	virtual FReply DroppedOnPanel(const TSharedRef< SWidget >& Panel, FVector2D ScreenPosition, FVector2D GraphPosition, UEdGraph& Graph) OVERRIDE;
	// End of FGraphEditorDragDropAction

	bool IsValid() const;
	
	static TSharedRef<FKismetDelegateDragDropAction> New( TSharedRef< SWidget > Owner, FName InVariableName, UClass* InClass, FNodeCreationAnalytic AnalyticCallback)
	{
		TSharedRef<FKismetDelegateDragDropAction> Operation = MakeShareable(new FKismetDelegateDragDropAction( Owner ) );
		FSlateApplication::GetDragDropReflector().RegisterOperation<FKismetDelegateDragDropAction>(Operation);
		Operation->VariableName = InVariableName;
		Operation->VariableSourceClass = InClass;
		Operation->AnalyticCallback = AnalyticCallback;
		Operation->Construct();
		return Operation;
	}

	/** Structure for required node construction parameters */
	struct FNodeConstructionParams
	{
		FVector2D GraphPosition;
		UEdGraph* Graph;
		bool bSelfContext;
		const UProperty* Property;
		FNodeCreationAnalytic AnalyticCallback;
	};

	template<class TNode> static void MakeMCDelegateNode(FNodeConstructionParams Params)
	{
		check(Params.Graph && Params.Property);
		TNode* Node = NewObject<TNode>();
		Node->SetFromProperty(Params.Property, Params.bSelfContext);
		FEdGraphSchemaAction_K2NewNode::SpawnNodeFromTemplate<TNode>(Params.Graph, Node, Params.GraphPosition);
		Params.AnalyticCallback.ExecuteIfBound();
	}

	/** Create new custom event node from construction parameters */
	static void MakeEvent(FNodeConstructionParams Params);

	/** Assign new delegate node from construction parameters */
	static void AssignEvent(FNodeConstructionParams Params);

protected:
	FKismetDelegateDragDropAction( const TSharedRef< SWidget >& InOwner );

private:

	TSharedRef< SWidget > Owner;
};
