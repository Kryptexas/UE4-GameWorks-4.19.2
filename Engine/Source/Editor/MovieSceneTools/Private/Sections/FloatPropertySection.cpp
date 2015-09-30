// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneToolsPrivatePCH.h"
#include "FloatPropertySection.h"
#include "MovieSceneFloatSection.h"

void FFloatPropertySection::GenerateSectionLayout( class ISectionLayoutBuilder& LayoutBuilder ) const
{
	UMovieSceneFloatSection* FloatSection = Cast<UMovieSceneFloatSection>( &SectionObject );
	LayoutBuilder.SetSectionAsKeyArea( MakeShareable( new FFloatCurveKeyArea( &FloatSection->GetFloatCurve(), FloatSection ) ) );
}