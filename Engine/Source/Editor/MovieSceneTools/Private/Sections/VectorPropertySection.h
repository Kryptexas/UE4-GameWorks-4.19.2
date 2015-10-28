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

	// FPropertySectionInterface

	virtual void GenerateSectionLayout( class ISectionLayoutBuilder& LayoutBuilder ) const override;
	virtual void SetIntermediateValue( FPropertyChangedParams PropertyChangedParams ) override;
	virtual void ClearIntermediateValue() override;

private:
	mutable TSharedPtr<FFloatCurveKeyArea> XKeyArea;
	mutable TSharedPtr<FFloatCurveKeyArea> YKeyArea;
	mutable TSharedPtr<FFloatCurveKeyArea> ZKeyArea;
	mutable TSharedPtr<FFloatCurveKeyArea> WKeyArea;
};