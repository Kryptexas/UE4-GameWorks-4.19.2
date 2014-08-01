// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


/**
 * Implements the details panel of the media asset editor.
 */
class SMediaAssetEditorDetails
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SMediaAssetEditorDetails) { }
	SLATE_END_ARGS()

public:

	/**
	 * Construct this widget
	 *
	 * @param InArgs The declaration data for this widget.
	 * @param InMediaAsset The media asset to show the details for.
	 * @param InStyleSet The style set to use.
	 */
	void Construct( const FArguments& InArgs, UMediaAsset* InMediaAsset, const TSharedRef<ISlateStyle>& InStyle );

private:

	// Pointer to the media player that is being viewed.
	UMediaAsset* MediaAsset;
};
