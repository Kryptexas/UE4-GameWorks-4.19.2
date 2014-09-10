// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UEditableText

UEditableText::UEditableText(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	static const FName StyleName(TEXT("Style"));
	WidgetStyle = PCIP.CreateDefaultSubobject<UEditableTextWidgetStyle>(this, StyleName);

	SEditableText::FArguments Defaults;
	WidgetStyle->EditableTextStyle = *Defaults._Style;

	ColorAndOpacity = FLinearColor::Black;

	// HACK Special font initialization hack since there are no font assets yet for slate.
	Font = FSlateFontInfo(TEXT("Slate/Fonts/Roboto-Bold.ttf"), 12);

	// Grab other defaults from slate arguments.
	IsReadOnly = Defaults._IsReadOnly.Get();
	IsPassword = Defaults._IsReadOnly.Get();
	MinimumDesiredWidth = Defaults._MinDesiredWidth.Get();
	IsCaretMovedWhenGainFocus = Defaults._IsCaretMovedWhenGainFocus.Get();
	SelectAllTextWhenFocused = Defaults._SelectAllTextWhenFocused.Get();
	RevertTextOnEscape = Defaults._RevertTextOnEscape.Get();
	ClearKeyboardFocusOnCommit = Defaults._ClearKeyboardFocusOnCommit.Get();
	SelectAllTextOnCommit = Defaults._SelectAllTextOnCommit.Get();
}

void UEditableText::ReleaseNativeWidget()
{
	Super::ReleaseNativeWidget();

	MyEditableText.Reset();
}

TSharedRef<SWidget> UEditableText::RebuildWidget()
{
	FString FontPath = FPaths::GameContentDir() / Font.FontName.ToString();

	if ( !FPaths::FileExists(FontPath) )
	{
		FontPath = FPaths::EngineContentDir() / Font.FontName.ToString();
	}
	
	MyEditableText = SNew(SEditableText)
	.Style(&WidgetStyle->EditableTextStyle)
	.Font(FSlateFontInfo(FontPath, Font.Size))
	.MinDesiredWidth(MinimumDesiredWidth)
	.IsCaretMovedWhenGainFocus(IsCaretMovedWhenGainFocus)
	.SelectAllTextWhenFocused(SelectAllTextWhenFocused)
	.RevertTextOnEscape(RevertTextOnEscape)
	.ClearKeyboardFocusOnCommit(ClearKeyboardFocusOnCommit)
	.SelectAllTextOnCommit(SelectAllTextOnCommit)
	.OnTextChanged(BIND_UOBJECT_DELEGATE(FOnTextChanged, HandleOnTextChanged))
	.OnTextCommitted(BIND_UOBJECT_DELEGATE(FOnTextCommitted, HandleOnTextCommitted))
	;
	
	return BuildDesignTimeWidget( MyEditableText.ToSharedRef() );
}

void UEditableText::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	TAttribute<FText> TextBinding = OPTIONAL_BINDING(FText, Text);
	TAttribute<FText> HintTextBinding = OPTIONAL_BINDING(FText, HintText);

	MyEditableText->SetText(TextBinding);
	MyEditableText->SetHintText(HintTextBinding);
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

void UEditableText::SetIsPassword(bool InbIsPassword)
{
	IsPassword = InbIsPassword;
	if ( MyEditableText.IsValid() )
	{
		MyEditableText->SetIsPassword(IsPassword);
	}
}

void UEditableText::SetHintText(FText InHintText)
{
	HintText = InHintText;
	if ( MyEditableText.IsValid() )
	{
		MyEditableText->SetHintText(HintText);
	}
}

void UEditableText::SetIsReadOnly(bool InbIsReadyOnly)
{
	IsReadOnly = InbIsReadyOnly;
	if ( MyEditableText.IsValid() )
	{
		MyEditableText->SetIsReadOnly(IsReadOnly);
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

void UEditableText::PostLoad()
{
	Super::PostLoad();

	if ( GetLinkerUE4Version() < VER_UE4_DEPRECATE_UMG_STYLE_ASSETS )
	{
		if ( Style_DEPRECATED != nullptr )
		{
			const FEditableTextStyle* StylePtr = Style_DEPRECATED->GetStyle<FEditableTextStyle>();
			if ( StylePtr != nullptr )
			{
				WidgetStyle->EditableTextStyle = *StylePtr;
			}

			Style_DEPRECATED = nullptr;
		}

		if ( BackgroundImageSelected_DEPRECATED != nullptr )
		{
			WidgetStyle->EditableTextStyle.BackgroundImageSelected = BackgroundImageSelected_DEPRECATED->Brush;
			BackgroundImageSelected_DEPRECATED = nullptr;
		}

		if ( BackgroundImageSelectionTarget_DEPRECATED != nullptr )
		{
			WidgetStyle->EditableTextStyle.BackgroundImageSelectionTarget = BackgroundImageSelectionTarget_DEPRECATED->Brush;
			BackgroundImageSelectionTarget_DEPRECATED = nullptr;
		}

		if ( BackgroundImageComposing_DEPRECATED != nullptr )
		{
			WidgetStyle->EditableTextStyle.BackgroundImageComposing = BackgroundImageComposing_DEPRECATED->Brush;
			BackgroundImageComposing_DEPRECATED = nullptr;
		}

		if ( CaretImage_DEPRECATED != nullptr )
		{
			WidgetStyle->EditableTextStyle.CaretImage = CaretImage_DEPRECATED->Brush;
			CaretImage_DEPRECATED = nullptr;
		}
	}
}

#if WITH_EDITOR

const FSlateBrush* UEditableText::GetEditorIcon()
{
	return FUMGStyle::Get().GetBrush("Widget.EditableText");
}

const FText UEditableText::GetToolboxCategory()
{
	return LOCTEXT("Primitive", "Primitive");
}

#endif

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
