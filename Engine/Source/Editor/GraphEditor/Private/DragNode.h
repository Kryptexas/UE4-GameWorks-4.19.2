// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class GRAPHEDITOR_API FDragNode : public FGraphEditorDragDropAction
{
public:
	// GetTypeId 
	static FString GetTypeId() { static FString Type = TEXT("FDragNode"); return Type; }

	static TSharedRef<FDragNode> New(const TSharedRef<SGraphPanel>& InGraphPanel, const TSharedRef<SGraphNode>& InDraggedNode);

	static TSharedRef<FDragNode> New(const TSharedRef<SGraphPanel>& InGraphPanel, const TArray< TSharedRef<SGraphNode> >& InDraggedNodes);
	
	// FGraphEditorDragDropAction interface
	virtual void HoverTargetChanged() OVERRIDE;
	virtual FReply DroppedOnNode(FVector2D ScreenPosition, FVector2D GraphPosition) OVERRIDE;
	virtual FReply DroppedOnPanel( const TSharedRef< SWidget >& Panel, FVector2D ScreenPosition, FVector2D GraphPosition, UEdGraph& Graph) OVERRIDE;
	//virtual void OnDragBegin( const TSharedRef<class SGraphPin>& InPin) OVERRIDE;
	virtual void OnDragged (const class FDragDropEvent& DragDropEvent ) OVERRIDE;
	// End of FGraphEditorDragDropAction interface
	
	const TArray< TSharedRef<SGraphNode> > & GetNodes() const;

	virtual bool IsValidOperation() const;

protected:
	typedef FGraphEditorDragDropAction Super;

	UEdGraphNode* GetGraphNodeForSGraphNode(TSharedRef<SGraphNode>& SNode);

protected:

	/** graph panel */
	TSharedPtr<SGraphPanel> GraphPanel;

	/** our dragged nodes*/
	TArray< TSharedRef<SGraphNode> > DraggedNodes;

	/** Offset information for the decorator widget */
	FVector2D	DecoratorAdjust;

	/** if we can drop our node here*/
	bool bValidOperation;
};
