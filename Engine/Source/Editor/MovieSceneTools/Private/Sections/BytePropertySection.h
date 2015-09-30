// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
* An implementation of byte property sections
*/
class FBytePropertySection : public FPropertySection
{
public:
	FBytePropertySection( UMovieSceneSection& InSectionObject, FName InSectionName, UEnum* InEnum )
		: FPropertySection( InSectionObject, InSectionName )
		, Enum( InEnum ) {}

	virtual void GenerateSectionLayout( class ISectionLayoutBuilder& LayoutBuilder ) const override;

private:
	UEnum* Enum;
};