// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ExternalImagePickerPrivatePCH.h"
#include "IExternalImagePickerModule.h"
#include "SExternalImagePicker.h"

/**
 * Public interface for splash screen editor module
 */
class FExternalImagePickerModule : public IExternalImagePickerModule
{
public:
	virtual TSharedRef<SWidget> MakeEditorWidget(const FExternalImagePickerConfiguration& InConfiguration) OVERRIDE
	{
		return SNew(SExternalImagePicker)
			.TargetImagePath(InConfiguration.TargetImagePath)
			.DefaultImagePath(InConfiguration.DefaultImagePath)
			.OnExternalImagePicked(InConfiguration.OnExternalImagePicked)
			.MaxDisplayedImageDimensions(InConfiguration.MaxDisplayedImageDimensions)
			.bRequiresSpecificSize(InConfiguration.bRequiresSpecificSize)
			.RequiredImageDimensions(InConfiguration.RequiredImageDimensions);
	}
};

IMPLEMENT_MODULE(FExternalImagePickerModule, ExternalImagePicker)