// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneToolsPrivatePCH.h"
#include "BoolKeyArea.h"
#include "VisibilityPropertySection.h"
#include "MovieSceneVisibilitySection.h"


void FVisibilityPropertySection::GenerateSectionLayout(class ISectionLayoutBuilder& LayoutBuilder) const
{
	UMovieSceneVisibilitySection* VisibilitySection = Cast<UMovieSceneVisibilitySection>(&SectionObject);
	TAttribute<TOptional<bool>> ExternalValue;
	ExternalValue.Bind(TAttribute<TOptional<bool>>::FGetter::CreateLambda([&]
	{
		TOptional<bool> BoolValue = GetPropertyValue<bool>();
		return BoolValue.IsSet()
			? TOptional<bool>(!BoolValue.GetValue())
			: TOptional<bool>();
	}));
	TSharedRef<FBoolKeyArea> KeyArea = MakeShareable(new FBoolKeyArea(VisibilitySection->GetCurve(), ExternalValue, VisibilitySection));
	LayoutBuilder.SetSectionAsKeyArea(KeyArea);
}