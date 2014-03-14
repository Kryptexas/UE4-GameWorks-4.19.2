// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "EdGraphSchema_BehaviorTree.generated.h"

/** Action to add a node to the graph */
USTRUCT()
struct FBehaviorTreeSchemaAction_NewNode : public FEdGraphSchemaAction
{
	GENERATED_USTRUCT_BODY();

	/** Template of node we want to create */
	UPROPERTY()
	class UBehaviorTreeGraphNode* NodeTemplate;


	FBehaviorTreeSchemaAction_NewNode() 
		: FEdGraphSchemaAction()
		, NodeTemplate(NULL)
	{}

	FBehaviorTreeSchemaAction_NewNode(const FString& InNodeCategory, const FString& InMenuDesc, const FString& InToolTip, const int32 InGrouping)
		: FEdGraphSchemaAction(InNodeCategory, InMenuDesc, InToolTip, InGrouping) 
		, NodeTemplate(NULL)
	{}

	// FEdGraphSchemaAction interface
	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) OVERRIDE;
	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, TArray<UEdGraphPin*>& FromPins, const FVector2D Location, bool bSelectNewNode = true) OVERRIDE;
	virtual void AddReferencedObjects( FReferenceCollector& Collector ) OVERRIDE;
	// End of FEdGraphSchemaAction interface

	template <typename NodeType>
	static NodeType* SpawnNodeFromTemplate(class UEdGraph* ParentGraph, NodeType* InTemplateNode, const FVector2D Location)
	{
		FBehaviorTreeSchemaAction_NewNode Action;
		Action.NodeTemplate = InTemplateNode;

		return Cast<NodeType>(Action.PerformAction(ParentGraph, NULL, Location));
	}
};

/** Action to add a subnode to the selected node */
USTRUCT()
struct FBehaviorTreeSchemaAction_NewSubNode : public FEdGraphSchemaAction
{
	GENERATED_USTRUCT_BODY();

	/** Template of node we want to create */
	UPROPERTY()
	class UBehaviorTreeGraphNode* NodeTemplate;

	/** parent node */
	UPROPERTY()
	class UBehaviorTreeGraphNode* ParentNode;

	FBehaviorTreeSchemaAction_NewSubNode() 
		: FEdGraphSchemaAction()
		, NodeTemplate(NULL)
	{}

	FBehaviorTreeSchemaAction_NewSubNode(const FString& InNodeCategory, const FString& InMenuDesc, const FString& InToolTip, const int32 InGrouping)
		: FEdGraphSchemaAction(InNodeCategory, InMenuDesc, InToolTip, InGrouping) 
		, NodeTemplate(NULL)
	{}

	// FEdGraphSchemaAction interface
	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) OVERRIDE;
	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, TArray<UEdGraphPin*>& FromPins, const FVector2D Location, bool bSelectNewNode = true) OVERRIDE;
	virtual void AddReferencedObjects( FReferenceCollector& Collector ) OVERRIDE;
	// End of FEdGraphSchemaAction interface
};

/** Action to auto arrange the graph */
USTRUCT()
struct FBehaviorTreeSchemaAction_AutoArrange : public FEdGraphSchemaAction
{
	GENERATED_USTRUCT_BODY();

	FBehaviorTreeSchemaAction_AutoArrange() 
		: FEdGraphSchemaAction() {}

	FBehaviorTreeSchemaAction_AutoArrange(const FString& InNodeCategory, const FString& InMenuDesc, const FString& InToolTip, const int32 InGrouping)
		: FEdGraphSchemaAction(InNodeCategory, InMenuDesc, InToolTip, InGrouping) 
	{}

	// FEdGraphSchemaAction interface
	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) OVERRIDE;
	// End of FEdGraphSchemaAction interface
};

UCLASS(MinimalAPI)
class UEdGraphSchema_BehaviorTree : public UEdGraphSchema
{
	GENERATED_UCLASS_BODY()

	void GetBreakLinkToSubMenuActions(class FMenuBuilder& MenuBuilder, class UEdGraphPin* InGraphPin);

	// Begin EdGraphSchema interface
	virtual void CreateDefaultNodesForGraph(UEdGraph& Graph) const OVERRIDE;
	virtual void GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const OVERRIDE;
	virtual void GetContextMenuActions(const UEdGraph* CurrentGraph, const UEdGraphNode* InGraphNode, const UEdGraphPin* InGraphPin, class FMenuBuilder* MenuBuilder, bool bIsDebugging) const OVERRIDE;
	virtual const FPinConnectionResponse CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const OVERRIDE;
	virtual const FPinConnectionResponse CanMergeNodes(const UEdGraphNode* A, const UEdGraphNode* B) const OVERRIDE;
	virtual FLinearColor GetPinTypeColor(const FEdGraphPinType& PinType) const OVERRIDE;
	virtual bool ShouldHidePinDefaultValue(UEdGraphPin* Pin) const OVERRIDE;
	virtual class FConnectionDrawingPolicy* CreateConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, class FSlateWindowElementList& InDrawElements, class UEdGraph* InGraphObj) const OVERRIDE;
	virtual int32 GetNodeSelectionCount(const UEdGraph* Graph) const OVERRIDE;
	// End EdGraphSchema interface

	/* subnode creation context menu */
	void GetGraphNodeContextActions(FGraphContextMenuBuilder& ContextMenuBuilder, ESubNode::Type SubNodeType) const;

protected:
	FString GetShortTypeName(const UObject* Ob) const;
};

