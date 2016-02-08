// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SequencerPrivatePCH.h"
#include "Sequencer.h"
#include "SequencerSettings.h"
#include "SSequencerGotoBox.h"


#define LOCTEXT_NAMESPACE "Sequencer"


void SSequencerGotoBox::Construct(const FArguments& InArgs, const TSharedRef<FSequencer>& InSequencer, USequencerSettings& InSettings, const TSharedRef<INumericTypeInterface<float>>& InNumericTypeInterface)
{
	WasHidden = true;
	SequencerPtr = InSequencer;
	Settings = &InSettings;
	NumericTypeInterface = InNumericTypeInterface;

	ChildSlot
	[
		SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.DarkGroupBorder"))
			.Padding(6.0f)
			.Visibility_Lambda([this]() -> EVisibility { return Settings->GetShowGotoBox() ? EVisibility::Visible : EVisibility::Collapsed; })
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
							.Text(LOCTEXT("GotoLabel", "Go to:"))
					]

				+ SHorizontalBox::Slot()
					.Padding(6.0f, 0.0f, 0.0f, 0.0f)
					.AutoWidth()
					[
						SAssignNew(EntryBox, SNumericEntryBox<float>)
							.MinDesiredValueWidth(64.0f)
							.OnValueCommitted(this, &SSequencerGotoBox::HandleEntryBoxValueCommitted)
							.TypeInterface(NumericTypeInterface)
							.Value_Lambda([this](){ return SequencerPtr.Pin()->GetGlobalTime(); })
					]
			]
	];
}


void SSequencerGotoBox::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	if (WasHidden)
	{
		FSlateApplication::Get().SetAllUserFocus(EntryBox, EFocusCause::Navigation);
		WasHidden = false;
	}
}


void SSequencerGotoBox::HandleEntryBoxValueCommitted(float Value, ETextCommit::Type CommitType)
{
	Settings->SetShowGotoBox(false);
	WasHidden = true;

	if (CommitType == ETextCommit::OnCleared)
	{
		return;
	}

	TSharedPtr<FSequencer> Sequencer = SequencerPtr.Pin();

	// scroll view range if new value is not visible
	const FAnimatedRange ViewRange = Sequencer->GetViewRange();

	if (!ViewRange.Contains(Value))
	{
		const float HalfViewWidth = 0.5f * ViewRange.Size<float>();
		const TRange<float> NewRange = TRange<float>(Value - HalfViewWidth, Value + HalfViewWidth);
		Sequencer->SetViewRange(NewRange);
	}

	Sequencer->SetGlobalTime(Value);
}


#undef LOCTEXT_NAMESPACE
