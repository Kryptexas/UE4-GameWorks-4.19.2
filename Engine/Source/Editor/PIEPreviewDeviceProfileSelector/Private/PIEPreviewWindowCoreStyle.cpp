// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "PIEPreviewWindowCoreStyle.h"
#include "Styling/CoreStyle.h"
#include "PIEPreviewWindowStyle.h"
#include "SlateGlobals.h"
#include "Brushes/SlateBorderBrush.h"
#include "Brushes/SlateBoxBrush.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateImageBrush.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateTypes.h"

#if WITH_EDITOR

TSharedPtr< ISlateStyle > FPIEPreviewWindowCoreStyle::Instance = nullptr;

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush(Style->RootToContentDir(RelativePath, TEXT(".png")), __VA_ARGS__)

TSharedRef<class ISlateStyle> FPIEPreviewWindowCoreStyle::Create(const FName& InStyleSetName)
{

	TSharedRef<FSlateStyleSet> Style = MakeShareable(new FSlateStyleSet(InStyleSetName));
	Style->SetContentRoot(FPaths::EngineContentDir() / TEXT("Slate"));

	const FButtonStyle Button = FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button");

	const FButtonStyle ScreenRotationButtonStyle = FButtonStyle(Button)
		.SetNormal(IMAGE_BRUSH("Icons/PIEWindow/WindowButton_Screen_Rotation_Normal", FVector2D(23.0f, 18.0f)))
		.SetHovered(IMAGE_BRUSH("Icons/PIEWindow/WindowButton_Screen_Rotation_Hovered", FVector2D(23.0f, 18.0f)))
		.SetPressed(IMAGE_BRUSH("Icons/PIEWindow/WindowButton_Screen_Rotation_Pressed", FVector2D(23.0f, 18.0f)));

	const FButtonStyle QuarterMobileContentScaleFactorButtonStyle = FButtonStyle(Button)
		.SetNormal(IMAGE_BRUSH("Icons/PIEWindow/WindowButton_025x_Normal", FVector2D(23.0f, 18.0f)))
		.SetHovered(IMAGE_BRUSH("Icons/PIEWindow/WindowButton_025x_Hovered", FVector2D(23.0f, 18.0f)))
		.SetPressed(IMAGE_BRUSH("Icons/PIEWindow/WindowButton_025x_Pressed", FVector2D(23.0f, 18.0f)));

	const FButtonStyle HalfMobileContentScaleFactorButtonStyle = FButtonStyle(Button)
		.SetNormal(IMAGE_BRUSH("Icons/PIEWindow/WindowButton_05x_Normal", FVector2D(23.0f, 18.0f)))
		.SetHovered(IMAGE_BRUSH("Icons/PIEWindow/WindowButton_05x_Hovered", FVector2D(23.0f, 18.0f)))
		.SetPressed(IMAGE_BRUSH("Icons/PIEWindow/WindowButton_05x_Pressed", FVector2D(23.0f, 18.0f)));

	const FButtonStyle FullMobileContentScaleFactorButtonStyle = FButtonStyle(Button)
		.SetNormal(IMAGE_BRUSH("Icons/PIEWindow/WindowButton_1x_Normal", FVector2D(23.0f, 18)))
		.SetHovered(IMAGE_BRUSH("Icons/PIEWindow/WindowButton_1x_Hovered", FVector2D(23.0f, 18)))
		.SetPressed(IMAGE_BRUSH("Icons/PIEWindow/WindowButton_1x_Pressed", FVector2D(23.0f, 18)));

	Style->Set("PIEWindow", FPIEPreviewWindowStyle()
		.SetScreenRotationButtonStyle(ScreenRotationButtonStyle)
		.SetQuarterMobileContentScaleFactorButtonStyle(QuarterMobileContentScaleFactorButtonStyle)
		.SetHalfMobileContentScaleFactorButtonStyle(HalfMobileContentScaleFactorButtonStyle)
		.SetFullMobileContentScaleFactorButtonStyle(FullMobileContentScaleFactorButtonStyle)
	);

	return Style;
}

#undef IMAGE_BRUSH


void FPIEPreviewWindowCoreStyle::InitializePIECoreStyle()
{
	if (!Instance.IsValid())
	{
		Instance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*Instance.Get());
	}
}
#endif