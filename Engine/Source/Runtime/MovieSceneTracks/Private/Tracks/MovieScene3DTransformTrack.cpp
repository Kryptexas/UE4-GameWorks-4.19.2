// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Tracks/MovieScene3DTransformTrack.h"
#include "MovieSceneCommonHelpers.h"
#include "Sections/MovieScene3DTransformSection.h"
#include "Compilation/MovieSceneTemplateInterrogation.h"
#include "Compilation/MovieSceneSegmentCompiler.h"
#include "MovieSceneEvaluationTrack.h"

UMovieScene3DTransformTrack::UMovieScene3DTransformTrack( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer )
{
	static FName Transform("Transform");
	SetPropertyNameAndPath(Transform, Transform.ToString());

	SupportedBlendTypes = FMovieSceneBlendTypeField::All();

#if WITH_EDITORONLY_DATA
	TrackTint = FColor(65, 173, 164, 65);
#endif

	EvalOptions.bEvaluateNearestSection_DEPRECATED = EvalOptions.bCanEvaluateNearestSection = true;
}

TArray<FTrajectoryKey> UMovieScene3DTransformTrack::GetTrajectoryData() const
{
	TArray<FTrajectoryKey> Data;

	struct FSectionAndCurve
	{
		FSectionAndCurve(UMovieScene3DTransformSection* InSection, EMovieSceneTransformChannel InType, const FRichCurve& InCurve)
			: Section(InSection), Type(InType), KeyIndex(0)
		{
			TRange<float> SectionRange = InSection->IsInfinite() ? TRange<float>::All() : InSection->GetRange();
			
			for (const FRichCurveKey& Key : InCurve.GetConstRefOfKeys())
			{
				if (SectionRange.Contains(Key.Time))
				{
					Keys.Emplace(Key.Time, Key.InterpMode);
				}
			}

			// Inser the start and end ranges
			if (!SectionRange.GetLowerBound().IsOpen() && (!Keys.Num() || !FMath::IsNearlyEqual(SectionRange.GetLowerBoundValue(), Keys[0].Get<0>())))
			{
				Keys.Insert(MakeTuple(SectionRange.GetLowerBoundValue(), RCIM_None), 0);
			}
			if (!SectionRange.GetUpperBound().IsOpen() && (!Keys.Num() || !FMath::IsNearlyEqual(SectionRange.GetUpperBoundValue(), Keys.Last().Get<0>())))
			{
				Keys.Add(MakeTuple(SectionRange.GetUpperBoundValue(), RCIM_None));
			}
		}

		UMovieScene3DTransformSection* Section;
		EMovieSceneTransformChannel Type;
		TArray<TTuple<float, ERichCurveInterpMode>> Keys;
		/** Running key index into the above array of tuples */
		int32 KeyIndex;
	};
	TArray<FSectionAndCurve> SectionsAndCurves;

	for (UMovieSceneSection* Section : Sections)
	{
		UMovieScene3DTransformSection* TransformSection = Cast<UMovieScene3DTransformSection>(Section);
		if (TransformSection)
		{
			EMovieSceneTransformChannel Mask = TransformSection->GetMask().GetChannels();
			if (EnumHasAnyFlags(Mask, EMovieSceneTransformChannel::TranslationX))
			{
				SectionsAndCurves.Emplace(TransformSection, EMovieSceneTransformChannel::TranslationX, TransformSection->GetTranslationCurve(EAxis::X));
			}
			if (EnumHasAnyFlags(Mask, EMovieSceneTransformChannel::TranslationY))
			{
				SectionsAndCurves.Emplace(TransformSection, EMovieSceneTransformChannel::TranslationY, TransformSection->GetTranslationCurve(EAxis::Y));
			}
			if (EnumHasAnyFlags(Mask, EMovieSceneTransformChannel::TranslationZ))
			{
				SectionsAndCurves.Emplace(TransformSection, EMovieSceneTransformChannel::TranslationZ, TransformSection->GetTranslationCurve(EAxis::Z));
			}
			if (EnumHasAnyFlags(Mask, EMovieSceneTransformChannel::RotationX))
			{
				SectionsAndCurves.Emplace(TransformSection, EMovieSceneTransformChannel::RotationX, TransformSection->GetRotationCurve(EAxis::X));
			}
			if (EnumHasAnyFlags(Mask, EMovieSceneTransformChannel::RotationY))
			{
				SectionsAndCurves.Emplace(TransformSection, EMovieSceneTransformChannel::RotationY, TransformSection->GetRotationCurve(EAxis::Y));
			}
			if (EnumHasAnyFlags(Mask, EMovieSceneTransformChannel::RotationZ))
			{
				SectionsAndCurves.Emplace(TransformSection, EMovieSceneTransformChannel::RotationZ, TransformSection->GetRotationCurve(EAxis::Z));
			}
		}
	}

	int32 TotalNumKeys = 0;
	for (const FSectionAndCurve& SectionAndCurve : SectionsAndCurves)
	{
		TotalNumKeys += SectionAndCurve.Keys.Num();
	}

	int32 NumVisitedKeys = 0;
	while (NumVisitedKeys < TotalNumKeys)
	{
		FTrajectoryKey& NewKey = Data[Data.Emplace(TNumericLimits<float>::Max())];

		// Find the earliest key time
		for (const FSectionAndCurve& SectionAndCurve : SectionsAndCurves)
		{
			if (SectionAndCurve.Keys.IsValidIndex(SectionAndCurve.KeyIndex))
			{
				NewKey.Time = FMath::Min(NewKey.Time, SectionAndCurve.Keys[SectionAndCurve.KeyIndex].Get<0>());
			}
		}

		// Add any keys at the current time to this trajectory key
		for (FSectionAndCurve& SectionAndCurve : SectionsAndCurves)
		{
			if (SectionAndCurve.Keys.IsValidIndex(SectionAndCurve.KeyIndex) && FMath::IsNearlyEqual(NewKey.Time, SectionAndCurve.Keys[SectionAndCurve.KeyIndex].Get<0>()))
			{
				// Add this key to the trajectory key
				NewKey.KeyData.Emplace(SectionAndCurve.Section, SectionAndCurve.Keys[SectionAndCurve.KeyIndex].Get<1>(), SectionAndCurve.Type);

				// Move onto the next key in this curve
				++SectionAndCurve.KeyIndex;
				++NumVisitedKeys;
			}
		}
	}
	return Data;
}


UMovieSceneSection* UMovieScene3DTransformTrack::CreateNewSection()
{
	return NewObject<UMovieSceneSection>(this, UMovieScene3DTransformSection::StaticClass(), NAME_None, RF_Transactional);
}

FMovieSceneInterrogationKey UMovieScene3DTransformTrack::GetInterrogationKey()
{
	static FMovieSceneAnimTypeID TypeID = FMovieSceneAnimTypeID::Unique();
	return TypeID;
}