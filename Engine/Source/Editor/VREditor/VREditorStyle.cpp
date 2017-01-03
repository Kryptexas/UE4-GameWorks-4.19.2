// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "VREditorStyle.h"
#include "SlateTypes.h"
#include "EditorStyleSet.h"
#include "Styling/SlateStyleRegistry.h"
#include "SUniformGridPanel.h"
#include "Brushes/SlateImageBrush.h"
#include "Brushes/SlateBoxBrush.h"
#include "Brushes/SlateBorderBrush.h"
#include "Fonts/SlateFontInfo.h"
#include "Framework/Application/SlateApplication.h"

TSharedPtr< FSlateStyleSet > FVREditorStyle::VREditorStyleInstance = NULL;

void FVREditorStyle::Initialize()
{
	if ( !VREditorStyleInstance.IsValid() )
	{
		VREditorStyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle( *VREditorStyleInstance);
	}
}

void FVREditorStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle( *VREditorStyleInstance);
	ensure(VREditorStyleInstance.IsUnique() );
	VREditorStyleInstance.Reset();
}

FName FVREditorStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("VREditorStyle"));
	return StyleSetName;
}

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define TTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".ttf") ), __VA_ARGS__ )
#define OTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".otf") ), __VA_ARGS__ )
#define TTF_CORE_FONT(RelativePath, ...) FSlateFontInfo(Style->RootToCoreContentDir(RelativePath, TEXT(".ttf") ), __VA_ARGS__)

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);
const FVector2D Icon40x40(40.0f, 40.0f);

TSharedRef< FSlateStyleSet > FVREditorStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet(FVREditorStyle::GetStyleSetName()));
	// Use the default menu button style, but set the background to dark grey.
	const FButtonStyle NormalButton = FEditorStyle::GetWidgetStyle<FButtonStyle>("Menu.Button");
	Style->Set("VREditorStyle.Button", FButtonStyle(NormalButton)
		.SetNormal(BOX_BRUSH("Common/RoundedSelection_16x", 4.0f / 16.0f, FLinearColor(0.1f,0.1f,0.1f))));

	const FTextBlockStyle NormalText = FEditorStyle::GetWidgetStyle<FTextBlockStyle>("NormalText");
	Style->Set("VREditorStyle.Label", FTextBlockStyle(NormalText)
		.SetFont(TTF_CORE_FONT("Slate/Fonts/Roboto-Regular", 9)));

	// Headings will have a font outline
	FFontOutlineSettings HeadingOutline;
	HeadingOutline.OutlineColor = FLinearColor(0.2f, 0.2f, 0.2f, 0.5f);
	HeadingOutline.OutlineSize = 2.0f;
	FSlateFontInfo HeadlineFont = TTF_CORE_FONT("Slate/Fonts/Roboto-Regular", 18);
	HeadlineFont.OutlineSettings = HeadingOutline;

	Style->Set("VREditorStyle.Heading", FTextBlockStyle(NormalText)
			.SetFont(HeadlineFont)
			.SetColorAndOpacity(FLinearColor::White));

	// Headings will have a font outline
	FFontOutlineSettings HelperOutline;
	HelperOutline.OutlineColor = FLinearColor( 0.2f, 0.2f, 0.2f, 0.5f );
	HelperOutline.OutlineSize = 3.0f;
	FSlateFontInfo HelperFont = TTF_CORE_FONT( "Slate/Fonts/Roboto-Regular", 24 );
	HelperFont.OutlineSettings = HelperOutline;

	Style->Set( "VREditorStyle.HelperText", FTextBlockStyle( NormalText )
		.SetFont( HelperFont )
		.SetColorAndOpacity( FLinearColor::White ) );
	
	const FCheckBoxStyle CheckStyle = FEditorStyle::GetWidgetStyle<FCheckBoxStyle>("Menu.Check");
	Style->Set("VREditorStyle.Check", FCheckBoxStyle(CheckStyle));

	const FCheckBoxStyle CheckboxStyle = FEditorStyle::GetWidgetStyle<FCheckBoxStyle>("Menu.CheckBox");
	Style->Set("VREditorStyle.CheckBox", FCheckBoxStyle(CheckboxStyle));

	const FCheckBoxStyle RadioButtonStyle = FEditorStyle::GetWidgetStyle<FCheckBoxStyle>("Menu.RadioButton");
	Style->Set("VREditorStyle.RadioButton", FCheckBoxStyle(RadioButtonStyle));

	const FTextBlockStyle KeybindingStyle = FEditorStyle::GetWidgetStyle<FTextBlockStyle>("Menu.Keybinding");
	Style->Set("VREditorStyle.Keybinding", FTextBlockStyle(KeybindingStyle));

	const FTextBlockStyle LabelStyle = FEditorStyle::GetWidgetStyle<FTextBlockStyle>("Menu.Label");
	Style->Set("VREditorStyle.Label", FTextBlockStyle(LabelStyle));

	return Style;
}

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef TTF_FONT
#undef OTF_FONT
#undef TTF_CORE_FONT

void FVREditorStyle::ReloadTextures()
{
	FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
}

const ISlateStyle& FVREditorStyle::Get()
{
	return *VREditorStyleInstance;
}
