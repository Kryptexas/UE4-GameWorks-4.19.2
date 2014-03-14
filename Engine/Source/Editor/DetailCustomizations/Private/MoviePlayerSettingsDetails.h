// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FMoviePlayerSettingsDetails : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails( IDetailLayoutBuilder& DetailLayout ) OVERRIDE;

private:
	/** Generate a widget for a movie array element */
	void GenerateArrayElementWidget(TSharedRef<IPropertyHandle> PropertyHandle, int32 ArrayIndex, IDetailChildrenBuilder& ChildrenBuilder);

	/** Delegate handler for when a new movie path is picked */
	bool HandlePathPicked(FString& InOutPath);

private:
	/** Handle to the movies array property */
	TSharedPtr<IPropertyHandle> StartupMoviesPropertyHandle;
};