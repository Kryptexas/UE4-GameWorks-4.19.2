// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

// This widget provides a fully-zoomed-out preview of a specified graph
class GRAPHEDITOR_API SGraphPreviewer : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGraphPreviewer) {}
		SLATE_ATTRIBUTE( FString, CornerOverlayText )
		SLATE_ARGUMENT( TSharedPtr<SWidget>, TitleBar )
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraph* InGraphObj);
public:
	// SWidget interface
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) OVERRIDE;
	// End of SWidget interface
protected:
	/// The Graph we are currently viewing
	UEdGraph* EdGraphObj;

	// Do we need to refresh next tick?
	bool bNeedsRefresh;

	// The underlying graph panel
	TSharedPtr<SGraphPanel> GraphPanel;
};
