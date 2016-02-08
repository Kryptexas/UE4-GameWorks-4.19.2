// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "SequenceRecorderPrivatePCH.h"
#include "SequenceRecorderStyle.h"
#include "SlateStyle.h"
#include "IPluginManager.h"

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( FSequenceRecorderStyle::InContent( RelativePath, ".png" ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( FSequenceRecorderStyle::InContent( RelativePath, ".png" ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( FSequenceRecorderStyle::InContent( RelativePath, ".png" ), __VA_ARGS__ )
#define TTF_FONT( RelativePath, ... ) FSlateFontInfo( FSequenceRecorderStyle::InContent( RelativePath, ".ttf" ), __VA_ARGS__ )
#define TTF_CORE_FONT(RelativePath, ...) FSlateFontInfo( StyleSet->RootToCoreContentDir( RelativePath, TEXT(".ttf") ), __VA_ARGS__ )
#define OTF_FONT( RelativePath, ... ) FSlateFontInfo( FSequenceRecorderStyle::InContent( RelativePath, ".otf" ), __VA_ARGS__ )
#define OTF_CORE_FONT(RelativePath, ...) FSlateFontInfo( StyleSet->RootToCoreContentDir( RelativePath, TEXT(".otf") ), __VA_ARGS__ )

FString FSequenceRecorderStyle::InContent( const FString& RelativePath, const ANSICHAR* Extension )
{
	static FString ContentDir = IPluginManager::Get().FindPlugin(TEXT("SequenceRecorder"))->GetContentDir();
	return ( ContentDir / RelativePath ) + Extension;
}

TSharedPtr< FSlateStyleSet > FSequenceRecorderStyle::StyleSet = NULL;
TSharedPtr< class ISlateStyle > FSequenceRecorderStyle::Get() { return StyleSet; }

FName FSequenceRecorderStyle::GetStyleSetName()
{
	static FName SequenceRecorderStyleName(TEXT("SequenceRecorderStyle"));
	return SequenceRecorderStyleName;
}

void FSequenceRecorderStyle::Initialize()
{
	// Const icon sizes
	const FVector2D Icon8x8(8.0f, 8.0f);
	const FVector2D Icon9x19(9.0f, 19.0f);
	const FVector2D Icon10x10(10.0f, 10.0f);
	const FVector2D Icon12x12(12.0f, 12.0f);
	const FVector2D Icon16x16(16.0f, 16.0f);
	const FVector2D Icon20x20(20.0f, 20.0f);
	const FVector2D Icon22x22(22.0f, 22.0f);
	const FVector2D Icon24x24(24.0f, 24.0f);
	const FVector2D Icon27x31(27.0f, 31.0f);
	const FVector2D Icon26x26(26.0f, 26.0f);
	const FVector2D Icon32x32(32.0f, 32.0f);
	const FVector2D Icon40x40(40.0f, 40.0f);
	const FVector2D Icon75x82(75.0f, 82.0f);
	const FVector2D Icon360x32(360.0f, 32.0f);
	const FVector2D Icon171x39(171.0f, 39.0f);
	const FVector2D Icon170x50(170.0f, 50.0f);
	const FVector2D Icon267x140(170.0f, 50.0f);

	// Only register once
	if( StyleSet.IsValid() )
	{
		return;
	}

	StyleSet = MakeShareable( new FSlateStyleSet(FSequenceRecorderStyle::GetStyleSetName()) );
	StyleSet->SetCoreContentRoot(FPaths::EngineContentDir() / TEXT("Slate"));

	// Animation recorder standalone UI
	{
		StyleSet->Set( "SequenceRecorder.TabIcon", new IMAGE_BRUSH( "Icons/icon_tab_SequenceRecorder_16x", Icon16x16 ) );
		StyleSet->Set( "SequenceRecorder.Common.RecordAll", new IMAGE_BRUSH( "Icons/icon_RecordAll_40x", Icon40x40 ) );
		StyleSet->Set( "SequenceRecorder.Common.StopAll", new IMAGE_BRUSH( "Icons/icon_StopAll_40x", Icon40x40 ) );
		StyleSet->Set( "SequenceRecorder.Common.AddRecording", new IMAGE_BRUSH( "Icons/icon_AddRecording_40x", Icon40x40 ) );
		StyleSet->Set( "SequenceRecorder.Common.RemoveRecording", new IMAGE_BRUSH( "Icons/icon_RemoveRecording_40x", Icon40x40 ) );
	}

	FSlateStyleRegistry::RegisterSlateStyle( *StyleSet.Get() );
};

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef TTF_FONT
#undef TTF_CORE_FONT
#undef OTF_FONT
#undef OTF_CORE_FONT

void FSequenceRecorderStyle::Shutdown()
{
	if( StyleSet.IsValid() )
	{
		FSlateStyleRegistry::UnRegisterSlateStyle( *StyleSet.Get() );
		ensure( StyleSet.IsUnique() );
		StyleSet.Reset();
	}
}