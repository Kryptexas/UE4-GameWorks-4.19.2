// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class SVirtualWindow : public SWindow
{
	SLATE_BEGIN_ARGS(SVirtualWindow)
		: _Size(FVector2D(100, 100))
	{}

	SLATE_ARGUMENT(FVector2D, Size)
	
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);
};

/**
 * 
 */
class FWidgetRenderer
{
public:
	FWidgetRenderer();
	~FWidgetRenderer();

	ISlate3DRenderer* GetSlateRenderer();

	UTextureRenderTarget2D* DrawWidget(TSharedRef<SWidget>& Widget, FVector2D DrawSize);
	void DrawWidget(UTextureRenderTarget2D* RenderTarget, TSharedRef<SWidget>& Widget, FVector2D DrawSize, float DeltaTime);
	void DrawWindow(UTextureRenderTarget2D* RenderTarget, TSharedRef<class FHittestGrid> HitTestGrid, TSharedRef<class SWindow> Window, FVector2D DrawSize, float DeltaTime);

private:
	/** The slate 3D renderer used to render the user slate widget */
	TSharedPtr<class ISlate3DRenderer> Renderer;
};
