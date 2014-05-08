// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UTextBlockComponent

UTextBlockComponent::UTextBlockComponent(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bIsVariable = false;

	Text = LOCTEXT("TextBlockDefaultValue", "Text Block");
	ShadowOffset = FVector2D(1.0f, 1.0f);
	ColorAndOpacity = FLinearColor::White;
	ShadowColorAndOpacity = FLinearColor::Transparent;

	Style = NULL;

	// HACK Special font initialization hack since there are no font assets yet for slate.
	Font = FSlateFontInfo(TEXT("Slate/Fonts/Roboto-Bold.ttf"), 24);
}

TSharedRef<SWidget> UTextBlockComponent::RebuildWidget()
{
	FString FontPath = FPaths::EngineContentDir() / Font.FontName.ToString();

	const FTextBlockStyle* StylePtr = ( Style != NULL ) ? Style->GetStyle<FTextBlockStyle>() : NULL;
	if ( StylePtr == NULL )
	{
		STextBlock::FArguments Defaults;
		StylePtr = Defaults._TextStyle;
	}

//	TAttribute<FText>::Create(TAttribute<FText>::FGetter::From::CreateUObject(this, &GlueText));

	return SNew(STextBlock)
		.Text(BIND_UOBJECT_ATTRIBUTE(FText, GetText))
		.TextStyle(StylePtr)
		.Font(FSlateFontInfo(FontPath, Font.Size))
		.ColorAndOpacity( BIND_UOBJECT_ATTRIBUTE(FSlateColor, GetColorAndOpacity) )
		.ShadowOffset(ShadowOffset)
		.ShadowColorAndOpacity(BIND_UOBJECT_ATTRIBUTE(FLinearColor, GetShadowColorAndOpacity));
}

FText UTextBlockComponent::GetText() const
{
	if ( TextDelegate.IsBound() )
	{
		return TextDelegate.Execute();
	}

	return Text;
}

FSlateColor UTextBlockComponent::GetColorAndOpacity() const
{
	return ColorAndOpacity;
}

FLinearColor UTextBlockComponent::GetShadowColorAndOpacity() const
{
	return ShadowColorAndOpacity;
}

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
