// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneToolsPrivatePCH.h"
#include "IntegerKeyArea.h"
#include "IntegerPropertySection.h"
#include "MovieSceneIntegerSection.h"
#include "MovieScenePropertyTrack.h"

void FIntegerPropertySection::GenerateSectionLayout(class ISectionLayoutBuilder& LayoutBuilder) const
{
	UMovieSceneIntegerSection* IntegerSection = Cast<UMovieSceneIntegerSection>(&SectionObject);
	TAttribute<TOptional<int32>> ExternalValue;
	if (CanGetPropertyValue())
	{
		ExternalValue.Bind(TAttribute<TOptional<int32>>::FGetter::CreateLambda([&]
		{
			return GetPropertyValue<int32>();
		}));
	}
	TSharedRef<FIntegerKeyArea> KeyArea = MakeShareable(
		new FIntegerKeyArea(IntegerSection->GetCurve(), ExternalValue, IntegerSection));
	LayoutBuilder.SetSectionAsKeyArea(KeyArea);
}
