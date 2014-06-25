// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UTextBlock

UTextBlock::UTextBlock(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bIsVariable = false;

	Text = LOCTEXT("TextBlockDefaultValue", "Text Block");
	ShadowOffset = FVector2D(1.0f, 1.0f);
	ColorAndOpacity = FLinearColor::White;
	ShadowColorAndOpacity = FLinearColor::Transparent;

	Style = NULL;

	// @TODO UMG HACK Special font initialization hack since there are no font assets yet for slate.
	Font = FSlateFontInfo(TEXT("Slate/Fonts/Roboto-Bold.ttf"), 24);
}

void UTextBlock::SetColorAndOpacity(FLinearColor InColorAndOpacity)
{
	ColorAndOpacity = InColorAndOpacity;
	if( MyTextBlock.IsValid() )
	{
		MyTextBlock->SetColorAndOpacity( InColorAndOpacity );
	}
}

void UTextBlock::SetShadowColorAndOpacity(FLinearColor InShadowColorAndOpacity)
{
	ShadowColorAndOpacity = InShadowColorAndOpacity;
	if(MyTextBlock.IsValid())
	{
		MyTextBlock->SetShadowColorAndOpacity(InShadowColorAndOpacity);
	}
}

void UTextBlock::SetShadowOffset(FVector2D InShadowOffset)
{
	ShadowOffset = InShadowOffset;
	if(MyTextBlock.IsValid())
	{
		MyTextBlock->SetShadowOffset(ShadowOffset);
	}
}

TSharedRef<SWidget> UTextBlock::RebuildWidget()
{
	MyTextBlock = SNew(STextBlock);

#if WITH_EDITOR
	MyEditorTextBlock = SNew(SInlineEditableTextBlock)
		.OnTextCommitted(BIND_UOBJECT_DELEGATE(FOnTextCommitted, HandleTextCommitted))
		//.IsSelected(InArgs._IsSelected)
		;

	//if ( IsDesignTime() )
	//{
	//	return MyEditorTextBlock.ToSharedRef();
	//}
#endif

	return MyTextBlock.ToSharedRef();
}

void UTextBlock::SyncronizeProperties()
{
	Super::SyncronizeProperties();

	FString FontPath = FPaths::EngineContentDir() / Font.FontName.ToString();

	const FTextBlockStyle* StylePtr = ( Style != NULL ) ? Style->GetStyle<FTextBlockStyle>() : NULL;
	if ( StylePtr == NULL )
	{
		STextBlock::FArguments Defaults;
		StylePtr = Defaults._TextStyle;
	}

	TAttribute<FText> TextBinding = OPTIONAL_BINDING(FText, Text);
	TAttribute<FSlateColor> ColorAndOpacityBinding = OPTIONAL_BINDING(FSlateColor, ColorAndOpacity);
	TAttribute<FLinearColor> ShadowColorAndOpacityBinding = OPTIONAL_BINDING(FLinearColor, ShadowColorAndOpacity);

	MyTextBlock->SetText(TextBinding);
	MyTextBlock->SetFont(FSlateFontInfo(FontPath, Font.Size));
	MyTextBlock->SetColorAndOpacity(ColorAndOpacityBinding);
	MyTextBlock->SetTextStyle(StylePtr);
	MyTextBlock->SetShadowOffset(ShadowOffset);
	MyTextBlock->SetShadowColorAndOpacity(ShadowColorAndOpacityBinding);

#if WITH_EDITOR
	MyEditorTextBlock->SetText(TextBinding);
	//MyEditorTextBlock->SetFont(FSlateFontInfo(FontPath, Font.Size));
	//MyEditorTextBlock->SetColorAndOpacity(ColorAndOpacityBinding);
	//MyEditorTextBlock->SetTextStyle(StylePtr);
	//MyEditorTextBlock->SetShadowOffset(ShadowOffset);
	//MyEditorTextBlock->SetShadowColorAndOpacity(ShadowColorAndOpacityBinding);
#endif
}

#if WITH_EDITOR

FString UTextBlock::GetLabelMetadata() const
{
	const int32 MaxSampleLength = 15;

	FString TextStr = Text.ToString();
	TextStr = TextStr.Len() <= MaxSampleLength ? TextStr : TextStr.Left(MaxSampleLength - 2) + TEXT("..");
	return TEXT(" \"") + TextStr + TEXT("\"");
}

void UTextBlock::HandleTextCommitted(const FText& InText, ETextCommit::Type CommitteType)
{
	//TODO UMG How will this migrate to the template?  Seems to me we need the previews to have access to their templates!
	//TODO UMG How will the user click the editable area?  There is an overlay blocking input so that other widgets don't get them.
	//     Need a way to recognize one particular widget and forward things to them!
}

void UTextBlock::OnDesignerDoubleClicked()
{
	MyEditorTextBlock->EnterEditingMode();
}

const FSlateBrush* UTextBlock::GetEditorIcon()
{
	return FUMGStyle::Get().GetBrush("Widget.TextBlock");
}

#endif

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
