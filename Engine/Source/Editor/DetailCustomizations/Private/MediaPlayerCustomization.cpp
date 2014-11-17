// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "DetailCustomizationsPrivatePCH.h"
#include "Media.h"
#include "MediaPlayer.h"
#include "MediaPlayerCustomization.h"
#include "SFilePathPicker.h"


#define LOCTEXT_NAMESPACE "FMediaPlayerCustomization"


/* IDetailCustomization interface
 *****************************************************************************/

void FMediaPlayerCustomization::CustomizeDetails( IDetailLayoutBuilder& DetailBuilder )
{
	DetailBuilder.GetObjectsBeingCustomized(CustomizedMediaPlayers);

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
								.Visibility(this, &FMediaPlayerCustomization::HandleUrlWarningIconVisibility)
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
						.FilePath(this, &FMediaPlayerCustomization::HandleUrlPickerFilePath)
						.FileTypeFilter(this, &FMediaPlayerCustomization::HandleUrlPickerFileTypeFilter)
						.OnPathPicked(this, &FMediaPlayerCustomization::HandleUrlPickerPathPicked)
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
					.Text(this, &FMediaPlayerCustomization::HandleDurationTextBlockText)
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
					.Text(this, &FMediaPlayerCustomization::HandleSupportedRatesTextBlockText, EMediaPlaybackDirections::Forward, false)
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
					.Text(this, &FMediaPlayerCustomization::HandleSupportedRatesTextBlockText, EMediaPlaybackDirections::Forward, true)
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
					.Text(this, &FMediaPlayerCustomization::HandleSupportedRatesTextBlockText, EMediaPlaybackDirections::Reverse, false)
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
					.Text(this, &FMediaPlayerCustomization::HandleSupportedRatesTextBlockText, EMediaPlaybackDirections::Reverse, true)
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
					.Text(this, &FMediaPlayerCustomization::HandleSupportsScrubbingTextBlockText)
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
					.Text(this, &FMediaPlayerCustomization::HandleSupportsSeekingTextBlockText)
					.Font(FEditorStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
			];
	}
}


/* FMediaPlayerCustomization callbacks
 *****************************************************************************/

FText FMediaPlayerCustomization::HandleDurationTextBlockText( ) const
{
	TOptional<FTimespan> Duration;

	// ensure that all assets have the same value
	for (auto MediaPlayerObject : CustomizedMediaPlayers)
	{
		if (MediaPlayerObject.IsValid())
		{
			FTimespan OtherDuration = Cast<UMediaPlayer>(MediaPlayerObject.Get())->GetDuration();

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


FText FMediaPlayerCustomization::HandleSupportedRatesTextBlockText( EMediaPlaybackDirections Direction, bool Unthinned ) const
{
	TOptional<TRange<float>> Rates;

	// ensure that all assets have the same value
	for (auto MediaPlayerObject : CustomizedMediaPlayers)
	{
		if (MediaPlayerObject.IsValid())
		{
			IMediaPlayerPtr Player = Cast<UMediaPlayer>(MediaPlayerObject.Get())->GetPlayer();

			if (!Player.IsValid())
			{
				return FText::GetEmpty();
			}

			TRange<float> OtherRates = Player->GetMediaInfo().GetSupportedRates(Direction, Unthinned);

			if (!Rates.IsSet())
			{
				Rates = OtherRates;
			}
			else if (!Player.IsValid() || (OtherRates != Rates.GetValue()))
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


FText FMediaPlayerCustomization::HandleSupportsScrubbingTextBlockText( ) const
{
	TOptional<bool> SupportsScrubbing;

	// ensure that all assets have the same value
	for (auto MediaPlayerObject : CustomizedMediaPlayers)
	{
		if (MediaPlayerObject.IsValid())
		{
			bool OtherSupportsScrubbing = Cast<UMediaPlayer>(MediaPlayerObject.Get())->SupportsScrubbing();

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


FText FMediaPlayerCustomization::HandleSupportsSeekingTextBlockText( ) const
{
	TOptional<bool> SupportsSeeking;

	// ensure that all assets have the same value
	for (auto MediaPlayerObject : CustomizedMediaPlayers)
	{
		if (MediaPlayerObject.IsValid())
		{
			bool OtherSupportsSeeking = Cast<UMediaPlayer>(MediaPlayerObject.Get())->SupportsSeeking();

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


FString FMediaPlayerCustomization::HandleUrlPickerFilePath( ) const
{
	FString Url;
	UrlProperty->GetValue(Url);

	return Url;
}


FString FMediaPlayerCustomization::HandleUrlPickerFileTypeFilter( ) const
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


void FMediaPlayerCustomization::HandleUrlPickerPathPicked( const FString& PickedPath )
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


EVisibility FMediaPlayerCustomization::HandleUrlWarningIconVisibility( ) const
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
