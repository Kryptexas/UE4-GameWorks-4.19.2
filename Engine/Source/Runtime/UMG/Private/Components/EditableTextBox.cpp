// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UEditableTextBox

UEditableTextBox::UEditableTextBox(const FPostConstructInitializeProperties& PCIP)
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

	WidgetStyle = *Defaults._Style;
}

void UEditableTextBox::ReleaseNativeWidget()
{
	Super::ReleaseNativeWidget();

	MyEditableTextBlock.Reset();
}

TSharedRef<SWidget> UEditableTextBox::RebuildWidget()
{
	FString FontPath = FPaths::GameContentDir() / Font.FontName.ToString();

	if ( !FPaths::FileExists(FontPath) )
	{
		FontPath = FPaths::EngineContentDir() / Font.FontName.ToString();
	}

	MyEditableTextBlock = SNew(SEditableTextBox)
		.Style(&WidgetStyle)
		.Font(FSlateFontInfo(FontPath, Font.Size))
		.ForegroundColor(ForegroundColor)
		.BackgroundColor(BackgroundColor)
		.ReadOnlyForegroundColor(ReadOnlyForegroundColor)
		.MinDesiredWidth(MinimumDesiredWidth)
		.Padding(Padding)
		.IsCaretMovedWhenGainFocus(IsCaretMovedWhenGainFocus)
		.SelectAllTextWhenFocused(SelectAllTextWhenFocused)
		.RevertTextOnEscape(RevertTextOnEscape)
		.ClearKeyboardFocusOnCommit(ClearKeyboardFocusOnCommit)
		.SelectAllTextOnCommit(SelectAllTextOnCommit)
		.OnTextChanged(BIND_UOBJECT_DELEGATE(FOnTextChanged, HandleOnTextChanged))
		.OnTextCommitted(BIND_UOBJECT_DELEGATE(FOnTextCommitted, HandleOnTextCommitted))
		;

	return MyEditableTextBlock.ToSharedRef();
}

void UEditableTextBox::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	TAttribute<FText> TextBinding = OPTIONAL_BINDING(FText, Text);
	TAttribute<FText> HintTextBinding = OPTIONAL_BINDING(FText, HintText);

	MyEditableTextBlock->SetText(TextBinding);
	MyEditableTextBlock->SetHintText(HintTextBinding);
	MyEditableTextBlock->SetIsReadOnly(IsReadOnly);
	MyEditableTextBlock->SetIsPassword(IsPassword);
//	MyEditableTextBlock->SetColorAndOpacity(ColorAndOpacity);

	// TODO UMG Complete making all properties settable on SEditableTextBox
}

FText UEditableTextBox::GetText() const
{
	if ( MyEditableTextBlock.IsValid() )
	{
		return MyEditableTextBlock->GetText();
	}

	return Text;
}

void UEditableTextBox::SetText(FText InText)
{
	Text = InText;
	if ( MyEditableTextBlock.IsValid() )
	{
		MyEditableTextBlock->SetText(Text);
	}
}

void UEditableTextBox::SetError(FText InError)
{
	if ( MyEditableTextBlock.IsValid() )
	{
		MyEditableTextBlock->SetError(InError);
	}
}

void UEditableTextBox::ClearError()
{
	if ( MyEditableTextBlock.IsValid() )
	{
		MyEditableTextBlock->SetError(FText::GetEmpty());
	}
}

void UEditableTextBox::HandleOnTextChanged(const FText& Text)
{
	OnTextChanged.Broadcast(Text);
}

void UEditableTextBox::HandleOnTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	OnTextCommitted.Broadcast(Text, CommitMethod);
}

void UEditableTextBox::PostLoad()
{
	Super::PostLoad();

	if ( GetLinkerUE4Version() < VER_UE4_DEPRECATE_UMG_STYLE_ASSETS )
	{
		if ( Style_DEPRECATED != nullptr )
		{
			const FEditableTextBoxStyle* StylePtr = Style_DEPRECATED->GetStyle<FEditableTextBoxStyle>();
			if ( StylePtr != nullptr )
			{
				WidgetStyle = *StylePtr;
			}

			Style_DEPRECATED = nullptr;
		}
	}
}

#if WITH_EDITOR

const FSlateBrush* UEditableTextBox::GetEditorIcon()
{
	return FUMGStyle::Get().GetBrush("Widget.EditableTextBox");
}

const FText UEditableTextBox::GetPaletteCategory()
{
	return LOCTEXT("Common", "Common");
}

#endif

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
