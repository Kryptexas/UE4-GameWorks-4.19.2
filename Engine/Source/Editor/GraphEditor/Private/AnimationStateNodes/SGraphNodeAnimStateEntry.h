// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class SGraphNodeAnimStateEntry : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SGraphNodeAnimStateEntry){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UAnimStateEntryNode* InNode);

	// SNodePanel::SNode interface
	virtual void GetNodeInfoPopups(FNodeInfoContext* Context, TArray<FGraphInformationPopupInfo>& Popups) const OVERRIDE;
	// End of SNodePanel::SNode interface

	// SGraphNode interface
	virtual void UpdateGraphNode() OVERRIDE;
	virtual void AddPin(const TSharedRef<SGraphPin>& PinToAdd) OVERRIDE;
	
	// End of SGraphNode interface

	
protected:
	FSlateColor GetBorderBackgroundColor() const;

	FString GetPreviewCornerText() const;
};
