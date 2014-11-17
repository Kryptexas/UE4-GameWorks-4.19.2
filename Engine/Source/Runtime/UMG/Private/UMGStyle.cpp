// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#include "UMGStyle.h"
#include "SlateGameResources.h"

TSharedPtr< FSlateStyleSet > FUMGStyle::UMGStyleInstance = NULL;

void FUMGStyle::Initialize()
{
	if ( !UMGStyleInstance.IsValid() )
	{
		UMGStyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle( *UMGStyleInstance );
	}
}

void FUMGStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle( *UMGStyleInstance );
	ensure( UMGStyleInstance.IsUnique() );
	UMGStyleInstance.Reset();
}

FName FUMGStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("UMGStyle"));
	return StyleSetName;
}

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define TTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".ttf") ), __VA_ARGS__ )
#define OTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".otf") ), __VA_ARGS__ )

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);

TSharedRef< FSlateStyleSet > FUMGStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("UMGStyle"));
	Style->SetContentRoot(FPaths::EngineContentDir() / TEXT("Editor/Slate/UMG"));
	
	Style->Set("Widget", new IMAGE_BRUSH(TEXT("Widget"), Icon16x16));
	Style->Set("Widget.CheckBox", new IMAGE_BRUSH(TEXT("CheckBox"), Icon16x16));
	Style->Set("Widget.Button", new IMAGE_BRUSH(TEXT("Button"), Icon16x16));
	Style->Set("Widget.EditableTextBlock", new IMAGE_BRUSH(TEXT("EditableTextBlock"), Icon16x16));
	Style->Set("Widget.EditableText", new IMAGE_BRUSH(TEXT("EditableText"), Icon16x16));
	Style->Set("Widget.HorizontalBox", new IMAGE_BRUSH(TEXT("HorizontalBox"), Icon16x16));
	Style->Set("Widget.VerticalBox", new IMAGE_BRUSH(TEXT("VerticalBox"), Icon16x16));
	Style->Set("Widget.Image", new IMAGE_BRUSH(TEXT("Image"), Icon16x16));
	Style->Set("Widget.Canvas", new IMAGE_BRUSH(TEXT("Canvas"), Icon16x16));
	Style->Set("Widget.TextBlock", new IMAGE_BRUSH(TEXT("TextBlock"), Icon16x16));
	Style->Set("Widget.Border", new IMAGE_BRUSH(TEXT("Border"), Icon16x16));
	Style->Set("Widget.Slider", new IMAGE_BRUSH(TEXT("Slider"), Icon16x16));
	Style->Set("Widget.Spacer", new IMAGE_BRUSH(TEXT("Spacer"), Icon16x16));
	Style->Set("Widget.ScrollBox", new IMAGE_BRUSH(TEXT("ScrollBox"), Icon16x16));
	Style->Set("Widget.ProgressBar", new IMAGE_BRUSH(TEXT("ProgressBar"), Icon16x16));

	Style->Set("Widget.UserWidget", new IMAGE_BRUSH(TEXT("UserWidget"), Icon16x16));

	return Style;
}

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef TTF_FONT
#undef OTF_FONT

void FUMGStyle::ReloadTextures()
{
	FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
}

const ISlateStyle& FUMGStyle::Get()
{
	return *UMGStyleInstance;
}
