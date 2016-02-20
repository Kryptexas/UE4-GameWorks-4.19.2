// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SequencerPrivatePCH.h"
#include "Sequencer.h"
#include "SequencerSectionPainter.h"

FSequencerSectionPainter::FSequencerSectionPainter(FSlateWindowElementList& OutDrawElements, const FGeometry& InSectionGeometry, UMovieSceneSection& InSection)
	: Section(InSection)
	, DrawElements(OutDrawElements)
	, SectionGeometry(InSectionGeometry)
	, LayerId(0)
	, bParentEnabled(true)
{
}

int32 FSequencerSectionPainter::PaintSectionBackground(const FColor& Tint)
{
	FSequencerSectionTintRegion Region(Tint);
	return PaintSectionBackground(&Region, 1);
}

int32 FSequencerSectionPainter::PaintSectionBackground()
{
	FSequencerSectionTintRegion Region(GetTrack()->GetColorTint());
	return PaintSectionBackground(&Region, 1);
}

UMovieSceneTrack* FSequencerSectionPainter::GetTrack() const
{
	return Section.GetTypedOuter<UMovieSceneTrack>();
}