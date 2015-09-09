// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneToolsPrivatePCH.h"
#include "MaterialParameterSection.h"
#include "MovieSceneMaterialParameterSection.h"

void FMaterialParameterSection::GenerateSectionLayout( class ISectionLayoutBuilder& LayoutBuilder ) const
{
	UMovieSceneMaterialParameterSection* ParameterSection = Cast<UMovieSceneMaterialParameterSection>( &SectionObject );
	TArray<FScalarParameterNameAndCurve>* ScalarParameterNamesAndCurves = ParameterSection->GetScalarParameterNamesAndCurves();
	for ( int32 i = 0; i < ScalarParameterNamesAndCurves->Num(); i++)
	{
		LayoutBuilder.AddKeyArea( 
			(*ScalarParameterNamesAndCurves)[i].ParameterName, 
			FText::FromName( (*ScalarParameterNamesAndCurves)[i].ParameterName ),
			MakeShareable( new FFloatCurveKeyArea( &(*ScalarParameterNamesAndCurves)[i].ParameterCurve, ParameterSection ) ) );
	}
}