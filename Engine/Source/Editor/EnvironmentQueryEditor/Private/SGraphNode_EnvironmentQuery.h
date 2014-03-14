// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class SGraphNode_EnvironmentQuery : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SGraphNode_EnvironmentQuery){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEnvironmentQueryGraphNode* InNode);

	// SGraphNode interface
	virtual void UpdateGraphNode() OVERRIDE;
	virtual void CreatePinWidgets() OVERRIDE;
	virtual void AddPin(const TSharedRef<SGraphPin>& PinToAdd) OVERRIDE;
	virtual TSharedPtr<SToolTip> GetComplexTooltip() OVERRIDE;
	virtual void OnDragEnter( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent ) OVERRIDE;
	virtual FReply OnDragOver( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent ) OVERRIDE;
	virtual FReply OnDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent ) OVERRIDE;
	virtual void OnDragLeave( const FDragDropEvent& DragDropEvent ) OVERRIDE;
	virtual FReply OnMouseMove(const FGeometry& SenderGeometry, const FPointerEvent& MouseEvent) OVERRIDE;
	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) OVERRIDE;
	// End of SGraphNode interface

	/** handle mouse down on the node */
	FReply OnMouseDown(const FGeometry& SenderGeometry, const FPointerEvent& MouseEvent);

	/** handle mouse up on the node */
	FReply OnMouseUp(const FGeometry& SenderGeometry, const FPointerEvent& MouseEvent);

	/** adds decorator widget inside current node */
	void AddTest(TSharedPtr<SGraphNode> TestWidget);

	/** gets decorator or service node if one is found under mouse cursor */
	TSharedPtr<SGraphNode> GetSubNodeUnderCursor(const FGeometry& WidgetGeometry, const FPointerEvent& MouseEvent);

	EVisibility GetWeightMarkerVisibility() const;
	TOptional<float> GetWeightProgressBarPercent() const;
	FSlateColor GetWeightProgressBarColor() const; 

	EVisibility GetTestToggleVisibility() const;
	ESlateCheckBoxState::Type IsTestToggleChecked() const;
	void OnTestToggleChanged(ESlateCheckBoxState::Type NewState);

	/** gets drag over marker visibility */
	EVisibility GetDragOverMarkerVisibility() const;

	/** sets drag marker visible or collapsed on this node */
	void SetDragMarker(bool bEnabled);

protected:
	TArray< TSharedPtr<SGraphNode> > TestWidgets;
	TSharedPtr<SVerticalBox> TestBox;

	float MouseDownTime;

	uint32 bDragMarkerVisible : 1;
	uint32 bIsMouseDown : 1;

	FSlateColor GetBorderBackgroundColor() const;
	FSlateColor GetBackgroundColor() const;
	FString GetDescription() const;

	virtual FString GetPreviewCornerText() const;
	virtual const FSlateBrush* GetNameIcon() const;
};
