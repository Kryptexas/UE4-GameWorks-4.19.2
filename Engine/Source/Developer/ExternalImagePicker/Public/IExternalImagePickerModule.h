// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Delegate fired when picking a new image.
 * @param	InChosenImage	The path to the image that has been picked by the user.
 * @param	InTargetImage	The path to the target image that this picker represents.
 * @return true if the image should be refreshed
 */
DECLARE_DELEGATE_RetVal_TwoParams(bool, FOnExternalImagePicked, const FString& /** InChosenImage */, const FString& /** InTargetImage */);

/**
 * Used for configuring the external image picker
 */
struct FExternalImagePickerConfiguration
{
	FExternalImagePickerConfiguration()
		: MaxDisplayedImageDimensions(400.0f, 400.0f)
		, bRequiresSpecificSize(false)
	{
	}

	/** The image on disk that the splash screen is stored as. */
	FString TargetImagePath;

	/** The image on disk that we will use if the target does not exist. */
	FString DefaultImagePath;

	/** Delegate fired when picking a new image. */
	FOnExternalImagePicked OnExternalImagePicked;

	/** The dimensions the image display should be constrained to. Aspect ratio is maintained. */
	FVector2D MaxDisplayedImageDimensions;

	/** The size the actual image needs to be (ignored unless bRequiresSpecificSize is set) */
	FIntPoint RequiredImageDimensions;

	/** Does the image need to be a specific size? */
	bool bRequiresSpecificSize;
};

/**
 * Public interface for splash screen editor module
 */
class IExternalImagePickerModule : public IModuleInterface
{
public:
	/**
	 * Make a widget used for displaying & editing splash screens
	 *
	 * @param	
	 * @return the image editing widget
	 */
	virtual TSharedRef<class SWidget> MakeEditorWidget(const FExternalImagePickerConfiguration& InConfiguration) = 0;

	/**
	 * Gets a reference to the ExternalImagePicker module instance.
	 *
	 * @return A reference to the ExternalImagePicker module.
	 */
	static IExternalImagePickerModule& Get()
	{
		return FModuleManager::LoadModuleChecked<IExternalImagePickerModule>("ExternalImagePicker");
	}

};