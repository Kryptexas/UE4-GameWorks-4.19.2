// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneToolsPrivatePCH.h"
#include "MovieSceneByteTrack.h"
#include "MovieSceneByteSection.h"
#include "BytePropertySection.h"

void FBytePropertySection::GenerateSectionLayout( class ISectionLayoutBuilder& LayoutBuilder ) const
{
	UMovieSceneByteSection* ByteSection = Cast<UMovieSceneByteSection>( &SectionObject );
	if ( Enum != nullptr )
	{
		LayoutBuilder.SetSectionAsKeyArea( MakeShareable( new FEnumKeyArea( ByteSection->GetCurve(), ByteSection, Enum ) ) );
	}
	else
	{
		LayoutBuilder.SetSectionAsKeyArea( MakeShareable( new FIntegralKeyArea( ByteSection->GetCurve(), ByteSection ) ) );
	}
}