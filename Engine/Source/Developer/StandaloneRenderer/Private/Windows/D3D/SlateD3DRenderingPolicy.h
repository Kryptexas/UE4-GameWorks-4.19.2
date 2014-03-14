// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#ifndef __SLATE_D3D_RENDERING_POLICY_H__
#define __SLATE_D3D_RENDERING_POLICY_H__

class FSlateD3D11RenderingPolicy : public FSlateRenderingPolicy
{
public:
	FSlateD3D11RenderingPolicy( TSharedPtr<FSlateFontCache> InFontCache, TSharedRef<FSlateD3DTextureManager> InTextureManager );
	~FSlateD3D11RenderingPolicy();

	void UpdateBuffers( const FSlateWindowElementList& InElementList );
	void DrawElements( const FMatrix& ViewProjectionMatrix, const TArray<FSlateRenderBatch>& RenderBatches );

	FSlateShaderResource* GetViewportResource( const ISlateViewport* InViewportInterface ) { return NULL; }
	FSlateShaderResourceProxy* GetTextureResource( const FSlateBrush& Brush );
	TSharedPtr<FSlateFontCache>& GetFontCache() { return FontCache; }

private:
	void InitResources();
	void ReleaseResources();
private:
	FSlateD3DVertexBuffer VertexBuffer;
	FSlateD3DIndexBuffer IndexBuffer;
	FSlateDefaultVS* VertexShader;
	FSlateDefaultPS* PixelShader;
	FSlateShaderResource* WhiteTexture;
	TRefCountPtr<ID3D11RasterizerState> NormalRasterState;
	TRefCountPtr<ID3D11RasterizerState> WireframeRasterState;
	TRefCountPtr<ID3D11RasterizerState> ScissorRasterState;
	TRefCountPtr<ID3D11BlendState> AlphaBlendState;
	TRefCountPtr<ID3D11BlendState> NoBlendState;
	TRefCountPtr<ID3D11DepthStencilState> DSStateOff;
	TRefCountPtr<ID3D11SamplerState> PointSamplerState_Wrap;
	TRefCountPtr<ID3D11SamplerState> PointSamplerState_Clamp;
	TRefCountPtr<ID3D11SamplerState> BilinearSamplerState_Wrap;
	TRefCountPtr<ID3D11SamplerState> BilinearSamplerState_Clamp;
	TSharedPtr<FSlateFontCache> FontCache;
	TSharedRef<FSlateD3DTextureManager> TextureManager;
};

#endif // __SLATE_D3D_RENDERING_POLICY_H__
