// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class SGraphNodeStateMachineInstance : public SGraphNodeK2Composite
{
public:
	SLATE_BEGIN_ARGS(SGraphNodeStateMachineInstance){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, class UAnimGraphNode_StateMachineBase* InNode);
protected:
	// SGraphNodeK2Composite interface
	virtual UEdGraph* GetInnerGraph() const OVERRIDE;
	// End of SGraphNodeK2Composite interface
};
