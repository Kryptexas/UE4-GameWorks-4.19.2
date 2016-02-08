// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once


class SEditableLabel;
class SSequencerTreeView;
class SSequencerTreeViewRow;


/**
 * A widget for displaying a sequencer tree node in the animation outliner.
 */
class SAnimationOutlinerTreeNode
	: public SCompoundWidget
{
public:

	SAnimationOutlinerTreeNode(){}
	~SAnimationOutlinerTreeNode();
	
	SLATE_BEGIN_ARGS(SAnimationOutlinerTreeNode){}
		SLATE_ATTRIBUTE(const FSlateBrush*, IconBrush)
		SLATE_ATTRIBUTE(const FSlateBrush*, IconOverlayBrush)
		SLATE_ATTRIBUTE(FText, IconToolTipText)
		SLATE_NAMED_SLOT(FArguments, CustomContent)
	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs, TSharedRef<FSequencerDisplayNode> Node, const TSharedRef<SSequencerTreeViewRow>& InTableRow );

public:

	/** Change the node's label text to edit mode. */
	void EnterRenameMode();

	/**
	 * @return The display node used by this widget.                                                           
	 */
	const TSharedPtr<FSequencerDisplayNode> GetDisplayNode() const
	{
		return DisplayNode;
	}

private:

	// SWidget interface

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	void OnMouseLeave(const FPointerEvent& MouseEvent) override;

	FSlateColor GetForegroundBasedOnSelection() const;

	/**
	 * @return The border image to show in the tree node.
	 */
	const FSlateBrush* GetNodeBorderImage() const;
	
	/**
	 * @return The tint to apply to the border image
	 */
	FSlateColor GetNodeBackgroundTint() const;

	/**
	 * @return The expander visibility of this node.
	 */
	EVisibility GetExpanderVisibility() const;

	/**
	 * @return The display name for this node.
	 */
	FText GetDisplayName() const;

	/** Callback for checking whether the node label can be edited. */
	bool HandleNodeLabelCanEdit() const;

	/** Callback for when the node label text has changed. */
	void HandleNodeLabelTextChanged(const FText& NewLabel);

	/** Get all descendant nodes from the given root node. */
	void GetAllDescendantNodes(TSharedPtr<FSequencerDisplayNode> RootNode, TArray<TSharedRef<FSequencerDisplayNode> >& AllNodes);

	/** Return the root node given an object node. */
	TSharedPtr<FSequencerDisplayNode> GetRootNode(TSharedPtr<FSequencerDisplayNode> ObjectNode);

	/** Called when nodes are selected. */
	void OnSelectionChanged(TArray<TSharedPtr<FSequencerDisplayNode>> AffectedNodes);

private:

	/** Layout node the widget is visualizing. */
	TSharedPtr<FSequencerDisplayNode> DisplayNode;

	/** Holds the editable text label widget. */
	TSharedPtr<SEditableLabel> EditableLabel;

	/** True if this node is a top level node, false otherwise */
	bool bIsTopLevelNode;

	/** Default background brush for this node when expanded */
	const FSlateBrush* ExpandedBackgroundBrush;

	/** Default background brush for this node when collapsed */
	const FSlateBrush* CollapsedBackgroundBrush;

	/** The table row style used for nodes in the tree. This is required as we don't actually use the tree for selection. */
	const FTableRowStyle* TableRowStyle;
};