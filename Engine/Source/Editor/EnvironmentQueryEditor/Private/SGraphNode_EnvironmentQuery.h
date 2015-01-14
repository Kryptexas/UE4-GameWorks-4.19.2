// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class UEnvironmentQueryGraphNode_Test;

class FDragNodeEQSTimed : public FDragNode
{
public:
	DRAG_DROP_OPERATOR_TYPE(FDragNodeEQSTimed, FDragNode)

	static TSharedRef<FDragNodeEQSTimed> New(const TSharedRef<SGraphPanel>& InGraphPanel, const TSharedRef<SGraphNode>& InDraggedNode);
	static TSharedRef<FDragNodeEQSTimed> New(const TSharedRef<SGraphPanel>& InGraphPanel, const TArray< TSharedRef<SGraphNode> >& InDraggedNodes);

	UEnvironmentQueryGraphNode_Test* GetDropTargetNode() const;

	double StartTime;

protected:
	typedef FDragNode Super;
};

class SGraphNode_EnvironmentQuery : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SGraphNode_EnvironmentQuery){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEnvironmentQueryGraphNode* InNode);

	// SGraphNode interface
	virtual void UpdateGraphNode() override;
	virtual void CreatePinWidgets() override;
	virtual void AddPin(const TSharedRef<SGraphPin>& PinToAdd) override;
	virtual TSharedPtr<SToolTip> GetComplexTooltip() override;
	virtual void OnDragEnter( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent ) override;
	virtual FReply OnDragOver( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent ) override;
	virtual FReply OnDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent ) override;
	virtual void OnDragLeave( const FDragDropEvent& DragDropEvent ) override;
	virtual FReply OnMouseMove(const FGeometry& SenderGeometry, const FPointerEvent& MouseEvent) override;
	virtual void SetOwner(const TSharedRef<SGraphPanel>& OwnerPanel) override;

	// End of SGraphNode interface

	/** handle mouse down on the node */
	FReply OnMouseDown(const FGeometry& SenderGeometry, const FPointerEvent& MouseEvent);

	/** adds decorator widget inside current node */
	void AddTest(TSharedPtr<SGraphNode> TestWidget);

	/** gets decorator or service node if one is found under mouse cursor */
	TSharedPtr<SGraphNode> GetSubNodeUnderCursor(const FGeometry& WidgetGeometry, const FPointerEvent& MouseEvent);

	EVisibility GetWeightMarkerVisibility() const;
	TOptional<float> GetWeightProgressBarPercent() const;
	FSlateColor GetWeightProgressBarColor() const; 

	EVisibility GetTestToggleVisibility() const;
	ECheckBoxState IsTestToggleChecked() const;
	void OnTestToggleChanged(ECheckBoxState NewState);

	/** gets drag over marker visibility */
	EVisibility GetDragOverMarkerVisibility() const;

	/** sets drag marker visible or collapsed on this node */
	void SetDragMarker(bool bEnabled);

protected:
	TArray< TSharedPtr<SGraphNode> > TestWidgets;
	TSharedPtr<SVerticalBox> TestBox;

	uint32 bDragMarkerVisible : 1;

	FSlateColor GetBorderBackgroundColor() const;
	FSlateColor GetBackgroundColor() const;
	FText GetDescription() const;

	virtual FText GetPreviewCornerText() const;
	virtual const FSlateBrush* GetNameIcon() const;
};
