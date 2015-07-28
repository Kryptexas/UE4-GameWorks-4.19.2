// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneToolsPrivatePCH.h"
#include "VectorPropertySection.h"
#include "MovieSceneVectorSection.h"

void FVectorPropertySection::GenerateSectionLayout( class ISectionLayoutBuilder& LayoutBuilder ) const
{
	UMovieSceneVectorSection* VectorSection = Cast<UMovieSceneVectorSection>( &SectionObject );
	int32 ChannelsUsed = VectorSection->GetChannelsUsed();
	check( ChannelsUsed >= 2 && ChannelsUsed <= 4 );

	LayoutBuilder.AddKeyArea( "Vector.X", NSLOCTEXT( "FVectorPropertySection", "XArea", "X" ), MakeShareable( new FFloatCurveKeyArea( &VectorSection->GetCurve( 0 ), VectorSection ) ) );
	LayoutBuilder.AddKeyArea( "Vector.Y", NSLOCTEXT( "FVectorPropertySection", "YArea", "Y" ), MakeShareable( new FFloatCurveKeyArea( &VectorSection->GetCurve( 1 ), VectorSection ) ) );
	if ( ChannelsUsed >= 3 )
	{
		LayoutBuilder.AddKeyArea( "Vector.Z", NSLOCTEXT( "FVectorPropertySection", "ZArea", "Z" ), MakeShareable( new FFloatCurveKeyArea( &VectorSection->GetCurve( 2 ), VectorSection ) ) );
	}
	if ( ChannelsUsed >= 4 )
	{
		LayoutBuilder.AddKeyArea( "Vector.W", NSLOCTEXT( "FVectorPropertySection", "WArea", "W" ), MakeShareable( new FFloatCurveKeyArea( &VectorSection->GetCurve( 3 ), VectorSection ) ) );
	}
}