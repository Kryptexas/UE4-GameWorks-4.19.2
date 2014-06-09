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

TSharedRef<SWidget> UTextBlock::RebuildWidget()
{
	MyTextBlock = SNew(STextBlock);
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
}

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
