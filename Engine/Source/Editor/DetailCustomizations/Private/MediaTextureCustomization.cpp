// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "DetailCustomizationsPrivatePCH.h"
#include "MediaAsset.h"
#include "MediaTextureCustomization.h"


#define LOCTEXT_NAMESPACE "FMediaTextureCustomization"


/* IDetailCustomization interface
 *****************************************************************************/

void FMediaTextureCustomization::CustomizeDetails( IDetailLayoutBuilder& DetailBuilder )
{
	DetailBuilder.GetObjectsBeingCustomized(CustomizedMediaTextures);

	IDetailCategoryBuilder& MediaAssetCategory = DetailBuilder.EditCategory(TEXT("MediaAsset"));
	
	MediaAssetProperty = DetailBuilder.GetProperty("MediaAsset");
	VideoTrackIndexProperty = DetailBuilder.GetProperty("VideoTrackIndex");

	// customize video track index	
	IDetailPropertyRow& VideoTrackIndexRow = MediaAssetCategory.AddProperty(VideoTrackIndexProperty);

	VideoTrackIndexRow.DisplayName(TEXT("Video Track"));
	VideoTrackIndexRow.CustomWidget()
		.NameContent()
		[
			VideoTrackIndexProperty->CreatePropertyNameWidget()
		]
		.ValueContent()
		.MaxDesiredWidth(0)
		[
			SNew(SComboButton)
				.OnGetMenuContent(this, &FMediaTextureCustomization::HandleVideoTrackComboButtonMenuContent)
 				.ContentPadding(FMargin( 2.0f, 2.0f ))
				.ButtonContent()
				[
					SNew(STextBlock) 
						.Text(this, &FMediaTextureCustomization::HandleVideoTrackComboButtonText)
						.Font(IDetailLayoutBuilder::GetDetailFont())
				]
		];
}


/* FMediaTextureCustomization callbacks
 *****************************************************************************/

TSharedRef<SWidget> FMediaTextureCustomization::HandleVideoTrackComboButtonMenuContent( ) const
{
	// get assigned media asset
	UObject* MediaAssetObj = nullptr;
	FPropertyAccess::Result Result = MediaAssetProperty->GetValue(MediaAssetObj);

	if (Result == FPropertyAccess::MultipleValues)
	{
		return SNullWidget::NullWidget;
	}

	UMediaAsset* MediaAsset = Cast<UMediaAsset>(MediaAssetObj);

	if (MediaAsset == nullptr)
	{
		return SNullWidget::NullWidget;
	}

	// get media tracks
	IMediaPlayerPtr MediaPlayer = MediaAsset->GetMediaPlayer();

	if (!MediaPlayer.IsValid())
	{
		return SNullWidget::NullWidget;
	}

	const TArray<IMediaTrackRef>& MediaTracks = MediaPlayer->GetTracks();

	// populate the menu
	FMenuBuilder MenuBuilder(true, nullptr);

	for (const IMediaTrackRef& Track : MediaTracks)
	{
		if (Track->GetType() == EMediaTrackTypes::Video)
		{
			FUIAction Action(FExecuteAction::CreateSP(this, &FMediaTextureCustomization::HandleVideoTrackComboButtonMenuEntryExecute, Track->GetIndex()));

			MenuBuilder.AddMenuEntry(
				Track->GetDisplayName(),
				TAttribute<FText>(),
				FSlateIcon(),
				Action
			);
		}
	}

	return MenuBuilder.MakeWidget();
}


void FMediaTextureCustomization::HandleVideoTrackComboButtonMenuEntryExecute( uint32 TrackIndex )
{
	VideoTrackIndexProperty->SetValue((int32)TrackIndex);
}


FText FMediaTextureCustomization::HandleVideoTrackComboButtonText( ) const
{
	// get assigned media asset
	UObject* MediaAssetObj = nullptr;
	FPropertyAccess::Result Result = MediaAssetProperty->GetValue(MediaAssetObj);

	if (Result != FPropertyAccess::Success)
	{
		return FText::GetEmpty();
	}

	UMediaAsset* MediaAsset = Cast<UMediaAsset>(MediaAssetObj);

	if (MediaAsset == nullptr)
	{
		return LOCTEXT("NoMediaAssetSelected", "No Media Asset selected");
	}

	// get selected value
	int32 VideoTrackIndex = -1;
	Result = VideoTrackIndexProperty->GetValue(VideoTrackIndex);

	if (Result != FPropertyAccess::Success)
	{
		return FText::GetEmpty();
	}

	// get selected media track
	IMediaPlayerPtr MediaPlayer = MediaAsset->GetMediaPlayer();

	if (!MediaPlayer.IsValid())
	{
		return LOCTEXT("NoMediaLoaded", "No media loaded");
	}

	IMediaTrackPtr Track = MediaPlayer->GetTrackSafe(VideoTrackIndex, EMediaTrackTypes::Video);

	// generate track name string
	if (Track.IsValid())
	{
		return Track->GetDisplayName();
	}

	return LOCTEXT("SelectVideoTrack", "Select a Video Track...");
}


#undef LOCTEXT_NAMESPACE
