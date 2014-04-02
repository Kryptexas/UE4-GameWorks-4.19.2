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

protected:
	virtual void CreateOutputSideAddButton(TSharedPtr<SVerticalBox> OutputBox) OVERRIDE;
	virtual EVisibility IsAddPinButtonVisible() const OVERRIDE;
	virtual FReply OnAddPin() OVERRIDE;
	// End of SGraphNode interface
};
