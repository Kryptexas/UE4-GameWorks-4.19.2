// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	TextureEditorViewportClient.h: Declares the FTextureEditorViewportClient class.
=============================================================================*/

#pragma once


class FTextureEditorViewportClient
	: public FViewportClient
	, public FGCObject
{
public:
	/** Constructor */
	FTextureEditorViewportClient(TWeakPtr<ITextureEditorToolkit> InTextureEditor, TWeakPtr<STextureEditorViewport> InTextureEditorViewport);
	~FTextureEditorViewportClient();

	/** FViewportClient interface */
	virtual void Draw(FViewport* Viewport, FCanvas* Canvas) OVERRIDE;
	virtual bool InputKey(FViewport* Viewport, int32 ControllerId, FKey Key, EInputEvent Event, float AmountDepressed = 1.0f, bool bGamepad = false) OVERRIDE;
	virtual UWorld* GetWorld() const OVERRIDE { return World; }

	/** FGCObject interface */
	virtual void AddReferencedObjects(FReferenceCollector& Collector) OVERRIDE;

	/** Modifies the checkerboard texture's data */
	void ModifyCheckerboardTextureColors();

	/** Returns a string representation of the currently displayed textures resolution */
	FString GetDisplayedResolution() const;

	/** Returns the ratio of the size of the Texture texture to the size of the viewport */
	float GetViewportVerticalScrollBarRatio() const;
	float GetViewportHorizontalScrollBarRatio() const;

private:
	/** Updates the states of the scrollbars */
	void UpdateScrollBars();

	/** Returns the positions of the scrollbars relative to the Texture textures */
	FVector2D GetViewportScrollBarPositions() const;

	/** Initialize the checkerboard texture for the texture preview, if necessary */
	void SetupCheckerboardTexture();

	/** Destroy the checkerboard texture if one exists */
	void DestroyCheckerboardTexture();

private:
	/** Pointer back to the Texture editor tool that owns us */
	TWeakPtr<ITextureEditorToolkit> TextureEditorPtr;

	/** Pointer back to the Texture viewport control that owns us */
	TWeakPtr<STextureEditorViewport> TextureEditorViewportPtr;

	/** Checkerboard texture */
	UTexture2D* CheckerboardTexture;

	/** The UWorld that this level editor is viewing and allowing the user to interact with through */
	UWorld* World;
};
