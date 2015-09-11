// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
* A movie scene section for material parameters.
*/
class FMaterialParameterSection : public FPropertySection
{
public:
	FMaterialParameterSection( UMovieSceneSection& InSectionObject, FName SectionName )
		: FPropertySection( InSectionObject, SectionName ) {}

	// ISequencerSection interface.
	virtual void GenerateSectionLayout( class ISectionLayoutBuilder& LayoutBuilder ) const override;
	virtual bool RequestDeleteCategory( const TArray<FName>& CategoryNamePath ) override;
	virtual bool RequestDeleteKeyArea( const TArray<FName>& KeyAreaNamePath ) override;
};