// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class SGraphNodeAnimTransition : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SGraphNodeAnimTransition){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UAnimStateTransitionNode* InNode);

	// SNodePanel::SNode interface
	virtual void GetNodeInfoPopups(FNodeInfoContext* Context, TArray<FGraphInformationPopupInfo>& Popups) const OVERRIDE;
	virtual void MoveTo(const FVector2D& NewPosition) OVERRIDE;
	virtual bool RequiresSecondPassLayout() const OVERRIDE;
	virtual void PerformSecondPassLayout(const TMap< UObject*, TSharedRef<SNode> >& NodeToWidgetLookup) const OVERRIDE;
	// End of SNodePanel::SNode interface

	// SGraphNode interface
	virtual void UpdateGraphNode() OVERRIDE;
	virtual TSharedPtr<SToolTip> GetComplexTooltip() OVERRIDE;
	// End of SGraphNode interface

	// SWidget interface
	void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) OVERRIDE;
	void OnMouseLeave(const FPointerEvent& MouseEvent) OVERRIDE;
	// End of SWidget interface

	static FLinearColor StaticGetTransitionColor(UAnimStateTransitionNode* TransNode, bool bIsHovered);
private:
	TSharedPtr<STextEntryPopup> TextEntryWidget;

private:
	FString GetPreviewCornerText(bool reverse) const;
	FSlateColor GetTransitionColor() const;

	TSharedRef<SWidget> GenerateInlineDisplayOrEditingWidget(bool bShowGraphPreview);
	TSharedRef<SWidget> GenerateRichTooltip();

	FString GetCurrentDuration() const;
	FString GetCurrentAlphaCurveMode() const;
};
