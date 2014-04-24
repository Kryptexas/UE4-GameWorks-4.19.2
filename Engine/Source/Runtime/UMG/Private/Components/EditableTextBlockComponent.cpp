// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UEditableTextBlockComponent

UEditableTextBlockComponent::UEditableTextBlockComponent(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	ForegroundColor = FLinearColor::Black;
	BackgroundColor = FLinearColor::White;
	ReadOnlyForegroundColor = FLinearColor::Black;

	// HACK Special font initialization hack since there are no font assets yet for slate.
	Font = FSlateFontInfo(TEXT("Slate/Fonts/Roboto-Bold.ttf"), 12);

	// Grab other defaults from slate arguments.
	SEditableTextBox::FArguments Defaults;
	IsReadOnly = Defaults._IsReadOnly.Get();
	IsPassword = Defaults._IsReadOnly.Get();
	MinimumDesiredWidth = Defaults._MinDesiredWidth.Get();
	Padding = Defaults._Padding.Get();
	IsCaretMovedWhenGainFocus = Defaults._IsCaretMovedWhenGainFocus.Get();
	SelectAllTextWhenFocused = Defaults._SelectAllTextWhenFocused.Get();
	RevertTextOnEscape = Defaults._RevertTextOnEscape.Get();
	ClearKeyboardFocusOnCommit = Defaults._ClearKeyboardFocusOnCommit.Get();
	SelectAllTextOnCommit = Defaults._SelectAllTextOnCommit.Get();
}

TSharedRef<SWidget> UEditableTextBlockComponent::RebuildWidget()
{
	FString FontPath = FPaths::EngineContentDir() / Font.FontName.ToString();

	return SNew(SEditableTextBox)
		.Text(Text)
		.HintText(HintText)
		.Font(FSlateFontInfo(FontPath, Font.Size))
		.ForegroundColor(ForegroundColor)
		.BackgroundColor(BackgroundColor)
		.ReadOnlyForegroundColor(ReadOnlyForegroundColor)
		.IsReadOnly(IsReadOnly)
		.IsPassword(IsPassword)
		.MinDesiredWidth(MinimumDesiredWidth)
		.Padding(Padding)
		.IsCaretMovedWhenGainFocus(IsCaretMovedWhenGainFocus)
		.SelectAllTextWhenFocused(SelectAllTextWhenFocused)
		.RevertTextOnEscape(RevertTextOnEscape)
		.ClearKeyboardFocusOnCommit(ClearKeyboardFocusOnCommit)
		.SelectAllTextOnCommit(SelectAllTextOnCommit);
}

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
