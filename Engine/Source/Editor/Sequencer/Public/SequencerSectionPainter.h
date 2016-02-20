// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

struct FTimeToPixel;
class UMovieSceneTrack;
class UMovieSceneSection;

/** Structure representing a specific region on a section to tint */
struct FSequencerSectionTintRegion
{
	FSequencerSectionTintRegion(const FColor& InColor)
		: Color(InColor)
	{}

	FSequencerSectionTintRegion(const FColor& InColor, float InStartTime, float InEndTime)
		: Color(InColor), Range(TRange<float>(InStartTime, InEndTime))
	{}

	/** The color of the tint */
	FColor Color;

	/** The time range to tint */
	TOptional<TRange<float>> Range;
};

/** Class that wraps up common section painting functionality */
class SEQUENCER_API FSequencerSectionPainter
{
public:
	/** Constructor */
	FSequencerSectionPainter(FSlateWindowElementList& OutDrawElements, const FGeometry& InSectionGeometry, UMovieSceneSection& Section);

	/** Paint the section background with the specified tint override */
	int32 PaintSectionBackground(const FColor& Tint);

	/** Paint the section background with the tint stored on the track */
	int32 PaintSectionBackground();

	/** Get the track that this painter is painting sections for */
	UMovieSceneTrack* GetTrack() const;

public:

	/** Paint the section background with the specified tints */
	virtual int32 PaintSectionBackground(const FSequencerSectionTintRegion* Tints, uint32 NumTints) = 0;

	/** Get a time-to-pixel converter for the section */
	virtual const FTimeToPixel& GetTimeConverter() const = 0;

public:

	/** The movie scene section we're painting */
	UMovieSceneSection& Section;

	/** List of slate draw elements - publicly modifiable */
	FSlateWindowElementList& DrawElements;

	/** The full geometry of the section. This is the width of the track area in the case of infinite sections */
	FGeometry SectionGeometry;
	
	/** The full clipping rectangle for the section */
	FSlateRect SectionClippingRect;
	
	/** The layer ID we're painting on */
	int32 LayerId;

	/** Whether our parent widget is enabled or not */
	bool bParentEnabled;
	
};

