// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Media.h"
#include "MediaAsset.h"


/**
 * Implements a details view customization for the UMediaAsset class.
 */
class FMediaAssetCustomization
	: public IDetailCustomization
{
public:

	// IDetailCustomization interface

	virtual void CustomizeDetails( IDetailLayoutBuilder& DetailBuilder ) override;

public:

	/**
	 * Creates an instance of this class.
	 *
	 * @return The new instance.
	 */
	static TSharedRef<IDetailCustomization> MakeInstance( )
	{
		return MakeShareable(new FMediaAssetCustomization());
	}

protected:

	/**
	 * Gets the playback rate range text for the specified playback direction.
	 *
	 * @param Direction The playback direction.
	 * @return The supported playback rate range converted to text.
	 */
	FText GetRatesText( EMediaPlaybackDirections Direction ) const;

private:

	// Callback for getting the text of the Duration text block.
	FText HandleDurationTextBlockText( ) const;

	// Callback for getting the text of the ForwardRates text block.
	FText HandleForwardRatesTextBlockText( ) const;

	// Callback for getting the text of a supported playback rate text block.
	FText HandleSupportedRatesTextBlockText( EMediaPlaybackDirections Direction, bool Unthinned ) const;

	// Callback for getting the text of the SupportsScrubbing text block.
	FText HandleSupportsScrubbingTextBlockText( ) const;

	// Callback for getting the text of the SupportsSeeking text block.
	FText HandleSupportsSeekingTextBlockText( ) const;

private:

	/** The collection of media assets being customized */
	TArray<TWeakObjectPtr<UObject>> CustomizedMediaAssets;
};
