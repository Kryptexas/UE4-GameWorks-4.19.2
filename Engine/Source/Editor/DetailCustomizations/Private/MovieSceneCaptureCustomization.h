// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class IDetailsView;

class FMovieSceneCaptureCustomization : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};
