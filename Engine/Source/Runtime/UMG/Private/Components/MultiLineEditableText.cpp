// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UMultiLineEditableText

UMultiLineEditableText::UMultiLineEditableText(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	// HACK Special font initialization hack since there are no font assets yet for slate.
	Font = FSlateFontInfo(TEXT("Slate/Fonts/Roboto-Bold.ttf"), 12);
}

TSharedRef<SWidget> UMultiLineEditableText::RebuildWidget()
{
	FString FontPath = FPaths::GameContentDir() / Font.FontName.ToString();

	if ( !FPaths::FileExists(FontPath) )
	{
		FontPath = FPaths::EngineContentDir() / Font.FontName.ToString();
	}
	
	SMultiLineEditableText::FArguments Defaults;
	
	//const FMultiLineEditableTextStyle* StylePtr = ( Style != NULL ) ? Style->GetStyle<FMultiLineEditableTextStyle>() : NULL;
	//if ( StylePtr == NULL )
	//{
	//	StylePtr = Defaults._Style;
	//}
	
	MyMultiLineEditableText = SNew(SMultiLineEditableText)
	//.Style(StylePtr)
	.Font(FSlateFontInfo(FontPath, Font.Size))
	.Justification(Justification)
//	.MinDesiredWidth(MinimumDesiredWidth)
//	.IsCaretMovedWhenGainFocus(IsCaretMovedWhenGainFocus)
//	.SelectAllTextWhenFocused(SelectAllTextWhenFocused)
//	.RevertTextOnEscape(RevertTextOnEscape)
//	.ClearKeyboardFocusOnCommit(ClearKeyboardFocusOnCommit)
//	.SelectAllTextOnCommit(SelectAllTextOnCommit)
//	.BackgroundImageSelected(BackgroundImageSelected ? TAttribute<const FSlateBrush*>(&BackgroundImageSelected->Brush) : TAttribute<const FSlateBrush*>())
//	.BackgroundImageSelectionTarget(BackgroundImageSelectionTarget ? TAttribute<const FSlateBrush*>(&BackgroundImageSelectionTarget->Brush) : TAttribute<const FSlateBrush*>())
//	.BackgroundImageComposing(BackgroundImageComposing ? TAttribute<const FSlateBrush*>(&BackgroundImageComposing->Brush) : TAttribute<const FSlateBrush*>())
//	.CaretImage(CaretImage ? TAttribute<const FSlateBrush*>(&CaretImage->Brush) : TAttribute<const FSlateBrush*>())
//	.OnTextChanged(BIND_UOBJECT_DELEGATE(FOnTextChanged, HandleOnTextChanged))
//	.OnTextCommitted(BIND_UOBJECT_DELEGATE(FOnTextCommitted, HandleOnTextCommitted))
	;
	
	return BuildDesignTimeWidget( MyMultiLineEditableText.ToSharedRef() );
}

void UMultiLineEditableText::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	MyMultiLineEditableText->SetText(Text);
//	MyMultiLineEditableText->SetHintText(HintText);
//	MyMultiLineEditableText->SetIsReadOnly(IsReadOnly);
//	MyMultiLineEditableText->SetIsPassword(IsPassword);
//	MyMultiLineEditableText->SetColorAndOpacity(ColorAndOpacity);

	// TODO UMG Complete making all properties settable on SMultiLineEditableText
}

FText UMultiLineEditableText::GetText() const
{
	if ( MyMultiLineEditableText.IsValid() )
	{
		return MyMultiLineEditableText->GetText();
	}

	return Text;
}

void UMultiLineEditableText::SetText(FText InText)
{
	Text = InText;
	if ( MyMultiLineEditableText.IsValid() )
	{
		MyMultiLineEditableText->SetText(Text);
	}
}

void UMultiLineEditableText::HandleOnTextChanged(const FText& Text)
{
	OnTextChanged.Broadcast(Text);
}

void UMultiLineEditableText::HandleOnTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	OnTextCommitted.Broadcast(Text, CommitMethod);
}

#if WITH_EDITOR

const FSlateBrush* UMultiLineEditableText::GetEditorIcon()
{
	return FUMGStyle::Get().GetBrush("Widget.MultiLineEditableText");
}

const FText UMultiLineEditableText::GetToolboxCategory()
{
	return LOCTEXT("Primitive", "Primitive");
}

#endif

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
