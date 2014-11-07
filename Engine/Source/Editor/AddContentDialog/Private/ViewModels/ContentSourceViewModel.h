// Copyright 1998 - 2014 Epic Games, Inc.All Rights Reserved.

#pragma once

/** A view model for displaying and interacting with an IContentSource in the FAddContentDialog. */
class FContentSourceViewModel : public TSharedFromThis<FContentSourceViewModel>
{
public:
	/** Creates a view model for a supplied content source. */
	FContentSourceViewModel(TSharedPtr<IContentSource> ContentSourceIn);

	/** Gets the content source represented by this view model. */
	TSharedPtr<IContentSource> GetContentSource();

	/** Gets the display name for this content source. */
	FText GetName();

	/** Gets the description of this content source. */
	FText GetDescription();

	/** Gets the view model for the category for this content source. */
	FCategoryViewModel GetCategory();

	/** Gets the brush which should be used to draw the icon representation of this content source. */
	TSharedPtr<FSlateBrush> GetIconBrush();

	/** Gets an array or brushes which should be used to display screenshots for this content source. */
	TArray<TSharedPtr<FSlateBrush>>* GetScreenshotBrushes();

	/** Gets the brush for the currently selected screenshot. */
	const FSlateBrush* GetSelectedScreenshotBrush() const;

	/** Selects the next available screenshot.  When at the last screenshot this will start again at
		the beginning of the array. */
	void SelectNextScreenshotBrush();

	/** Selects the previous available screenshot.  When at the first screenshot this will start again at
		the end of the array. */
	void SelectPreviousScreenshotBrush();

private:
	/** Sets up brushes from the images data supplied by the IContentSource. */
	void SetupBrushes();

	/** Creates a slate brush from raw binary PNG formatted image data and the supplied FName. */
	TSharedPtr<FSlateDynamicImageBrush> CreateBrushFromRawData(FName ResourceName, const TArray<uint8>& RawData) const;

	/** Selects an FLocalizedText from an array which matches either the supplied language code, or the default language code. */
	FLocalizedText ChooseLocalizedText(TArray<FLocalizedText> Choices, FString LanguageCode);

private:
	/** The content source represented by this view model. */
	TSharedPtr<IContentSource> ContentSource;

	/** The brush which should be used to draw the icon representation of this content source. */
	TSharedPtr<FSlateBrush> IconBrush;

	/** An array or brushes which should be used to display screenshots for this content source. */
	TArray<TSharedPtr<FSlateBrush>> ScreenshotBrushes;

	/** The brush for the currently selected screenshot. */
	TSharedPtr<FSlateBrush> SelectedScreenshotBrush;

	/** The view model for the category for this content source. */
	FCategoryViewModel Category;

	/** The FLocalizedText representing the name of the content source, in the language which was active the
		last time it was requested, or the default language if a translation was not available. */
	FLocalizedText NameText;

	/** The FLocalizedText representing the description of the content source, in the language which was active
		the last time it was requested, or the default language if a translation was not available. */
	FLocalizedText DescriptionText;
};