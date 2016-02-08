// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SequencerPrivatePCH.h"
#include "SequencerUtilities.h"

static EVisibility GetRolloverVisibility(TAttribute<bool> HoverState, TWeakPtr<SComboButton> WeakComboButton)
{
	TSharedPtr<SComboButton> ComboButton = WeakComboButton.Pin();
	if (HoverState.Get() || ComboButton->IsOpen())
	{
		return EVisibility::SelfHitTestInvisible;
	}
	else
	{
		return EVisibility::Collapsed;
	}
}

TSharedRef<SWidget> FSequencerUtilities::MakeAddButton(FText HoverText, FOnGetContent MenuContent, const TAttribute<bool>& HoverState)
{
	FSlateFontInfo SmallLayoutFont( FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 8 );

	TSharedRef<STextBlock> ComboButtonText = SNew(STextBlock)
		.Text(HoverText)
		.Font(SmallLayoutFont)
		.ColorAndOpacity( FSlateColor::UseForeground() );

	TSharedRef<SComboButton> ComboButton =

		SNew(SComboButton)
		.HasDownArrow(false)
		.ButtonStyle(FEditorStyle::Get(), "HoverHintOnly")
		.ForegroundColor( FSlateColor::UseForeground() )
		.OnGetMenuContent(MenuContent)
		.ContentPadding(FMargin(5, 2))
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.ButtonContent()
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(0,0,2,0))
			[
				SNew(SImage)
				.ColorAndOpacity( FSlateColor::UseForeground() )
				.Image(FEditorStyle::GetBrush("Plus"))
			]

			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				ComboButtonText
			]
		];

	TAttribute<EVisibility> Visibility = TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateStatic(GetRolloverVisibility, HoverState, TWeakPtr<SComboButton>(ComboButton)));
	ComboButtonText->SetVisibility(Visibility);

	return ComboButton;
}

