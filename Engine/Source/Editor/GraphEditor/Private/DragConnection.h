// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FDragConnection : public FGraphEditorDragDropAction
{
public:
	// GetTypeId is the parent: FGraphEditorDragDropAction

	static TSharedRef<FDragConnection> New(const TSharedRef<SGraphPanel>& InGraphPanel, const TSharedRef<SGraphPin>& InStartingPin);
	static TSharedRef<FDragConnection> New(const TSharedRef<SGraphPanel>& InGraphPanel, const TArray< TSharedRef<SGraphPin> >& InStartingPins, bool bIsMovingLinks);
	
	// FDragDropOperation interface
	virtual void OnDrop( bool bDropWasHandled, const FPointerEvent& MouseEvent ) OVERRIDE;
	// End of FDragDropOperation interface

	// FGraphEditorDragDropAction interface
	virtual void HoverTargetChanged() OVERRIDE;
	virtual FReply DroppedOnPin(FVector2D ScreenPosition, FVector2D GraphPosition) OVERRIDE;
	virtual FReply DroppedOnPanel( const TSharedRef< SWidget >& Panel, FVector2D ScreenPosition, FVector2D GraphPosition, UEdGraph& Graph) OVERRIDE;
	virtual void OnDragBegin( const TSharedRef<class SGraphPin>& InPin) OVERRIDE;
	virtual void OnDragged (const class FDragDropEvent& DragDropEvent ) OVERRIDE;
	// End of FGraphEditorDragDropAction interface
	
protected:
	typedef FGraphEditorDragDropAction Super;

	UEdGraphPin* GetGraphPinForSPin(TSharedRef<SGraphPin>& SPin);

	/*
	 *	Function to check validity of graph pins in the StartPins list. This check helps to prevent processing graph pins which are outdated.
	 */
	void ValidateGraphPinList( );
	
	void Init( const TSharedRef<SGraphPanel>& InGraphPanel, const TArray< TSharedRef<SGraphPin> >& InStartingPins );

protected:
	TSharedPtr<SGraphPanel> GraphPanel;
	TArray< TSharedRef<SGraphPin> > StartingPins;
	/** Offset information for the decorator widget */
	FVector2D	DecoratorAdjust;
};
