// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#ifndef __SGraphNodeK2Sequence_h__
#define __SGraphNodeK2Sequence_h__

struct FAddPinButtonHelper
{
	static EVisibility IsAddPinButtonVisible(TWeakPtr<SGraphPanel> OwnerGraphPanelPtr);

	static TSharedRef<SWidget> AddPinButtonContent(bool bRightSide = true);
};

class SGraphNodeK2Sequence : public SGraphNodeK2Base
{
public:
	SLATE_BEGIN_ARGS(SGraphNodeK2Sequence){}
	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs, UK2Node* InNode );

	/**
	 * Update this GraphNode to match the data that it is observing
	 */
	virtual void UpdateGraphNode();

private:

	/** Callback function to initiate adding new pin to the sequence node */
	FReply OnAddPin();

	EVisibility IsAddPinButtonVisible() const;
};

#endif // __SGraphNodeK2Sequence_h__