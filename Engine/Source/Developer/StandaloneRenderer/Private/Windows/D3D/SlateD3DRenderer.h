// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#ifndef _SLATERENDERRD3D_H__
#define _SLATERENDERRD3D_H__

#include "Renderer.h"

class FSlateD3DTextureManager;
class FSlateD3D11RenderingPolicy;

struct FSlateD3DViewport
{
	FMatrix ProjectionMatrix;
	D3D11_VIEWPORT ViewportInfo;
	TRefCountPtr<IDXGISwapChain> D3DSwapChain;
	TRefCountPtr<ID3D11Texture2D> BackBufferTexture;
	TRefCountPtr<ID3D11RenderTargetView> RenderTargetView;
	TRefCountPtr<ID3D11DepthStencilView> DepthStencilView;
	bool bFullscreen;

	FSlateD3DViewport()
		: bFullscreen( false )
	{

	}

	~FSlateD3DViewport()
	{	
		BackBufferTexture.SafeRelease();
		RenderTargetView.SafeRelease();
		DepthStencilView.SafeRelease();
		D3DSwapChain.SafeRelease();
	}

};

extern TRefCountPtr<ID3D11Device> GD3DDevice;
extern TRefCountPtr<ID3D11DeviceContext> GD3DDeviceContext;

class FSlateD3DRenderer : public FSlateRenderer
{
public:
	STANDALONERENDERER_API FSlateD3DRenderer( const ISlateStyle &InStyle );
	~FSlateD3DRenderer();

	/** FSlateRenderer Interface */
	virtual void Initialize() OVERRIDE;
	virtual void Destroy() OVERRIDE;
	virtual FSlateDrawBuffer& GetDrawBuffer() OVERRIDE; 
	virtual void DrawWindows( FSlateDrawBuffer& InWindowDrawBuffer ) OVERRIDE;
	virtual void OnWindowDestroyed( const TSharedRef<SWindow>& InWindow ) OVERRIDE;
	virtual void CreateViewport( const TSharedRef<SWindow> InWindow ) OVERRIDE;
	virtual void RequestResize( const TSharedPtr<SWindow>& InWindow, uint32 NewSizeX, uint32 NewSizeY ) OVERRIDE;
	virtual void UpdateFullscreenState( const TSharedRef<SWindow> InWindow, uint32 OverrideResX, uint32 OverrideResY ) OVERRIDE;
	virtual void ReleaseDynamicResource( const FSlateBrush& Brush ) OVERRIDE;
	virtual bool GenerateDynamicImageResource(FName ResourceName, uint32 Width, uint32 Height, const TArray< uint8 >& Bytes) OVERRIDE;
	virtual void LoadStyleResources( const ISlateStyle& Style ) OVERRIDE;
	
	void CreateDevice();
	void CreateDepthStencilBuffer( FSlateD3DViewport& Viewport );

private:
	void Private_CreateViewport( TSharedRef<SWindow> InWindow, const FVector2D& WindowSize );
	void Private_ResizeViewport( const TSharedRef<SWindow> InWindow, uint32 Width, uint32 Height, bool bFullscreen );
	void CreateBackBufferResources( TRefCountPtr<IDXGISwapChain>& InSwapChain, TRefCountPtr<ID3D11Texture2D>& OutBackBuffer, TRefCountPtr<ID3D11RenderTargetView>& OutRTV );
private:
	FMatrix ViewMatrix;
	TMap<const SWindow*, FSlateD3DViewport> WindowToViewportMap;
	FSlateDrawBuffer DrawBuffer;
	TSharedPtr<FSlateElementBatcher> ElementBatcher;
	TSharedPtr<FSlateD3DTextureManager> TextureManager;
	TSharedPtr<FSlateD3D11RenderingPolicy> RenderingPolicy;
};

#endif