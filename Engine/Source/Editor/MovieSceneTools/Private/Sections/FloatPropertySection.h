// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
* An implementation of float property sections
*/
class FFloatPropertySection : public FPropertySection
{
public:
	FFloatPropertySection( UMovieSceneSection& InSectionObject, FName SectionName )
		: FPropertySection( InSectionObject, SectionName ) {}

	virtual void GenerateSectionLayout( class ISectionLayoutBuilder& LayoutBuilder ) const override;
};