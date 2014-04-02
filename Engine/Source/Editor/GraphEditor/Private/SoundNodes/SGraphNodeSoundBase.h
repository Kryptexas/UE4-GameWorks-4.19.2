// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class SGraphNodeSoundBase : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SGraphNodeSoundBase){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, class USoundCueGraphNode* InNode);

protected:
	// SGraphNode Interface
	virtual void CreateOutputSideAddButton(TSharedPtr<SVerticalBox> OutputBox) OVERRIDE;
	virtual EVisibility IsAddPinButtonVisible() const OVERRIDE;
	virtual FReply OnAddPin() OVERRIDE;

private:
	USoundCueGraphNode* SoundNode;
};
