// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Widgets/SMediaPlayerEditorInfo.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "EditorStyleSet.h"
#include "IMediaControls.h"
#include "IMediaPlayer.h"
#include "MediaPlayer.h"
#include "HAL/PlatformApplicationMisc.h"


#define LOCTEXT_NAMESPACE "SMediaPlayerEditorInfo"


/* SMediaPlayerEditorInfo interface
 *****************************************************************************/

void SMediaPlayerEditorInfo::Construct(const FArguments& InArgs, UMediaPlayer& InMediaPlayer, const TSharedRef<ISlateStyle>& InStyle)
{
	MediaPlayer = &InMediaPlayer;

	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SScrollBox)

				+ SScrollBox::Slot()
					.Padding(4.0f)
					[
						SAssignNew(InfoTextBlock, STextBlock)
					]
			]

		+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBorder)
					.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
					.HAlign(HAlign_Right)
					.Padding(2.0f)
					[
						SNew(SButton)
							.Text(LOCTEXT("CopyClipboardButtonText", "Copy to Clipboard"))
							.ToolTipText(LOCTEXT("CopyClipboardButtonHint", "Copy the media information to the the clipboard"))
							.OnClicked_Lambda([this]() -> FReply {
								FPlatformApplicationMisc::ClipboardCopy(*InfoTextBlock->GetText().ToString());
								return FReply::Handled();
							})
					]
			]
	];

	MediaPlayer->OnMediaEvent().AddSP(this, &SMediaPlayerEditorInfo::HandleMediaPlayerMediaEvent);
}


/* SMediaPlayerEditorInfo callbacks
 *****************************************************************************/

void SMediaPlayerEditorInfo::HandleMediaPlayerMediaEvent(EMediaEvent Event)
{
	static FText NoMediaText = LOCTEXT("NoMediaOpened", "No media opened");

	if ((Event == EMediaEvent::MediaOpened) || (Event == EMediaEvent::MediaOpenFailed) || (Event == EMediaEvent::TracksChanged))
	{
		FMediaPlayerBase& Player = MediaPlayer->GetBasePlayer();

		if (Player.GetUrl().IsEmpty())
		{
			InfoTextBlock->SetText(NoMediaText);
		}
		else
		{
			TRange<float> ThinnedForwardRates = Player.GetForwardRates(false);
			TRange<float> UnthinnedForwardRates = Player.GetForwardRates(true);
			TRange<float> ThinnedReverseRates = Player.GetReverseRates(false);
			TRange<float> UnthinnedReverseRates = Player.GetReverseRates(true);

			FFormatNamedArguments Arguments;
			{
				Arguments.Add(TEXT("PlayerName"), FText::FromName(Player.GetPlayerName()));
				Arguments.Add(TEXT("SupportsScrubbing"), Player.SupportsScrubbing() ? GYes : GNo);
				Arguments.Add(TEXT("SupportsSeeking"), Player.SupportsSeeking() ? GYes : GNo);
				Arguments.Add(TEXT("PlayerInfo"), FText::FromString(Player.GetInfo()));

				Arguments.Add(TEXT("ForwardRateThinned"), ThinnedForwardRates.IsEmpty()
					? LOCTEXT("RateNotSupported", "Not supported")
					: FText::Format(LOCTEXT("RatesFormat", "{0} to {1}"), FText::AsNumber(ThinnedForwardRates.GetLowerBoundValue()), FText::AsNumber(ThinnedForwardRates.GetUpperBoundValue()))
				);

				Arguments.Add(TEXT("ForwardRateUnthinned"), UnthinnedForwardRates.IsEmpty()
					? LOCTEXT("RateNotSupported", "Not supported")
					: FText::Format(LOCTEXT("RatesFormat", "{0} to {1}"), FText::AsNumber(UnthinnedForwardRates.GetLowerBoundValue()), FText::AsNumber(UnthinnedForwardRates.GetUpperBoundValue()))
				);

				Arguments.Add(TEXT("ReverseRateThinned"), ThinnedReverseRates.IsEmpty()
					? LOCTEXT("RateNotSupported", "Not supported")
					: FText::Format(LOCTEXT("RatesFormat", "{0} to {1}"), FText::AsNumber(ThinnedReverseRates.GetLowerBoundValue()), FText::AsNumber(ThinnedReverseRates.GetUpperBoundValue()))
				);

				Arguments.Add(TEXT("ReverseRateUnthinned"), UnthinnedReverseRates.IsEmpty()
					? LOCTEXT("RateNotSupported", "Not supported")
					: FText::Format(LOCTEXT("RatesFormat", "{0} to {1}"), FText::AsNumber(UnthinnedReverseRates.GetLowerBoundValue()), FText::AsNumber(UnthinnedReverseRates.GetUpperBoundValue()))
				);				
			}

			InfoTextBlock->SetText(
				FText::Format(LOCTEXT("InfoFormat",
					"Player: {PlayerName}\n"
					"\n"
					"Forward Rates\n"
					"    Thinned: {ForwardRateThinned}\n"
					"    Unthinned: {ForwardRateUnthinned}\n"
					"\n"
					"Reverse Rates\n"
					"    Thinned: {ReverseRateThinned}\n"
					"    Unthinned: {ReverseRateUnthinned}\n"
					"\n"
					"Capabilities\n"
					"    Scrubbing: {SupportsScrubbing}\n"
					"    Seeking: {SupportsSeeking}\n"
					"\n"
					"{PlayerInfo}"),
					Arguments
				)
			);
		}
	}
	else if (Event == EMediaEvent::MediaClosed)
	{
		InfoTextBlock->SetText(NoMediaText);
	}
}


#undef LOCTEXT_NAMESPACE
