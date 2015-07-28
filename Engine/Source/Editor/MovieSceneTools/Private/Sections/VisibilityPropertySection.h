// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BoolPropertySection.h"

/**
* An implementation of visibility property sections
*/
class FVisibilityPropertySection : public FBoolPropertySection
{
public:
	FVisibilityPropertySection( UMovieSceneSection& InSectionObject, FName SectionName, ISequencer* InSequencer )
		: FBoolPropertySection( InSectionObject, SectionName, InSequencer )
	{
		DisplayName = FText::FromString( TEXT( "Visible" ) );
	}
};