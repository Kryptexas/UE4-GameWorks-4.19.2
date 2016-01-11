// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
* An implementation of bool property sections
*/
class FBoolPropertySection : public FPropertySection
{
public:
	FBoolPropertySection( UMovieSceneSection& InSectionObject, FName SectionName, ISequencer* InSequencer )
		: FPropertySection( InSectionObject, SectionName )
		, Sequencer( InSequencer ) {}

	virtual void GenerateSectionLayout( class ISectionLayoutBuilder& LayoutBuilder ) const override;

	virtual int32 OnPaintSection( const FGeometry& AllottedGeometry, const FSlateRect& SectionClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, bool bParentEnabled ) const override;
private:
	ISequencer* Sequencer;
};