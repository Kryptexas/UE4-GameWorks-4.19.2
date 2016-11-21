// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

class IDetailsView;

class FMovieSceneCurveInterfaceKeyEditStructCustomization : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};