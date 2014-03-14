// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlateComponentWrappersPrivatePCH.h"

#define LOCTEXT_NAMESPACE "SlateComponentWrappers"

/////////////////////////////////////////////////////
// UTextBlockComponent

UTextBlockComponent::UTextBlockComponent(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	DisplayText = LOCTEXT("TextBlockDefaultValue", "Hello World");
	ColorAndOpacity = FLinearColor::White;
	ShadowColorAndOpacity = FLinearColor::Transparent;
}

TSharedRef<SWidget> UTextBlockComponent::RebuildWidget()
{
	return SNew(STextBlock)
		.Text( BIND_UOBJECT_ATTRIBUTE(FString, GetDisplayText) )
		.Font( FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 24) )
		.ColorAndOpacity( BIND_UOBJECT_ATTRIBUTE(FSlateColor, GetColorAndOpacity) )
		.ShadowOffset(FVector2D(1.0f, 1.0f))
		.ShadowColorAndOpacity( BIND_UOBJECT_ATTRIBUTE(FLinearColor, GetShadowColorAndOpacity) );
}

FString UTextBlockComponent::GetDisplayText() const
{
	return DisplayText.ToString();
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
