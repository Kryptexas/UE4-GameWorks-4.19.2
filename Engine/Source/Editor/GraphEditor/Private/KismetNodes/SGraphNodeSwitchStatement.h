// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class SGraphNodeSwitchStatement : public SGraphNodeK2Base
{
public:
	SLATE_BEGIN_ARGS(SGraphNodeSwitchStatement){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UK2Node_Switch* InNode);

	// SGraphNode interface
	virtual void CreatePinWidgets() OVERRIDE;
	// End of SGraphNode interface

	/**
	 * Update this GraphNode to match the data that it is observing
	 */
	virtual void UpdateGraphNode();

private:

	// the functions handle 'AddPin' button
	EVisibility IsAddPinButtonVisible() const;
	FReply OnAddPin();
};
