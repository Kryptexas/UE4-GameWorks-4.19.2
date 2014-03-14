// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

// Base class for drag-drop actions that pass into the graph editor and perform an action when dropped
class GRAPHEDITOR_API FGraphEditorDragDropAction : public FDragDropOperation
{
public:
	static FString GetTypeId() { static FString Type = TEXT("FGraphEditorDragDropAction"); return Type; }

	void SetHoveredPin(const TSharedPtr<class SGraphPin>& InPin);
	void SetHoveredNode(const TSharedPtr<class SGraphNode>& InNode);
	void SetHoveredGraph(const TSharedPtr<class SGraphPanel>& InGraph);
	void SetHoveredCategoryName(const FString& InHoverCategoryName);
	void SetHoveredAction(TSharedPtr<struct FEdGraphSchemaAction> Action);

	// Interface to override
	virtual void HoverTargetChanged() {}
	virtual FReply DroppedOnPin(FVector2D ScreenPosition, FVector2D GraphPosition) { return FReply::Unhandled(); }
	virtual FReply DroppedOnNode(FVector2D ScreenPosition, FVector2D GraphPosition) { return FReply::Unhandled(); }
	virtual FReply DroppedOnPanel( const TSharedRef< class SWidget >& Panel, FVector2D ScreenPosition, FVector2D GraphPosition, UEdGraph& Graph) { return FReply::Unhandled(); }
	virtual FReply DroppedOnAction(TSharedRef<struct FEdGraphSchemaAction> Action) { return FReply::Unhandled(); }
	virtual FReply DroppedOnCategory(FString Category) { return FReply::Unhandled(); }
	virtual void OnDragBegin(const TSharedRef<class SGraphPin>& InPin) {}
	// End of interface to override
	
protected:
	void SetFeedbackMessage(const TSharedPtr<SWidget>& Message);
	void SetSimpleFeedbackMessage(const FSlateBrush* Icon, const FSlateColor& IconColor, const FString& Message);

	UEdGraphPin* GetHoveredPin() const;
	UEdGraphNode* GetHoveredNode() const;
	UEdGraph* GetHoveredGraph() const;

	/** Constructs the window and widget if applicable */
	virtual void Construct();

private:
	// The pin that the drag action is currently hovering over
	TSharedPtr<class SGraphPin> HoveredPin;

	// The node that the drag action is currently hovering over
	TSharedPtr<class SGraphNode> HoveredNode;

	// The graph that the drag action is currently hovering over
	TSharedPtr<class SGraphPanel> HoveredGraph;

protected:

	// Name of category we are hovering over
	FString HoveredCategoryName;

	// Action we are hovering over
	TWeakPtr<struct FEdGraphSchemaAction> HoveredAction;
};

// Drag-drop action where an FEdGraphSchemaAction should be performed when dropped
class GRAPHEDITOR_API FGraphSchemaActionDragDropAction : public FGraphEditorDragDropAction
{
public:
	// GetTypeId is the parent: FGraphEditorDragDropAction

	// FGraphEditorDragDropAction interface
	virtual void HoverTargetChanged() OVERRIDE;
	virtual FReply DroppedOnPanel( const TSharedRef< class SWidget >& Panel, FVector2D ScreenPosition, FVector2D GraphPosition, UEdGraph& Graph) OVERRIDE;
	// End of FGraphEditorDragDropAction

	static TSharedRef<FGraphSchemaActionDragDropAction> New(TSharedPtr<FEdGraphSchemaAction> InActionNode )
	{
		TSharedRef<FGraphSchemaActionDragDropAction> Operation = MakeShareable(new FGraphSchemaActionDragDropAction);
		FSlateApplication::GetDragDropReflector().RegisterOperation<FGraphSchemaActionDragDropAction>(Operation);
		Operation->ActionNode = InActionNode;
		Operation->Construct();
		return Operation;
	}

protected:
	/** */
	TSharedPtr<FEdGraphSchemaAction> ActionNode;
};
