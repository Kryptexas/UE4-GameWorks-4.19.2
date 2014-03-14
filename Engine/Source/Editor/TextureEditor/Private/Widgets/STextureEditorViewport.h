// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	STextureEditorViewport.h: Declares the STextureEditorViewport class.
=============================================================================*/

#pragma once


/**
 * Implements the texture editor's view port.
 */
class STextureEditorViewport
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(STextureEditorViewport) { }
	SLATE_END_ARGS()

public:

	/**
	 */
	void AddReferencedObjects( FReferenceCollector& Collector );

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs - The construction arguments.
	 */
	void Construct( const FArguments& InArgs, const TSharedRef<ITextureEditorToolkit>& InToolkit );
	
	/**
	 * Modifies the checkerboard texture's data.
	 */
	void ModifyCheckerboardTextureColors( );

	/**
	 * Gets the exposure bias.
	 *
	 * @return Exposure bias value.
	 */
	int32 GetExposureBias( ) const
	{
		return ExposureBias;
	}

	TSharedPtr<FSceneViewport> GetViewport( ) const;
	TSharedPtr<SViewport> GetViewportWidget( ) const;
	TSharedPtr<SScrollBar> GetVerticalScrollBar( ) const;
	TSharedPtr<SScrollBar> GetHorizontalScrollBar( ) const;

public:

	// Begin SWidget overrides

	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) OVERRIDE;

	// End SWidget overrides

protected:

	/** Returns a string representation of the currently displayed textures resolution */
	FString GetDisplayedResolution() const;

private:

	// Callback for getting the visibility of the exposure bias widget.
	EVisibility HandleExposureBiasWidgetVisibility( ) const;

	// Callback for getting the exposure bias.
	TOptional<int32> HandleExposureBiasBoxValue( ) const;

	// Callback for changing the exposure bias.
	void HandleExposureBiasBoxValueChanged( int32 NewExposure );

	// Callback for the horizontal scroll bar.
	void HandleHorizontalScrollBarScrolled( float InScrollOffsetFraction );

	// Callback for getting the visibility of the horizontal scroll bar.
	EVisibility HandleHorizontalScrollBarVisibility( ) const;

	// Callback for the vertical scroll bar.
	void HandleVerticalScrollBarScrolled( float InScrollOffsetFraction );

	// Callback for getting the visibility of the horizontal scroll bar.
	EVisibility HandleVerticalScrollBarVisibility( ) const;

	// Callback for clicking an item in the 'Zoom' menu.
	void HandleZoomMenuEntryClicked( double ZoomValue );

	// Callback for getting the zoom percentage text.
	FText HandleZoomPercentageText( ) const;

	// Callback for changes in the zoom slider.
	void HandleZoomSliderChanged( float NewValue );

	// Callback for getting the zoom slider's value.
	float HandleZoomSliderValue( ) const;

private:

	// Which exposure level should be used, in FStop e.g. 0:original, -1:half as bright, 1:2x as bright, 2:4x as bright.
	int32 ExposureBias;

	// Pointer back to the Texture editor tool that owns us.
	TWeakPtr<ITextureEditorToolkit> ToolkitPtr;
	
	// Level viewport client.
	TSharedPtr<class FTextureEditorViewportClient> ViewportClient;

	// Slate viewport for rendering and IO.
	TSharedPtr<FSceneViewport> Viewport;

	// Viewport widget.
	TSharedPtr<SViewport> ViewportWidget;

	// Vertical scrollbar.
	TSharedPtr<SScrollBar> TextureViewportVerticalScrollBar;

	// Horizontal scrollbar.
	TSharedPtr<SScrollBar> TextureViewportHorizontalScrollBar;
};
