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

protected:
	virtual void CreateInputSideAddButton(TSharedPtr<SVerticalBox> InputBox) OVERRIDE;
	virtual EVisibility IsAddPinButtonVisible() const OVERRIDE;
	virtual FReply OnAddPin() OVERRIDE;
	// End of SGraphNode interface
};
