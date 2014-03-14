// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class SGraphNodeSoundResult : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SGraphNodeSoundResult){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, class USoundCueGraphNode_Base* InNode);

protected:
	// SGraphNode interface
	virtual TSharedRef<SWidget> CreateNodeContentArea() OVERRIDE;
	// End of SGraphNode interface
};
