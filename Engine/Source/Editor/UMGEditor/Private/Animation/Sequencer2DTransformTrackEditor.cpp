// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"
#include "Sequencer2DTransformTrackEditor.h"
#include "MovieScene2DTransformSection.h"
#include "MovieScene2DTransformTrack.h"
#include "PropertySection.h"
#include "ISectionLayoutBuilder.h"
#include "MovieSceneToolHelpers.h"


class F2DTransformSection
	: public FPropertySection
{
public:

	F2DTransformSection( UMovieSceneSection& InSectionObject, FName SectionName )
		: FPropertySection(InSectionObject, SectionName) {}

	virtual void GenerateSectionLayout( class ISectionLayoutBuilder& LayoutBuilder ) const override
	{
		UMovieScene2DTransformSection* TransformSection = Cast<UMovieScene2DTransformSection>(&SectionObject);

		// This generates the tree structure for the transform section
		LayoutBuilder.PushCategory("Location", NSLOCTEXT("F2DTransformSection", "LocationArea", "Location"));
			LayoutBuilder.AddKeyArea("Location.X", NSLOCTEXT("F2DTransformSection", "LocXArea", "X"), MakeShareable(new FFloatCurveKeyArea(&TransformSection->GetTranslationCurve(EAxis::X), TransformSection)));
			LayoutBuilder.AddKeyArea("Location.Y", NSLOCTEXT("F2DTransformSection", "LocYArea", "Y"), MakeShareable(new FFloatCurveKeyArea(&TransformSection->GetTranslationCurve(EAxis::Y), TransformSection)));
		LayoutBuilder.PopCategory();

		LayoutBuilder.PushCategory("Rotation", NSLOCTEXT("F2DTransformSection", "RotationArea", "Rotation"));
			LayoutBuilder.AddKeyArea("Rotation.Angle", NSLOCTEXT("F2DTransformSection", "AngleArea", "Angle"), MakeShareable(new FFloatCurveKeyArea(&TransformSection->GetRotationCurve(), TransformSection)));
		LayoutBuilder.PopCategory();

		LayoutBuilder.PushCategory("Scale", NSLOCTEXT("F2DTransformSection", "ScaleArea", "Scale"));
			LayoutBuilder.AddKeyArea("Scale.X", NSLOCTEXT("F2DTransformSection", "ScaleXArea", "X"), MakeShareable(new FFloatCurveKeyArea(&TransformSection->GetScaleCurve(EAxis::X), TransformSection)));
			LayoutBuilder.AddKeyArea("Scale.Y", NSLOCTEXT("F2DTransformSection", "ScaleYArea", "Y"), MakeShareable(new FFloatCurveKeyArea(&TransformSection->GetScaleCurve(EAxis::Y), TransformSection)));
		LayoutBuilder.PopCategory();

		LayoutBuilder.PushCategory("Sheer", NSLOCTEXT("F2DTransformSection", "SheerArea", "Sheer"));
			LayoutBuilder.AddKeyArea("Sheer.X", NSLOCTEXT("F2DTransformSection", "SheerXArea", "X"), MakeShareable(new FFloatCurveKeyArea(&TransformSection->GetSheerCurve(EAxis::X), TransformSection)));
			LayoutBuilder.AddKeyArea("Sheer.Y", NSLOCTEXT("F2DTransformSection", "SheerYArea", "Y"), MakeShareable(new FFloatCurveKeyArea(&TransformSection->GetSheerCurve(EAxis::Y), TransformSection)));
		LayoutBuilder.PopCategory();
	}
};


TSharedRef<ISequencerTrackEditor> F2DTransformTrackEditor::CreateTrackEditor( TSharedRef<ISequencer> InSequencer )
{
	return MakeShareable( new F2DTransformTrackEditor( InSequencer ) );
}


TSharedRef<ISequencerSection> F2DTransformTrackEditor::MakeSectionInterface( UMovieSceneSection& SectionObject, UMovieSceneTrack& Track )
{
	check( SupportsType( SectionObject.GetOuter()->GetClass() ) );

	UClass* SectionClass = SectionObject.GetOuter()->GetClass();
	return MakeShareable( new F2DTransformSection( SectionObject, Track.GetTrackName() ) );
}


bool F2DTransformTrackEditor::TryGenerateKeyFromPropertyChanged( const UMovieSceneTrack* InTrack, const FPropertyChangedParams& PropertyChangedParams, F2DTransformKey& OutKey )
{
	OutKey.CurveName = PropertyChangedParams.StructPropertyNameToKey;
	OutKey.Value = *PropertyChangedParams.GetPropertyValue<FWidgetTransform>();

	if (InTrack)
	{
		const UMovieScene2DTransformTrack* TransformTrack = CastChecked<const UMovieScene2DTransformTrack>( InTrack );
		if (TransformTrack)
		{
			float KeyTime =	GetTimeForKey(GetMovieSceneSequence());
			return TransformTrack->CanKeyTrack(KeyTime, OutKey, PropertyChangedParams.KeyParams);
		}
	}

	return false;
}
