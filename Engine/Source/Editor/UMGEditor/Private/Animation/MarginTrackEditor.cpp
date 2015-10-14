// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"
#include "MovieSceneMarginTrack.h"
#include "MarginTrackEditor.h"
#include "MovieSceneMarginSection.h"
#include "PropertySection.h"
#include "ISectionLayoutBuilder.h"
#include "MovieSceneToolHelpers.h"


class FMarginPropertySection
	: public FPropertySection
{
public:
	FMarginPropertySection( UMovieSceneSection& InSectionObject, FName SectionName )
		: FPropertySection(InSectionObject, SectionName) {}

	virtual void GenerateSectionLayout( class ISectionLayoutBuilder& LayoutBuilder ) const override
	{
		UMovieSceneMarginSection* MarginSection = Cast<UMovieSceneMarginSection>(&SectionObject);

		LayoutBuilder.AddKeyArea("Left", NSLOCTEXT("FMarginPropertySection", "MarginLeft", "Left"), MakeShareable(new FFloatCurveKeyArea(&MarginSection->GetLeftCurve(), MarginSection)));
		LayoutBuilder.AddKeyArea("Top", NSLOCTEXT("FMarginPropertySection", "MarginTop", "Top"), MakeShareable(new FFloatCurveKeyArea(&MarginSection->GetTopCurve(), MarginSection)));
		LayoutBuilder.AddKeyArea("Right", NSLOCTEXT("FMarginPropertySection", "MarginRight", "Right"), MakeShareable(new FFloatCurveKeyArea(&MarginSection->GetRightCurve(), MarginSection)));
		LayoutBuilder.AddKeyArea("Bottom", NSLOCTEXT("FMarginPropertySection", "MarginBottom", "Bottom"), MakeShareable(new FFloatCurveKeyArea(&MarginSection->GetBottomCurve(), MarginSection)));
	}
};


TSharedRef<ISequencerTrackEditor> FMarginTrackEditor::CreateTrackEditor( TSharedRef<ISequencer> InSequencer )
{
	return MakeShareable( new FMarginTrackEditor( InSequencer ) );
}


TSharedRef<ISequencerSection> FMarginTrackEditor::MakeSectionInterface( UMovieSceneSection& SectionObject, UMovieSceneTrack& Track )
{
	check( SupportsType( SectionObject.GetOuter()->GetClass() ) );

	UClass* SectionClass = SectionObject.GetOuter()->GetClass();
	return MakeShareable( new FMarginPropertySection( SectionObject, Track.GetTrackName() ) );
}


bool FMarginTrackEditor::TryGenerateKeyFromPropertyChanged( const UMovieSceneTrack* InTrack, const FPropertyChangedParams& PropertyChangedParams, FMarginKey& OutKey )
{
	OutKey.CurveName = PropertyChangedParams.StructPropertyNameToKey;
	OutKey.Value = *PropertyChangedParams.GetPropertyValue<FMargin>();

	if (InTrack)
	{
		const UMovieSceneMarginTrack* MarginTrack = CastChecked<const UMovieSceneMarginTrack>( InTrack );
		if (MarginTrack)
		{
			float KeyTime =	GetTimeForKey(GetMovieSceneSequence());
			return MarginTrack->CanKeyTrack(KeyTime, OutKey, PropertyChangedParams.KeyParams);
		}
	}

	return false;
}
