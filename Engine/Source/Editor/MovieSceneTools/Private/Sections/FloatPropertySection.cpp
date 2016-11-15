// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneToolsPrivatePCH.h"
#include "FloatCurveKeyArea.h"
#include "FloatPropertySection.h"
#include "MovieSceneFloatSection.h"
#include "MovieScenePropertyTrack.h"

void FFloatPropertySection::GenerateSectionLayout(class ISectionLayoutBuilder& LayoutBuilder) const
{
	UMovieSceneFloatSection* FloatSection = Cast<UMovieSceneFloatSection>(&SectionObject);
	TAttribute<TOptional<float>> ExternalValue;
	if (CanGetPropertyValue())
	{
		ExternalValue.Bind(TAttribute<TOptional<float>>::FGetter::CreateLambda([&]
		{
			return GetPropertyValue<float>();
		}));
	}
	TSharedRef<FFloatCurveKeyArea> KeyArea = MakeShareable(
		new FFloatCurveKeyArea(&FloatSection->GetFloatCurve(), ExternalValue, FloatSection));
	LayoutBuilder.SetSectionAsKeyArea(KeyArea);
}
