// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UEditableText

UEditableText::UEditableText(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	ColorAndOpacity = FLinearColor::Black;

	// HACK Special font initialization hack since there are no font assets yet for slate.
	Font = FSlateFontInfo(TEXT("Slate/Fonts/Roboto-Bold.ttf"), 12);

	// Grab other defaults from slate arguments.
	SEditableTextBox::FArguments Defaults;
	IsReadOnly = Defaults._IsReadOnly.Get();
	IsPassword = Defaults._IsReadOnly.Get();
	MinimumDesiredWidth = Defaults._MinDesiredWidth.Get();
	IsCaretMovedWhenGainFocus = Defaults._IsCaretMovedWhenGainFocus.Get();
	SelectAllTextWhenFocused = Defaults._SelectAllTextWhenFocused.Get();
	RevertTextOnEscape = Defaults._RevertTextOnEscape.Get();
	ClearKeyboardFocusOnCommit = Defaults._ClearKeyboardFocusOnCommit.Get();
	SelectAllTextOnCommit = Defaults._SelectAllTextOnCommit.Get();
}

TSharedRef<SWidget> UEditableText::RebuildWidget()
{
	FString FontPath = FPaths::EngineContentDir() / Font.FontName.ToString();

	SEditableText::FArguments Defaults;

	return SNew(SEditableText)
		.Text(Text)
		//.Style(Style ? Style->TextBlockStyle : Defaults._Style)
		.HintText(HintText)
		.Font(FSlateFontInfo(FontPath, Font.Size))
		.ColorAndOpacity(ColorAndOpacity)
		.IsReadOnly(IsReadOnly)
		.IsPassword(IsPassword)
		.MinDesiredWidth(MinimumDesiredWidth)
		.IsCaretMovedWhenGainFocus(IsCaretMovedWhenGainFocus)
		.SelectAllTextWhenFocused(SelectAllTextWhenFocused)
		.RevertTextOnEscape(RevertTextOnEscape)
		.ClearKeyboardFocusOnCommit(ClearKeyboardFocusOnCommit)
		.SelectAllTextOnCommit(SelectAllTextOnCommit)
		.OnTextChanged(BIND_UOBJECT_DELEGATE(FOnTextChanged, HandleOnTextChanged))
		.OnTextCommitted(BIND_UOBJECT_DELEGATE(FOnTextCommitted, HandleOnTextCommitted))
		;
}

#if WITH_EDITOR

void UEditableText::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	
}

#endif

void UEditableText::HandleOnTextChanged(const FText& Text)
{
	OnTextChanged.Broadcast(Text);
}

void UEditableText::HandleOnTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	OnTextCommitted.Broadcast(Text, CommitMethod);
}

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
