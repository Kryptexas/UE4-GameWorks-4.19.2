// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/////////////////////////////////////////////////////
// SExternalImageReference

// This widget shows an external image preview of a per-project configurable image
// (one where the engine provides a default, but each project may have its own override)

class SHAREDSETTINGSWIDGETS_API SExternalImageReference : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SExternalImageReference)
		: _FileDescription(NSLOCTEXT("SExternalImageReference", "FileDescription", "External Image"))
		, _MaxDisplaySize(400.0f, 400.0f)
		, _RequiredSize(-1, -1)
		{}

		// The description of the file, used in error messages/notifications
		SLATE_ARGUMENT(FText, FileDescription)

		// How big should we display the image?
		SLATE_ARGUMENT(FVector2D, MaxDisplaySize)

		// How big does the image need to be (any size is allowed if this is omitted)
		SLATE_ARGUMENT(FIntPoint, RequiredSize)
	SLATE_END_ARGS()

public:
	SExternalImageReference();

	void Construct(const FArguments& InArgs, const FString& InBaseFilename, const FString& InOverrideFilename);

protected:
	bool HandleExternalImagePicked(const FString& InChosenImage, const FString& InTargetImage);

protected:
	FString BaseFilename;
	FString OverrideFilename;
	FText FileDescription;
};
