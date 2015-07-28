// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class UMovieSceneColorSection;

/**
* A color section implementation
*/
class FColorPropertySection : public FPropertySection
{
public:
	FColorPropertySection( UMovieSceneSection& InSectionObject, FName SectionName, ISequencer* InSequencer, UMovieSceneTrack* InTrack )
		: FPropertySection( InSectionObject, SectionName )
		, Sequencer( InSequencer )
		, Track( InTrack ) {}

	virtual void GenerateSectionLayout( class ISectionLayoutBuilder& LayoutBuilder ) const override;

	virtual int32 OnPaintSection( const FGeometry& AllottedGeometry, const FSlateRect& SectionClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, bool bParentEnabled ) const override;

private:
	void ConsolidateColorCurves( TArray< TKeyValuePair<float, FLinearColor> >& OutColorKeys, const UMovieSceneColorSection* Section ) const;

	ISequencer* Sequencer;
	UMovieSceneTrack* Track;
};