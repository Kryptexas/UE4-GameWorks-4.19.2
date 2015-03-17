// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SGraphNodeDefault.h"
#include "K2Node_Knot.h"

class SCommentBubble;

class SGraphNodeKnot : public SGraphNodeDefault
{
public:
	SLATE_BEGIN_ARGS(SGraphNodeKnot) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, class UK2Node_Knot* InKnot);

	// SGraphNode interface
	virtual void UpdateGraphNode() override;
	virtual const FSlateBrush* GetShadowBrush(bool bSelected) const override;
	virtual TSharedPtr<SGraphPin> CreatePinWidget(UEdGraphPin* Pin) const override;
	virtual void AddPin(const TSharedRef<SGraphPin>& PinToAdd) override;
	virtual void RequestRenameOnSpawn() override { }
	// End of SGraphNode interface

	// SWidget interface
	virtual void OnMouseEnter( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;
	virtual void OnMouseLeave( const FPointerEvent& MouseEvent ) override;
	// End of SWidget interface

	/** Returns Offset to center comment on the node's only pin */
	FVector2D GetCommentOffset() const;

	/** Called when the comment bubble visibility/pinned state is changed */
	void OnCommentBubbleToggle( ECheckBoxState State );

protected:

	/** The pinned state for comment bubble */
	bool bCommentPinnedState;
	/** SharedPtr to comment bubble */
	TSharedPtr<SCommentBubble> CommentBubble;

};
