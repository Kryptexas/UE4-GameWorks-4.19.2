// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SGraphNodeDefault.h"
#include "K2Node_Knot.h"

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
};
