// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "DetailCustomizationsPrivatePCH.h"
#include "Media.h"
#include "MediaAsset.h"
#include "MediaAssetCustomization.h"
#include "SFilePathPicker.h"


#define LOCTEXT_NAMESPACE "FMediaAssetCustomization"


/* IDetailCustomization interface
 *****************************************************************************/

void FMediaAssetCustomization::CustomizeDetails( IDetailLayoutBuilder& DetailBuilder )
{
	DetailBuilder.GetObjectsBeingCustomized(CustomizedMediaAssets);

	// customize 'Source' category
	IDetailCategoryBuilder& SourceCategory = DetailBuilder.EditCategory("Source", TEXT(""));
	{
		// URL
		UrlProperty = DetailBuilder.GetProperty("URL");
		{
			IDetailPropertyRow& UrlRow = SourceCategory.AddProperty(UrlProperty);

			UrlRow.DisplayName(TEXT("URL"));
			UrlRow.CustomWidget()
				.NameContent()
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							UrlProperty->CreatePropertyNameWidget()
						]

					+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.HAlign(HAlign_Left)
						.Padding(4.0f, 0.0f, 0.0f, 0.0f)
						[
							SNew(SImage)
								.Image(FCoreStyle::Get().GetBrush("Icons.Warning"))
								.ToolTipText(LOCTEXT("InvalidUrlPathWarning", "The current URL points to a file that does not exist or is not located inside the /Content/Movies/ directory."))
								.Visibility(this, &FMediaAssetCustomization::HandleUrlWarningIconVisibility)
						]
				]
				.ValueContent()
				.MaxDesiredWidth(0.0f)
				.MinDesiredWidth(125.0f)
				[
					SNew(SFilePathPicker)
						.BrowseButtonImage(FEditorStyle::GetBrush("PropertyWindow.Button_Ellipsis"))
						.BrowseButtonStyle(FEditorStyle::Get(), "HoverHintOnly")
						.BrowseButtonToolTip(LOCTEXT("UrlBrowseButtonToolTipText", "Choose a file from this computer"))
						.BrowseDirectory(FPaths::GameContentDir() / TEXT("Movies"))
						.FilePath(this, &FMediaAssetCustomization::HandleUrlPickerFilePath)
						.FileTypeFilter(this, &FMediaAssetCustomization::HandleUrlPickerFileTypeFilter)
						.OnPathPicked(this, &FMediaAssetCustomization::HandleUrlPickerPathPicked)
				];
		}
	}

	// customize 'Information' category
	IDetailCategoryBuilder& InformationCategory = DetailBuilder.EditCategory("Information", TEXT(""), ECategoryPriority::Uncommon);
	{
		// duration
		InformationCategory.AddCustomRow(TEXT("Duration"))
			.NameContent()
			[
				SNew(STextBlock)
					.Text(LOCTEXT("Duration", "Duration"))
					.Font(FEditorStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
			]
			.ValueContent()
			.MaxDesiredWidth(0.0f)
			[
				SNew(STextBlock)
					.Text(this, &FMediaAssetCustomization::HandleDurationTextBlockText)
					.Font(FEditorStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
			];

		// forward rates (thinned)
		InformationCategory.AddCustomRow(TEXT("ForwardRatesThinned"))
			.NameContent()
			[
				SNew(STextBlock)
					.Text(LOCTEXT("ForwardRatesThinned", "Forward Rates (Thinned)"))
					.Font(FEditorStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
			]
			.ValueContent()
			.MaxDesiredWidth(0.0f)
			[
				SNew(STextBlock)
					.Text(this, &FMediaAssetCustomization::HandleSupportedRatesTextBlockText, EMediaPlaybackDirections::Forward, false)
					.Font(FEditorStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
			];

		// forward rates (unthinned)
		InformationCategory.AddCustomRow(TEXT("ForwardRatesUnthinned"))
			.NameContent()
			[
				SNew(STextBlock)
					.Text(LOCTEXT("ForwardRatesUnthinned", "Forward Rates (Unhinned)"))
					.Font(FEditorStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
			]
			.ValueContent()
			.MaxDesiredWidth(0.0f)
			[
				SNew(STextBlock)
					.Text(this, &FMediaAssetCustomization::HandleSupportedRatesTextBlockText, EMediaPlaybackDirections::Forward, true)
					.Font(FEditorStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
			];

		// reverse rates (thinned)
		InformationCategory.AddCustomRow(TEXT("ReverseRatesThinned"))
			.NameContent()
			[
				SNew(STextBlock)
					.Text(LOCTEXT("ReverseRatesThinned", "Reverse Rates (Thinned)"))
					.Font(FEditorStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
			]
			.ValueContent()
			.MaxDesiredWidth(0.0f)
			[
				SNew(STextBlock)
					.Text(this, &FMediaAssetCustomization::HandleSupportedRatesTextBlockText, EMediaPlaybackDirections::Reverse, false)
					.Font(FEditorStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
			];

		// reverse rates (unthinned)
		InformationCategory.AddCustomRow(TEXT("ReverseRatesUnthinned"))
			.NameContent()
			[
				SNew(STextBlock)
					.Text(LOCTEXT("ReverseRatesUnthinned", "Reverse Rates (Unthinned)"))
					.Font(FEditorStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
			]
			.ValueContent()
			.MaxDesiredWidth(0.0f)
			[
				SNew(STextBlock)
					.Text(this, &FMediaAssetCustomization::HandleSupportedRatesTextBlockText, EMediaPlaybackDirections::Reverse, true)
					.Font(FEditorStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
			];

		// supports scrubbing
		InformationCategory.AddCustomRow(TEXT("SupportsScrubbing"))
			.NameContent()
			[
				SNew(STextBlock)
					.Text(LOCTEXT("SupportsScrubbing", "Supports Scrubbing"))
					.Font(FEditorStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
			]
			.ValueContent()
			.MaxDesiredWidth(0.0f)
			[
				SNew(STextBlock)
					.Text(this, &FMediaAssetCustomization::HandleSupportsScrubbingTextBlockText)
					.Font(FEditorStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
			];

		// supports seeking
		InformationCategory.AddCustomRow(TEXT("SupportsSeeking"))
			.NameContent()
			[
				SNew(STextBlock)
					.Text(LOCTEXT("SupportsSeeking", "Supports Seeking"))
					.Font(FEditorStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
			]
			.ValueContent()
			.MaxDesiredWidth(0.0f)
			[
				SNew(STextBlock)
					.Text(this, &FMediaAssetCustomization::HandleSupportsSeekingTextBlockText)
					.Font(FEditorStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
			];
	}
}


/* FMediaAssetCustomization callbacks
 *****************************************************************************/

FText FMediaAssetCustomization::HandleDurationTextBlockText( ) const
{
	TOptional<FTimespan> Duration;

	// ensure that all assets have the same value
	for (auto MediaAssetObject : CustomizedMediaAssets)
	{
		if (MediaAssetObject.IsValid())
		{
			FTimespan OtherDuration = Cast<UMediaAsset>(MediaAssetObject.Get())->GetDuration();

			if (!Duration.IsSet())
			{
				Duration = OtherDuration;
			}
			else if (OtherDuration != Duration.GetValue())
			{
				return FText::GetEmpty();
			}
		}
	}

	if (!Duration.IsSet())
	{
		return FText::GetEmpty();
	}

	return FText::AsTimespan(Duration.GetValue());
}


FText FMediaAssetCustomization::HandleSupportedRatesTextBlockText( EMediaPlaybackDirections Direction, bool Unthinned ) const
{
	TOptional<TRange<float>> Rates;

	// ensure that all assets have the same value
	for (auto MediaAssetObject : CustomizedMediaAssets)
	{
		if (MediaAssetObject.IsValid())
		{
			IMediaPlayerPtr MediaPlayer = Cast<UMediaAsset>(MediaAssetObject.Get())->GetMediaPlayer();

			if (!MediaPlayer.IsValid())
			{
				return FText::GetEmpty();
			}

			TRange<float> OtherRates = MediaPlayer->GetMediaInfo().GetSupportedRates(Direction, Unthinned);

			if (!Rates.IsSet())
			{
				Rates = OtherRates;
			}
			else if (!MediaPlayer.IsValid() || (OtherRates != Rates.GetValue()))
			{
				return FText::GetEmpty();
			}
		}
	}

	if (!Rates.IsSet())
	{
		return FText::GetEmpty();
	}

	if ((Rates.GetValue().GetLowerBoundValue() == 0.0f) && (Rates.GetValue().GetUpperBoundValue() == 0.0f))
	{
		return LOCTEXT("RateNotSupported", "Not supported");
	}

	return FText::Format(LOCTEXT("RatesFormat", "{0} to {1}"),
		FText::AsNumber(Rates.GetValue().GetLowerBoundValue()),
		FText::AsNumber(Rates.GetValue().GetUpperBoundValue()));
}


FText FMediaAssetCustomization::HandleSupportsScrubbingTextBlockText( ) const
{
	TOptional<bool> SupportsScrubbing;

	// ensure that all assets have the same value
	for (auto MediaAssetObject : CustomizedMediaAssets)
	{
		if (MediaAssetObject.IsValid())
		{
			bool OtherSupportsScrubbing = Cast<UMediaAsset>(MediaAssetObject.Get())->SupportsScrubbing();

			if (!SupportsScrubbing.IsSet())
			{
				SupportsScrubbing = OtherSupportsScrubbing;
			}
			else if (OtherSupportsScrubbing != SupportsScrubbing.GetValue())
			{
				return FText::GetEmpty();
			}
		}
	}

	if (!SupportsScrubbing.IsSet())
	{
		return FText::GetEmpty();
	}

	return SupportsScrubbing.GetValue() ? GTrue : GFalse;
}


FText FMediaAssetCustomization::HandleSupportsSeekingTextBlockText( ) const
{
	TOptional<bool> SupportsSeeking;

	// ensure that all assets have the same value
	for (auto MediaAssetObject : CustomizedMediaAssets)
	{
		if (MediaAssetObject.IsValid())
		{
			bool OtherSupportsSeeking = Cast<UMediaAsset>(MediaAssetObject.Get())->SupportsSeeking();

			if (!SupportsSeeking.IsSet())
			{
				SupportsSeeking = OtherSupportsSeeking;
			}
			else if (OtherSupportsSeeking != SupportsSeeking.GetValue())
			{
				return FText::GetEmpty();
			}
		}
	}

	if (!SupportsSeeking.IsSet())
	{
		return FText::GetEmpty();
	}

	return SupportsSeeking.GetValue() ? GTrue : GFalse;
}


FString FMediaAssetCustomization::HandleUrlPickerFilePath( ) const
{
	FString Url;
	UrlProperty->GetValue(Url);

	return Url;
}


FString FMediaAssetCustomization::HandleUrlPickerFileTypeFilter( ) const
{
	FString Filter = TEXT("All files (*.*)|*.*");

	auto MediaModule = FModuleManager::GetModulePtr<IMediaModule>("Media");

	if (MediaModule == nullptr)
	{
		return Filter;
	}

	FMediaFormats SupportedFormats;
	MediaModule->GetSupportedFormats(SupportedFormats);

	if (SupportedFormats.Num() == 0)
	{
		return Filter;
	}

	FString AllExtensions;
	FString AllFilters;
			
	for (auto& Format : SupportedFormats)
	{
		if (!AllExtensions.IsEmpty())
		{
			AllExtensions += TEXT(";");
		}

		AllExtensions += TEXT("*.") + Format.Key;
		AllFilters += TEXT("|") + Format.Value.ToString() + TEXT(" (*.") + Format.Key + TEXT(")|*.") + Format.Key;
	}

	Filter += TEXT("|All movie files (") + AllExtensions + TEXT(")|") + AllExtensions + AllFilters;

	return Filter;
}


void FMediaAssetCustomization::HandleUrlPickerPathPicked( const FString& PickedPath )
{
	FString FullPath;

	if (!PickedPath.IsEmpty() && FPaths::IsRelative(PickedPath))
	{
		FullPath = FPaths::ConvertRelativePathToFull(PickedPath);
	}
	else
	{
		FullPath = PickedPath;
		FPaths::NormalizeFilename(FullPath);
	}

	if (FullPath.StartsWith(FPaths::RootDir()))
	{
		FPaths::MakePathRelativeTo(FullPath, FPlatformProcess::BaseDir());
	}

	UrlProperty->SetValue(FullPath);
}


EVisibility FMediaAssetCustomization::HandleUrlWarningIconVisibility( ) const
{
	FString Url;

	if ((UrlProperty->GetValue(Url) != FPropertyAccess::Success) || Url.IsEmpty() || Url.Contains(TEXT("://")))
	{
		return EVisibility::Hidden;
	}

	const FString FullPath = FPaths::ConvertRelativePathToFull(Url);
	const FString FullMoviesPath = FPaths::ConvertRelativePathToFull(FPaths::GameContentDir() / TEXT("Movies"));

	if (FullPath.StartsWith(FullMoviesPath))
	{
		if (FPaths::FileExists(FullPath))
		{
			return EVisibility::Hidden;
		}

		// file doesn't exist
		return EVisibility::Visible;
	}

	// file not inside Movies folder
	return EVisibility::Visible;
}


#undef LOCTEXT_NAMESPACE
