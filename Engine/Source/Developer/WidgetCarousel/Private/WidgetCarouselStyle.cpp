// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "WidgetCarouselPrivatePCH.h"

#include "EditorStyle.h"

TSharedPtr< FSlateStyleSet > FWidgetCarouselStyle::WidgetCarouselStyleInstance = NULL;

void FWidgetCarouselStyle::Initialize()
{
	if ( !WidgetCarouselStyleInstance.IsValid() )
	{
		WidgetCarouselStyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle( *WidgetCarouselStyleInstance );
	}
}

void FWidgetCarouselStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle( *WidgetCarouselStyleInstance );
	ensure( WidgetCarouselStyleInstance.IsUnique() );
	WidgetCarouselStyleInstance.Reset();
}

FName FWidgetCarouselStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("WidgetCarouselStyle"));
	return StyleSetName;
}

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define TTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".ttf") ), __VA_ARGS__ )
#define OTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".otf") ), __VA_ARGS__ )

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);
const FVector2D Icon40x40(40.0f, 40.0f);

TSharedRef< FSlateStyleSet > FWidgetCarouselStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("WidgetCarouselStyle"));
	Style->SetContentRoot(FPaths::EngineContentDir() / TEXT("Editor/Slate/WidgetCarousel"));

	const FButtonStyle DefaultButton = FButtonStyle()
		.SetNormalPadding(0)
		.SetPressedPadding(0);

	FLinearColor PrimaryCallToActionColor = FLinearColor(1.0, 0.7372, 0.05637);
	FLinearColor PrimaryCallToActionColor_Hovered = FLinearColor(1.0, 0.83553, 0.28445);
	FLinearColor PrimaryCallToActionColor_Pressed = FLinearColor(1.0, 0.66612, 0.0012);

	Style->Set("PrimaryColor", PrimaryCallToActionColor);

	Style->Set("WidgetCarousel.NavigationButtonLeft", new IMAGE_BRUSH("Arrow-Left", FVector2D(25.0f, 42.0f)));

	Style->Set("WidgetCarousel.NavigationButtonRight", new IMAGE_BRUSH("Arrow-Right", FVector2D(25.0f, 42.0f)));

	Style->Set("WidgetCarousel.NavigationButton", FButtonStyle(DefaultButton)
		.SetNormal(BOX_BRUSH("WhiteBox_7px_CornerRadius", FVector2D(17, 17), FMargin(0.5), PrimaryCallToActionColor))
		.SetPressed(BOX_BRUSH("WhiteBox_7px_CornerRadius", FVector2D(17, 17), FMargin(0.5), PrimaryCallToActionColor_Pressed))
		.SetHovered(BOX_BRUSH("WhiteBox_7px_CornerRadius", FVector2D(17, 17), FMargin(0.5), PrimaryCallToActionColor_Hovered)));

	Style->Set("WidgetCarousel.NavigationScrollBarHighlight", new IMAGE_BRUSH("CarouselNavMarker", FVector2D(80.0f, 20.0f), PrimaryCallToActionColor));

	Style->Set("WidgetCarousel.NavigationScrollBarLeftButton", FButtonStyle(DefaultButton)
		.SetNormal(IMAGE_BRUSH("CarouselNavLeft", FVector2D(80.0f, 20.0f), FLinearColor::White))
		.SetHovered(IMAGE_BRUSH("CarouselNavLeft", FVector2D(80.0f, 20.0f), PrimaryCallToActionColor))
		.SetPressed(IMAGE_BRUSH("CarouselNavLeft", FVector2D(80.0f, 20.0f), PrimaryCallToActionColor_Pressed)));

	Style->Set("WidgetCarousel.NavigationScrollBarCenterButton", FButtonStyle(DefaultButton)
		.SetNormal(IMAGE_BRUSH("CarouselNavCenter", FVector2D(80.0f, 20.0f), FLinearColor::White))
		.SetHovered(IMAGE_BRUSH("CarouselNavCenter", FVector2D(80.0f, 20.0f), PrimaryCallToActionColor))
		.SetPressed(IMAGE_BRUSH("CarouselNavCenter", FVector2D(80.0f, 20.0f), PrimaryCallToActionColor_Pressed)));

	Style->Set("WidgetCarousel.NavigationScrollBarRightButton", FButtonStyle(DefaultButton)
		.SetNormal(IMAGE_BRUSH("CarouselNavRight", FVector2D(80.0f, 20.0f), FLinearColor::White))
		.SetHovered(IMAGE_BRUSH("CarouselNavRight", FVector2D(80.0f, 20.0f), PrimaryCallToActionColor))
		.SetPressed(IMAGE_BRUSH("CarouselNavRight", FVector2D(80.0f, 20.0f), PrimaryCallToActionColor_Pressed)));

	Style->Set("Black .6", FLinearColor(0, 0, 0, 0.6f));

	return Style;
}

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef TTF_FONT
#undef OTF_FONT

void FWidgetCarouselStyle::ReloadTextures()
{
	FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
}

const ISlateStyle& FWidgetCarouselStyle::Get()
{
	return *WidgetCarouselStyleInstance;
}
