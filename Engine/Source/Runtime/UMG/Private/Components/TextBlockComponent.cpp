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

	// HACK Special font initialization hack since there are no font assets yet for slate.
	Font = FSlateFontInfo(TEXT("Slate/Fonts/Roboto-Bold.ttf"), 24);
}

TSharedRef<SWidget> UTextBlockComponent::RebuildWidget()
{
	FString FontPath = FPaths::EngineContentDir() / Font.FontName.ToString();

	return SNew(STextBlock)
		.Text( BIND_UOBJECT_ATTRIBUTE(FString, GetText) )
		.Font(FSlateFontInfo(FontPath, Font.Size))
		.ColorAndOpacity( BIND_UOBJECT_ATTRIBUTE(FSlateColor, GetColorAndOpacity) )
		.ShadowOffset(ShadowOffset)
		.ShadowColorAndOpacity(BIND_UOBJECT_ATTRIBUTE(FLinearColor, GetShadowColorAndOpacity));
}

FString UTextBlockComponent::GetText() const
{
	return Text.ToString();
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
