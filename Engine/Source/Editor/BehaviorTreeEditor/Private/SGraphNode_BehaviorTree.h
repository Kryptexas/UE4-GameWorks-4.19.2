// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class SGraphNode_BehaviorTree : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SGraphNode_BehaviorTree){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UBehaviorTreeGraphNode* InNode);

	// SGraphNode interface
	virtual void UpdateGraphNode() OVERRIDE;
	virtual void CreatePinWidgets() OVERRIDE;
	virtual void AddPin(const TSharedRef<SGraphPin>& PinToAdd) OVERRIDE;
	virtual TSharedPtr<SToolTip> GetComplexTooltip() OVERRIDE;
	virtual void OnDragEnter( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent ) OVERRIDE;
	virtual FReply OnDragOver( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent ) OVERRIDE;
	virtual FReply OnDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent ) OVERRIDE;
	virtual void OnDragLeave( const FDragDropEvent& DragDropEvent ) OVERRIDE;
	virtual void GetOverlayBrushes(bool bSelected, const FVector2D WidgetSize, TArray<FOverlayBrushInfo>& Brushes) const OVERRIDE;
	// End of SGraphNode interface

	/** handle double click */
	virtual FReply OnMouseButtonDoubleClick( const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent ) OVERRIDE;

	/** handle mouse down on the node */
	FReply OnMouseDown(const FGeometry& SenderGeometry, const FPointerEvent& MouseEvent);

	/** handle mouse up on the node */
	FReply OnMouseUp(const FGeometry& SenderGeometry, const FPointerEvent& MouseEvent);

	/** handle mouse move on the node - activate drag and drop when mouse button is being held */
	virtual FReply OnMouseMove(const FGeometry& SenderGeometry, const FPointerEvent& MouseEvent) OVERRIDE;

	/**
	 * Ticks this widget.  Override in derived classes, but always call the parent implementation.
	 *
	 * @param  AllottedGeometry The space allotted for this widget
	 * @param  InCurrentTime  Current absolute real time
	 * @param  InDeltaTime  Real time passed since last tick
	 */
	void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) OVERRIDE;
	
	/** gets decorator or service node if one is found under mouse cursor */
	TSharedPtr<SGraphNode> GetSubNodeUnderCursor(const FGeometry& WidgetGeometry, const FPointerEvent& MouseEvent);

	/** adds decorator widget inside current node */
	void AddDecorator(TSharedPtr<SGraphNode> DecoratorWidget);

	/** adds service widget inside current node */
	void AddService(TSharedPtr<SGraphNode> ServiceWidget);

	/** gets drag over marker visibility */
	EVisibility GetDragOverMarkerVisibility() const;

	/** shows red marker when search failed*/
	EVisibility GetDebuggerSearchFailedMarkerVisibility() const;

	/** sets drag marker visible or collapsed on this node */
	void SetDragMarker(bool bEnabled);

	FVector2D GetCachedPosition() const { return CachedPosition; }

protected:
	float MouseDownTime;

	uint32 bDragMarkerVisible : 1;
	uint32 bIsMouseDown : 1;

	uint32 bSuppressDebuggerColor : 1;
	uint32 bSuppressDebuggerTriggers : 1;
	
	/** time spent in current state */
	float DebuggerStateDuration;

	/** currently displayed state */
	int32 DebuggerStateCounter;

	/** debugger colors */
	FLinearColor FlashColor;
	float FlashAlpha;

	/** height offsets for search triggers */
	TArray<FNodeBounds> TriggerOffsets;

	/** cached draw position */
	FVector2D CachedPosition;

	TArray< TSharedPtr<SGraphNode> > DecoratorWidgets;
	TArray< TSharedPtr<SGraphNode> > ServicesWidgets;
	TSharedPtr<SVerticalBox> DecoratorsBox;
	TSharedPtr<SVerticalBox> ServicesBox;
	TSharedPtr<SHorizontalBox> OutputPinBox;
	FSlateColor GetBorderBackgroundColor() const;
	FSlateColor GetBackgroundColor() const;
	FString	GetDescription() const;

	virtual FString GetPreviewCornerText() const;
	virtual const FSlateBrush* GetNameIcon() const;

	FString GetPinTooltip(UEdGraphPin* GraphPinObj) const;
};
