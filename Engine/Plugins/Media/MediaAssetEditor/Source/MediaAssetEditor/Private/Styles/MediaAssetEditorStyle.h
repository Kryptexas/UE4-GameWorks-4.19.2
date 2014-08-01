// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush(RootToContentDir(RelativePath, TEXT(".png")), __VA_ARGS__)
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush(RootToContentDir(RelativePath, TEXT(".png")), __VA_ARGS__)
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush(RootToContentDir(RelativePath, TEXT(".png")), __VA_ARGS__)
#define TTF_FONT( RelativePath, ... ) FSlateFontInfo(RootToContentDir(RelativePath, TEXT(".ttf")), __VA_ARGS__)
#define OTF_FONT( RelativePath, ... ) FSlateFontInfo(RootToContentDir(RelativePath, TEXT(".otf")), __VA_ARGS__)


/**
 * Implements the visual style of the messaging debugger UI.
 */
class FMediaAssetEditorStyle
	: public FSlateStyleSet
{
public:

	/**
	 * Default constructor.
	 */
	 FMediaAssetEditorStyle( )
		 : FSlateStyleSet("MediaAssetEditorStyle")
	 {
		const FVector2D Icon16x16(16.0f, 16.0f);
		const FVector2D Icon20x20(20.0f, 20.0f);
		const FVector2D Icon40x40(40.0f, 40.0f);

		SetContentRoot(FPaths::EnginePluginsDir() / TEXT("Media/MediaAssetEditor/Content"));

		// toolbar icons
		Set("MediaAssetEditor.ForwardMedia", new IMAGE_BRUSH("icon_forward_40x", Icon40x40));
		Set("MediaAssetEditor.ForwardMedia.Small", new IMAGE_BRUSH("icon_forward_40x", Icon20x20));
		Set("MediaAssetEditor.PauseMedia", new IMAGE_BRUSH("icon_pause_40x", Icon40x40));
		Set("MediaAssetEditor.PauseMedia.Small", new IMAGE_BRUSH("icon_pause_40x", Icon20x20));
		Set("MediaAssetEditor.PlayMedia", new IMAGE_BRUSH("icon_play_40x", Icon40x40));
		Set("MediaAssetEditor.PlayMedia.Small", new IMAGE_BRUSH("icon_play_40x", Icon20x20));
		Set("MediaAssetEditor.ReverseMedia", new IMAGE_BRUSH("icon_reverse_40x", Icon40x40));
		Set("MediaAssetEditor.ReverseMedia.Small", new IMAGE_BRUSH("icon_reverse_40x", Icon20x20));
		Set("MediaAssetEditor.RewindMedia", new IMAGE_BRUSH("icon_rewind_40x", Icon40x40));
		Set("MediaAssetEditor.RewindMedia.Small", new IMAGE_BRUSH("icon_rewind_40x", Icon20x20));
		Set("MediaAssetEditor.StopMedia", new IMAGE_BRUSH("icon_stop_40x", Icon40x40));
		Set("MediaAssetEditor.StopMedia.Small", new IMAGE_BRUSH("icon_stop_40x", Icon20x20));
		
		FSlateStyleRegistry::RegisterSlateStyle(*this);
	 }

	 /**
	  * Destructor.
	  */
	 ~FMediaAssetEditorStyle( )
	 {
		FSlateStyleRegistry::UnRegisterSlateStyle(*this);
	 }
};


#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef TTF_FONT
#undef OTF_FONT
