// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MediaAssetEditorPrivatePCH.h"


#define LOCTEXT_NAMESPACE "SMediaAssetEditorPlayer"


/* SMediaAssetEditorPlayer structors
 *****************************************************************************/

SMediaAssetEditorPlayer::SMediaAssetEditorPlayer( )
	: CaptionBuffer(MakeShareable(new FMediaSampleBuffer))
{ }


/* SMediaAssetEditorPlayer interface
 *****************************************************************************/

void SMediaAssetEditorPlayer::Construct( const FArguments& InArgs, UMediaAsset* InMediaAsset, const TSharedRef<ISlateStyle>& InStyle )
{
	MediaAsset = InMediaAsset;

	TSharedPtr<SViewport> ViewportWidget;

	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4.0f, 0.0f)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						// video track selector
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
									.Text(LOCTEXT("VideoTrackLabel", "Video Track:"))
							]

						+ SHorizontalBox::Slot()
							.FillWidth(1.0f)
							.Padding(4.0f, 0.0f)
							.VAlign(VAlign_Center)
							[
								SAssignNew(VideoTrackComboBox, SComboBox<IMediaTrackPtr>)
									.OnGenerateWidget(this, &SMediaAssetEditorPlayer::HandleVideoTrackComboBoxGenerateWidget)
									.OnSelectionChanged(this, &SMediaAssetEditorPlayer::HandleVideoTrackComboBoxSelectionChanged)
									.OptionsSource(&VideoTracks)
									.ToolTipText(LOCTEXT("CaptionTrackToolTip", "Select the video track to be played"))
									[
										SNew(STextBlock)
											.Text(this, &SMediaAssetEditorPlayer::HandleVideoTrackComboBoxText)
									]
							]
					]

				+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(16.0f, 0.0f, 0.0f, 0.0f)
					[
						// audio track selector
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
									.Text(LOCTEXT("AudioTrackLabel", "Audio Track:"))
							]

						+ SHorizontalBox::Slot()
							.FillWidth(1.0f)
							.Padding(4.0f, 0.0f, 0.0f, 0.0f)
							.VAlign(VAlign_Center)
							[
								SAssignNew(AudioTrackComboBox, SComboBox<IMediaTrackPtr>)
									.OnGenerateWidget(this, &SMediaAssetEditorPlayer::HandleAudioTrackComboBoxGenerateWidget)
									.OnSelectionChanged(this, &SMediaAssetEditorPlayer::HandleAudioTrackComboBoxSelectionChanged)
									.OptionsSource(&AudioTracks)
									.ToolTipText(LOCTEXT("CaptionTrackToolTip", "Select the audio track to be played"))
									[
										SNew(STextBlock)
											.Text(this, &SMediaAssetEditorPlayer::HandleAudioTrackComboBoxText)
									]
							]
					]

				+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(16.0f, 0.0f, 0.0f, 0.0f)
					[
						// caption track selector
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
									.Text(LOCTEXT("CaptionTrackLabel", "Caption Track:"))
							]

						+ SHorizontalBox::Slot()
							.FillWidth(1.0f)
							.Padding(4.0f, 0.0f, 0.0f, 0.0f)
							.VAlign(VAlign_Center)
							[
								SAssignNew(CaptionTrackComboBox, SComboBox<IMediaTrackPtr>)
									.OnGenerateWidget(this, &SMediaAssetEditorPlayer::HandleCaptionTrackComboBoxGenerateWidget)
									.OnSelectionChanged(this, &SMediaAssetEditorPlayer::HandleCaptionTrackComboBoxSelectionChanged)
									.OptionsSource(&CaptionTracks)
									.ToolTipText(LOCTEXT("CaptionTrackToolTip", "Select the caption text to be displayed"))
									[
										SNew(STextBlock)
											.Text(this, &SMediaAssetEditorPlayer::HandleCaptionTrackComboBoxText)
									]
							]
					]

				+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.HAlign(HAlign_Right)
					.Padding(16.0f, 0.0f, 0.0f, 0.0f)
					[
						// playback rate spin box
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
									.Text(LOCTEXT("CaptionTrackLabel", "Playback Rate:"))
							]

						+ SHorizontalBox::Slot()
							.FillWidth(1.0f)
							.Padding(4.0f, 0.0f, 0.0f, 0.0f)
							.VAlign(VAlign_Center)
							[
								SNew(SNumericEntryBox<float>)
									.Delta(1.0f)
									.MaxValue(this, &SMediaAssetEditorPlayer::HandlePlaybackRateBoxMaxValue)
									.MinValue(this, &SMediaAssetEditorPlayer::HandlePlaybackRateBoxMinValue)
									.Value(this, &SMediaAssetEditorPlayer::HandlePlaybackRateSpinBoxValue)
									.OnValueChanged(this, &SMediaAssetEditorPlayer::HandlePlaybackRateBoxValueChanged)
							]
					]
			]

		+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(0.0f, 4.0f, 0.0f, 8.0f)
			[
				SNew(SOverlay)

				+ SOverlay::Slot()
					[
						// movie viewport
						SAssignNew(ViewportWidget, SViewport)
							.EnableGammaCorrection(false)
					]

				+ SOverlay::Slot()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Top)
					.Padding(FMargin(12.0f, 8.0f))
					[
						// playback state
						SNew(STextBlock)
							.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 20))
							.ShadowOffset(FVector2D(1.f, 1.f))
							.Text(this, &SMediaAssetEditorPlayer::HandleOverlayStateText)
					]

				+ SOverlay::Slot()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Bottom)
					.Padding(16.0f)
					[
						// caption text
						SNew(STextBlock)
							.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 16))
							.ShadowOffset(FVector2D(1.f, 1.f))
							.Text(this, &SMediaAssetEditorPlayer::HandleOverlayCaptionText)
							.WrapTextAt(600.0f)
					]
			]

		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4.0f, 0.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SHorizontalBox)

				// elapsed time
				+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(STextBlock)
							.Text(this, &SMediaAssetEditorPlayer::HandleElapsedTimeTextBlockText)
							.ToolTipText(LOCTEXT("ElapsedTimeTooltip", "Elapsed Time"))
					]

				// scrubber
				+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.Padding(8.0f, 0.0f)
					[
						SAssignNew(ScrubberSlider, SSlider)
							.IsEnabled(this, &SMediaAssetEditorPlayer::HandlePositionSliderIsEnabled)
							.OnMouseCaptureBegin(this, &SMediaAssetEditorPlayer::HandlePositionSliderMouseCaptureBegin)
							.OnMouseCaptureEnd(this, &SMediaAssetEditorPlayer::HandlePositionSliderMouseCaptureEnd)
							.OnValueChanged(this, &SMediaAssetEditorPlayer::HandlePositionSliderValueChanged)
							.Orientation(Orient_Horizontal)
							.Value(this, &SMediaAssetEditorPlayer::HandlePositionSliderValue)
					]

				// remaining time
				+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(STextBlock)
							.Text(this, &SMediaAssetEditorPlayer::HandleRemainingTimeTextBlockText)
							.ToolTipText(LOCTEXT("RemainingTimeTooltip", "Remaining Time"))
					]
			]
	];

	Viewport = MakeShareable(new FMediaAssetEditorViewport());
	ViewportWidget->SetViewportInterface(Viewport.ToSharedRef());

	ReloadMediaAsset();
}


/* SMediaAssetEditorPlayer implementation
 *****************************************************************************/

void SMediaAssetEditorPlayer::ReloadMediaAsset( )
{
	// clear track collections
	IMediaTrackPtr SelectedCaptionTrack = CaptionTrackComboBox->GetSelectedItem();

	if (SelectedCaptionTrack.IsValid())
	{
		SelectedCaptionTrack->RemoveSink(CaptionBuffer);
	}

	AudioTracks.Reset();
	CaptionTracks.Reset();
	VideoTracks.Reset();

	// get track collections
	if (MediaAsset != nullptr)
	{
		IMediaPlayerPtr MediaPlayer = MediaAsset->GetMediaPlayer();

		if (MediaPlayer.IsValid())
		{
			for (IMediaTrackPtr Track : MediaPlayer->GetTracks())
			{
				switch (Track->GetType())
				{
				case EMediaTrackTypes::Audio:
					AudioTracks.Add(Track);
					break;

				case EMediaTrackTypes::Caption:
					CaptionTracks.Add(Track);
					break;

				case EMediaTrackTypes::Video:
					VideoTracks.Add(Track);
					break;
				}
			}
		}
	}

	// refresh combo box selections
	if (!AudioTrackComboBox->GetSelectedItem().IsValid() || !AudioTracks.Contains(AudioTrackComboBox->GetSelectedItem()))
	{
		if (AudioTracks.Num() > 0)
		{
			AudioTrackComboBox->SetSelectedItem(AudioTracks[0]);
		}
		else
		{
			AudioTrackComboBox->SetSelectedItem(nullptr);
		}		
	}

	if (!CaptionTrackComboBox->GetSelectedItem().IsValid() || !CaptionTracks.Contains(CaptionTrackComboBox->GetSelectedItem()))
	{
		if (CaptionTracks.Num() > 0)
		{
			CaptionTrackComboBox->SetSelectedItem(CaptionTracks[0]);
			CaptionTracks[0]->AddSink(CaptionBuffer);
		}
		else
		{
			CaptionTrackComboBox->SetSelectedItem(nullptr);
		}		
	}

	if (!VideoTrackComboBox->GetSelectedItem().IsValid() || !VideoTracks.Contains(VideoTrackComboBox->GetSelectedItem()))
	{
		if (VideoTracks.Num() > 0)
		{
			VideoTrackComboBox->SetSelectedItem(VideoTracks[0]);
		}
		else
		{
			VideoTrackComboBox->SetSelectedItem(nullptr);
		}		
	}

	AudioTrackComboBox->RefreshOptions();
	CaptionTrackComboBox->RefreshOptions();
	VideoTrackComboBox->RefreshOptions();

	// refresh viewport
	Viewport->Initialize(VideoTrackComboBox->GetSelectedItem());
}


/* SMediaAssetEditorPlayer callbacks
 *****************************************************************************/

TSharedRef<SWidget> SMediaAssetEditorPlayer::HandleAudioTrackComboBoxGenerateWidget( IMediaTrackPtr Value ) const
{
	return SNew(STextBlock)
		.Text(Value->GetDisplayName());
}


void SMediaAssetEditorPlayer::HandleAudioTrackComboBoxSelectionChanged( IMediaTrackPtr Selection, ESelectInfo::Type SelectInfo )
{
	if (SelectInfo != ESelectInfo::Direct)
	{
		ReloadMediaAsset();
	}
}


FText SMediaAssetEditorPlayer::HandleAudioTrackComboBoxText( ) const
{
	IMediaTrackPtr AudioTrack = AudioTrackComboBox->GetSelectedItem();

	if (AudioTrack.IsValid())
	{
		return AudioTrack->GetDisplayName();
	}

	if (AudioTracks.Num() == 0)
	{
		return LOCTEXT("NoAudioTracksAvailable", "No audio tracks available");
	}

	return LOCTEXT("SelectAudioTrack", "Select an audio track...");
}


TSharedRef<SWidget> SMediaAssetEditorPlayer::HandleCaptionTrackComboBoxGenerateWidget( IMediaTrackPtr Value ) const
{
	return SNew(STextBlock)
		.Text(Value->GetDisplayName());
}


void SMediaAssetEditorPlayer::HandleCaptionTrackComboBoxSelectionChanged( IMediaTrackPtr Selection, ESelectInfo::Type SelectInfo )
{
	if (SelectInfo != ESelectInfo::Direct)
	{
		ReloadMediaAsset();
	}
}


FText SMediaAssetEditorPlayer::HandleCaptionTrackComboBoxText( ) const
{
	IMediaTrackPtr CaptionTrack = CaptionTrackComboBox->GetSelectedItem();

	if (CaptionTrack.IsValid())
	{
		return CaptionTrack->GetDisplayName();
	}

	if (CaptionTracks.Num() == 0)
	{
		return LOCTEXT("NoCaptionTracksAvailable", "No caption tracks available");
	}

	return LOCTEXT("SelectCaptionTrack", "Select a caption track...");
}


FText SMediaAssetEditorPlayer::HandleElapsedTimeTextBlockText( ) const
{
	return FText::AsTimespan(MediaAsset->GetTime());
}


FText SMediaAssetEditorPlayer::HandleOverlayCaptionText( ) const
{
	TSharedPtr<TArray<uint8>, ESPMode::ThreadSafe> CurrentCaption = CaptionBuffer->GetCurrentSample();

	if (!CurrentCaption.IsValid())
	{
		return FText::GetEmpty();
	}

	return FText::FromString(FString((TCHAR*)CurrentCaption->GetData()));
}


FText SMediaAssetEditorPlayer::HandleOverlayStateText( ) const
{
	if (MediaAsset->IsPaused())
	{
		return LOCTEXT("StateOverlayPaused", "Paused");
	}

	if (MediaAsset->IsStopped())
	{
		return LOCTEXT("StateOverlayStopped", "Stopped");
	}

	float Rate = MediaAsset->GetRate();

	if (FMath::IsNearlyZero(Rate) || (Rate == 1.0f))
	{
		return FText::GetEmpty();
	}

	if (Rate < 0.0f)
	{
		return FText::Format(LOCTEXT("StateOverlayReverseFormat", "Reverse ({0}x)"), FText::AsNumber(-Rate));
	}

	return FText::Format(LOCTEXT("StateOverlayForwardFormat", "Forward ({0}x)"), FText::AsNumber(Rate));
}


TOptional<float> SMediaAssetEditorPlayer::HandlePlaybackRateBoxMaxValue( ) const
{
	IMediaPlayerPtr MediaPlayer = MediaAsset->GetMediaPlayer();

	if (MediaPlayer.IsValid())
	{
		return MediaPlayer->GetMediaInfo().GetSupportedRates(EMediaPlaybackDirections::Forward, false).GetUpperBoundValue();
	}

	return 0.0f;
}


TOptional<float> SMediaAssetEditorPlayer::HandlePlaybackRateBoxMinValue( ) const
{
	IMediaPlayerPtr MediaPlayer = MediaAsset->GetMediaPlayer();

	if (MediaPlayer.IsValid())
	{
		return MediaPlayer->GetMediaInfo().GetSupportedRates(EMediaPlaybackDirections::Reverse, false).GetLowerBoundValue();
	}

	return 0.0f;
}


TOptional<float> SMediaAssetEditorPlayer::HandlePlaybackRateSpinBoxValue( ) const
{
	return MediaAsset->GetRate();
}


void SMediaAssetEditorPlayer::HandlePlaybackRateBoxValueChanged( float NewValue )
{
	MediaAsset->SetRate(NewValue);
}


bool SMediaAssetEditorPlayer::HandlePositionSliderIsEnabled( ) const
{
	return MediaAsset->SupportsSeeking();
}


void SMediaAssetEditorPlayer::HandlePositionSliderMouseCaptureBegin( )
{
	if (MediaAsset->SupportsScrubbing())
	{
		PreScrubRate = MediaAsset->GetRate();
		MediaAsset->SetRate(0.0f);
	}
}


void SMediaAssetEditorPlayer::HandlePositionSliderMouseCaptureEnd( )
{
	if (MediaAsset->SupportsScrubbing())
	{
		MediaAsset->SetRate(PreScrubRate);
	}
}


float SMediaAssetEditorPlayer::HandlePositionSliderValue( ) const
{
	if (MediaAsset->GetDuration() > FTimespan::Zero())
	{
		return (float)MediaAsset->GetTime().GetTicks() / (float)MediaAsset->GetDuration().GetTicks();
	}

	return 0.0;
}


void SMediaAssetEditorPlayer::HandlePositionSliderValueChanged( float NewValue )
{
	if (!ScrubberSlider->HasMouseCapture() || MediaAsset->SupportsScrubbing())
	{
		MediaAsset->Seek(MediaAsset->GetDuration() * NewValue);
	}
}


FText SMediaAssetEditorPlayer::HandleRemainingTimeTextBlockText( ) const
{
	return FText::AsTimespan(MediaAsset->GetDuration() - MediaAsset->GetTime());
}


TSharedRef<SWidget> SMediaAssetEditorPlayer::HandleVideoTrackComboBoxGenerateWidget( IMediaTrackPtr Value ) const
{
	return SNew(STextBlock)
		.Text(Value->GetDisplayName());
}


void SMediaAssetEditorPlayer::HandleVideoTrackComboBoxSelectionChanged( IMediaTrackPtr Selection, ESelectInfo::Type SelectInfo )
{
	if (SelectInfo != ESelectInfo::Direct)
	{
		ReloadMediaAsset();
	}
}


FText SMediaAssetEditorPlayer::HandleVideoTrackComboBoxText( ) const
{
	IMediaTrackPtr VideoTrack = VideoTrackComboBox->GetSelectedItem();

	if (VideoTrack.IsValid())
	{
		return VideoTrack->GetDisplayName();
	}

	if (VideoTracks.Num() == 0)
	{
		return LOCTEXT("NoVideoTracksAvailable", "No video tracks available");
	}

	return LOCTEXT("SelectVideoTrack", "Select a video track...");
}


#undef LOCTEXT_NAMESPACE
