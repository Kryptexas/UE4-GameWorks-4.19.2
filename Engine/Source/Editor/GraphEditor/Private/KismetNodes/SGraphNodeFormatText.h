// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class SGraphNodeFormatText : public SGraphNodeK2Base
{
public:
	SLATE_BEGIN_ARGS(SGraphNodeFormatText){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UK2Node_FormatText* InNode);

	// SGraphNode interface
	virtual void CreatePinWidgets() OVERRIDE;
	// End of SGraphNode interface

	/**
	 * Update this GraphNode to match the data that it is observing
	 */
	virtual void UpdateGraphNode();

private:

	/** Returns the visibility of the "AddPin" button */
	EVisibility IsAddPinButtonVisible() const;

	/** Callback when adding a pin */
	FReply OnAddPin();
};
