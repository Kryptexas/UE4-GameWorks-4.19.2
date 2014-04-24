// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class SGraphNodeAnimState : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SGraphNodeAnimState){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UAnimStateNodeBase* InNode);

	// SNodePanel::SNode interface
	virtual void GetNodeInfoPopups(FNodeInfoContext* Context, TArray<FGraphInformationPopupInfo>& Popups) const OVERRIDE;
	// End of SNodePanel::SNode interface

	// SGraphNode interface
	virtual void UpdateGraphNode() OVERRIDE;
	virtual void CreatePinWidgets() OVERRIDE;
	virtual void AddPin(const TSharedRef<SGraphPin>& PinToAdd) OVERRIDE;
	virtual TSharedPtr<SToolTip> GetComplexTooltip() OVERRIDE;
	// End of SGraphNode interface

	static void GetStateInfoPopup(UEdGraphNode* GraphNode, TArray<FGraphInformationPopupInfo>& Popups);
protected:
	FSlateColor GetBorderBackgroundColor() const;

	virtual FString GetPreviewCornerText() const;
	virtual const FSlateBrush* GetNameIcon() const;
};


class SGraphNodeAnimConduit : public SGraphNodeAnimState
{
public:
	SLATE_BEGIN_ARGS(SGraphNodeAnimConduit){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UAnimStateConduitNode* InNode);

	// SNodePanel::SNode interface
	virtual void GetNodeInfoPopups(FNodeInfoContext* Context, TArray<FGraphInformationPopupInfo>& Popups) const OVERRIDE;
	// End of SNodePanel::SNode interface
protected:
	virtual FString GetPreviewCornerText() const OVERRIDE;
	virtual const FSlateBrush* GetNameIcon() const OVERRIDE;
};
