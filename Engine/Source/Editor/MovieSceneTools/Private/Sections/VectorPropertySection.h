// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
* An implementation of vector property sections
*/
class FVectorPropertySection : public FPropertySection
{
public:
	FVectorPropertySection( UMovieSceneSection& InSectionObject, FName SectionName )
		: FPropertySection( InSectionObject, SectionName ) {}

	virtual void GenerateSectionLayout( class ISectionLayoutBuilder& LayoutBuilder ) const override;
};