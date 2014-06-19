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
	
	const FEditableTextStyle* StylePtr = ( Style != NULL ) ? Style->GetStyle<FEditableTextStyle>() : NULL;
	if ( StylePtr == NULL )
	{
		StylePtr = Defaults._Style;
	}
	
	MyEditableText = SNew(SEditableText)
	.Style(StylePtr)
	.Font(FSlateFontInfo(FontPath, Font.Size))
	.MinDesiredWidth(MinimumDesiredWidth)
	.IsCaretMovedWhenGainFocus(IsCaretMovedWhenGainFocus)
	.SelectAllTextWhenFocused(SelectAllTextWhenFocused)
	.RevertTextOnEscape(RevertTextOnEscape)
	.ClearKeyboardFocusOnCommit(ClearKeyboardFocusOnCommit)
	.SelectAllTextOnCommit(SelectAllTextOnCommit)
	.BackgroundImageSelected(BackgroundImageSelected ? TAttribute<const FSlateBrush*>(&BackgroundImageSelected->Brush) : TAttribute<const FSlateBrush*>())
	.BackgroundImageSelectionTarget(BackgroundImageSelectionTarget ? TAttribute<const FSlateBrush*>(&BackgroundImageSelectionTarget->Brush) : TAttribute<const FSlateBrush*>())
	.BackgroundImageComposing(BackgroundImageComposing ? TAttribute<const FSlateBrush*>(&BackgroundImageComposing->Brush) : TAttribute<const FSlateBrush*>())
	.CaretImage(CaretImage ? TAttribute<const FSlateBrush*>(&CaretImage->Brush) : TAttribute<const FSlateBrush*>())
	.OnTextChanged(BIND_UOBJECT_DELEGATE(FOnTextChanged, HandleOnTextChanged))
	.OnTextCommitted(BIND_UOBJECT_DELEGATE(FOnTextCommitted, HandleOnTextCommitted))
	;
	
	return BuildDesignTimeWidget( MyEditableText.ToSharedRef() );
}

void UEditableText::SyncronizeProperties()
{
	Super::SyncronizeProperties();

	MyEditableText->SetText(Text);
	MyEditableText->SetHintText(HintText);
	MyEditableText->SetIsReadOnly(IsReadOnly);
	MyEditableText->SetIsPassword(IsPassword);
	MyEditableText->SetColorAndOpacity(ColorAndOpacity);

	// TODO UMG Complete making all properties settable on SEditableText
}

FText UEditableText::GetText() const
{
	if ( MyEditableText.IsValid() )
	{
		return MyEditableText->GetText();
	}

	return Text;
}

void UEditableText::SetText(FText InText)
{
	Text = InText;
	if ( MyEditableText.IsValid() )
	{
		MyEditableText->SetText(Text);
	}
}

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
