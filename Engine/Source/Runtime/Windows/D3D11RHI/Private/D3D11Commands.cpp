// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	D3D11Commands.cpp: D3D RHI commands implementation.
=============================================================================*/

#include "D3D11RHIPrivate.h"
#if WITH_D3DX_LIBS
#include "AllowWindowsPlatformTypes.h"
	#include <xnamath.h>
#include "HideWindowsPlatformTypes.h"
#endif
#include "D3D11RHIPrivateUtil.h"
#include "StaticBoundShaderState.h"
#include "GlobalShader.h"
#include "OneColorShader.h"

FGlobalBoundShaderState GD3D11ClearMRTBoundShaderState[8];
TGlobalResource<FVector4VertexDeclaration> GD3D11Vector4VertexDeclaration;

#define DECLARE_ISBOUNDSHADER(ShaderType) inline void ValidateBoundShader(FD3D11StateCache& InStateCache, F##ShaderType##RHIParamRef ShaderType##RHI) \
{ \
	ID3D11##ShaderType* CachedShader; \
	InStateCache.Get##ShaderType(&CachedShader); \
	DYNAMIC_CAST_D3D11RESOURCE(ShaderType,ShaderType); \
	ensureMsgf(CachedShader == ShaderType->Resource, TEXT("Parameters are being set for a %s which is not currently bound"), TEXT( #ShaderType )); \
}

DECLARE_ISBOUNDSHADER(VertexShader)
DECLARE_ISBOUNDSHADER(PixelShader)
DECLARE_ISBOUNDSHADER(GeometryShader)
DECLARE_ISBOUNDSHADER(HullShader)
DECLARE_ISBOUNDSHADER(DomainShader)
DECLARE_ISBOUNDSHADER(ComputeShader)

#if DO_CHECK
#define VALIDATE_BOUND_SHADER(s) ValidateBoundShader(StateCache, s)
#else
#define VALIDATE_BOUND_SHADER(s)
#endif

// Vertex state.
void FD3D11DynamicRHI::RHISetStreamSource(uint32 StreamIndex,FVertexBufferRHIParamRef VertexBufferRHI,uint32 Stride,uint32 Offset)
{
	DYNAMIC_CAST_D3D11RESOURCE(VertexBuffer,VertexBuffer);

	ID3D11Buffer* D3DBuffer = VertexBuffer ? VertexBuffer->Resource : NULL;
	StateCache.SetStreamSource(D3DBuffer, StreamIndex, Stride, Offset);
}

void FD3D11DynamicRHI::RHISetStreamOutTargets(uint32 NumTargets, const FVertexBufferRHIParamRef* VertexBuffers, const uint32* Offsets)
{
	ID3D11Buffer* D3DVertexBuffers[D3D11_SO_BUFFER_SLOT_COUNT] = {0};

	if (VertexBuffers)
	{
		for (uint32 BufferIndex = 0; BufferIndex < NumTargets; BufferIndex++)
		{
			D3DVertexBuffers[BufferIndex] = VertexBuffers[BufferIndex] ? ((FD3D11VertexBuffer*)VertexBuffers[BufferIndex])->Resource : NULL;
		}
	}

	Direct3DDeviceIMContext->SOSetTargets(NumTargets, D3DVertexBuffers, Offsets);
}

// Rasterizer state.
void FD3D11DynamicRHI::RHISetRasterizerState(FRasterizerStateRHIParamRef NewStateRHI)
{
	DYNAMIC_CAST_D3D11RESOURCE(RasterizerState,NewState);
	StateCache.SetRasterizerState(NewState->Resource);
}

void FD3D11DynamicRHI::RHISetComputeShader(FComputeShaderRHIParamRef ComputeShaderRHI)
{
	DYNAMIC_CAST_D3D11RESOURCE(ComputeShader,ComputeShader);
	SetCurrentComputeShader(ComputeShaderRHI);
}

void FD3D11DynamicRHI::RHIDispatchComputeShader(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ) 
{ 
	FComputeShaderRHIParamRef ComputeShaderRHI = GetCurrentComputeShader();
	DYNAMIC_CAST_D3D11RESOURCE(ComputeShader,ComputeShader);

	StateCache.SetComputeShader(ComputeShader->Resource);

	GPUProfilingData.RegisterGPUWork(1);	

	if (ComputeShader->bShaderNeedsGlobalConstantBuffer)
	{
		CommitComputeShaderConstants();
	}
	
	Direct3DDeviceIMContext->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);	
	StateCache.SetComputeShader(nullptr);
}

void FD3D11DynamicRHI::RHIDispatchIndirectComputeShader(FVertexBufferRHIParamRef ArgumentBufferRHI, uint32 ArgumentOffset) 
{ 
	FComputeShaderRHIParamRef ComputeShaderRHI = GetCurrentComputeShader();
	DYNAMIC_CAST_D3D11RESOURCE(ComputeShader,ComputeShader);
	DYNAMIC_CAST_D3D11RESOURCE(VertexBuffer,ArgumentBuffer);

	GPUProfilingData.RegisterGPUWork(1);

	StateCache.SetComputeShader(ComputeShader->Resource);
	
	if (ComputeShader->bShaderNeedsGlobalConstantBuffer)
	{
		CommitComputeShaderConstants();
	}

	Direct3DDeviceIMContext->DispatchIndirect(ArgumentBuffer->Resource,ArgumentOffset);
	StateCache.SetComputeShader(nullptr);
}

void FD3D11DynamicRHI::RHISetViewport(uint32 MinX,uint32 MinY,float MinZ,uint32 MaxX,uint32 MaxY,float MaxZ)
{
	// These are the maximum viewport extents for D3D11. Exceeding them leads to badness.
	check(MinX <= (uint32)D3D11_VIEWPORT_BOUNDS_MAX);
	check(MinY <= (uint32)D3D11_VIEWPORT_BOUNDS_MAX);
	check(MaxX <= (uint32)D3D11_VIEWPORT_BOUNDS_MAX);
	check(MaxY <= (uint32)D3D11_VIEWPORT_BOUNDS_MAX);

	D3D11_VIEWPORT Viewport = { MinX, MinY, MaxX - MinX, MaxY - MinY, MinZ, MaxZ };
	//avoid setting a 0 extent viewport, which the debug runtime doesn't like
	if (Viewport.Width > 0 && Viewport.Height > 0)
	{
		StateCache.SetViewport(Viewport);
		SetScissorRectIfRequiredWhenSettingViewport(MinX, MinY, MaxX, MaxY);
	}
}

void FD3D11DynamicRHI::RHISetScissorRect(bool bEnable,uint32 MinX,uint32 MinY,uint32 MaxX,uint32 MaxY)
{
	if(bEnable)
	{
		D3D11_RECT ScissorRect;
		ScissorRect.left = MinX;
		ScissorRect.right = MaxX;
		ScissorRect.top = MinY;
		ScissorRect.bottom = MaxY;
		Direct3DDeviceIMContext->RSSetScissorRects(1,&ScissorRect);
	}
	else
	{
		D3D11_RECT ScissorRect;
		ScissorRect.left = 0;
		ScissorRect.right = GetMax2DTextureDimension();
		ScissorRect.top = 0;
		ScissorRect.bottom = GetMax2DTextureDimension();
		Direct3DDeviceIMContext->RSSetScissorRects(1,&ScissorRect);
	}
}

/**
* Set bound shader state. This will set the vertex decl/shader, and pixel shader
* @param BoundShaderState - state resource
*/
void FD3D11DynamicRHI::RHISetBoundShaderState( FBoundShaderStateRHIParamRef BoundShaderStateRHI)
{
	DYNAMIC_CAST_D3D11RESOURCE(BoundShaderState,BoundShaderState);

	StateCache.SetInputLayout(BoundShaderState->InputLayout);
	StateCache.SetVertexShader(BoundShaderState->VertexShader);
	StateCache.SetPixelShader(BoundShaderState->PixelShader);

	StateCache.SetHullShader(BoundShaderState->HullShader);
	StateCache.SetDomainShader(BoundShaderState->DomainShader);
	StateCache.SetGeometryShader(BoundShaderState->GeometryShader);

	if(BoundShaderState->HullShader != NULL && BoundShaderState->DomainShader != NULL)
	{
		bUsingTessellation = true;
	}
	else
	{
		bUsingTessellation = false;
	}

	// @TODO : really should only discard the constants if the shader state has actually changed.
	bDiscardSharedConstants = true;

	// Prevent transient bound shader states from being recreated for each use by keeping a history of the most recently used bound shader states.
	// The history keeps them alive, and the bound shader state cache allows them to be reused if needed.
	BoundShaderStateHistory.Add(BoundShaderState);
}

void FD3D11DynamicRHI::RHISetShaderTexture(FVertexShaderRHIParamRef VertexShaderRHI,uint32 TextureIndex,FTextureRHIParamRef NewTextureRHI)
{
	VALIDATE_BOUND_SHADER(VertexShaderRHI);

	FD3D11TextureBase* NewTexture = GetD3D11TextureFromRHITexture(NewTextureRHI);
	ID3D11ShaderResourceView* ShaderResourceView = NewTexture ? NewTexture->GetShaderResourceView() : NULL;
	
	if (  ( NewTexture == NULL) || ( NewTexture->GetRenderTargetView( 0, 0 ) !=NULL) || ( NewTexture->HasDepthStencilView()) )
		SetShaderResourceView<SF_Vertex>(NewTexture, ShaderResourceView, TextureIndex, FD3D11StateCache::SRV_Dynamic);
	else
		SetShaderResourceView<SF_Vertex>(NewTexture, ShaderResourceView, TextureIndex, FD3D11StateCache::SRV_Static);
}

void FD3D11DynamicRHI::RHISetShaderTexture(FHullShaderRHIParamRef HullShaderRHI,uint32 TextureIndex,FTextureRHIParamRef NewTextureRHI)
{
	VALIDATE_BOUND_SHADER(HullShaderRHI);

	FD3D11TextureBase* NewTexture = GetD3D11TextureFromRHITexture(NewTextureRHI);
	ID3D11ShaderResourceView* ShaderResourceView = NewTexture ? NewTexture->GetShaderResourceView() : NULL;

	if (  ( NewTexture == NULL) || ( NewTexture->GetRenderTargetView( 0, 0 ) !=NULL) || ( NewTexture->HasDepthStencilView()) )
		SetShaderResourceView<SF_Hull>(NewTexture, ShaderResourceView, TextureIndex, FD3D11StateCache::SRV_Dynamic);
	else
		SetShaderResourceView<SF_Hull>(NewTexture, ShaderResourceView, TextureIndex, FD3D11StateCache::SRV_Static);

}

void FD3D11DynamicRHI::RHISetShaderTexture(FDomainShaderRHIParamRef DomainShaderRHI,uint32 TextureIndex,FTextureRHIParamRef NewTextureRHI)
{
	VALIDATE_BOUND_SHADER(DomainShaderRHI);

	FD3D11TextureBase* NewTexture = GetD3D11TextureFromRHITexture(NewTextureRHI);
	ID3D11ShaderResourceView* ShaderResourceView = NewTexture ? NewTexture->GetShaderResourceView() : NULL;

	if (  ( NewTexture == NULL) || ( NewTexture->GetRenderTargetView( 0, 0 ) !=NULL) || ( NewTexture->HasDepthStencilView()) )
		SetShaderResourceView<SF_Domain>(NewTexture, ShaderResourceView, TextureIndex, FD3D11StateCache::SRV_Dynamic);
	else
		SetShaderResourceView<SF_Domain>(NewTexture, ShaderResourceView, TextureIndex, FD3D11StateCache::SRV_Static);
}

void FD3D11DynamicRHI::RHISetShaderTexture(FGeometryShaderRHIParamRef GeometryShaderRHI,uint32 TextureIndex,FTextureRHIParamRef NewTextureRHI)
{
	VALIDATE_BOUND_SHADER(GeometryShaderRHI);

	FD3D11TextureBase* NewTexture = GetD3D11TextureFromRHITexture(NewTextureRHI);
	ID3D11ShaderResourceView* ShaderResourceView = NewTexture ? NewTexture->GetShaderResourceView() : NULL;

	if (  ( NewTexture == NULL) || ( NewTexture->GetRenderTargetView( 0, 0 ) !=NULL) || ( NewTexture->HasDepthStencilView()) )
		SetShaderResourceView<SF_Geometry>(NewTexture, ShaderResourceView, TextureIndex, FD3D11StateCache::SRV_Dynamic);
	else
		SetShaderResourceView<SF_Geometry>(NewTexture, ShaderResourceView, TextureIndex, FD3D11StateCache::SRV_Static);
}

void FD3D11DynamicRHI::RHISetShaderTexture(FPixelShaderRHIParamRef PixelShaderRHI,uint32 TextureIndex,FTextureRHIParamRef NewTextureRHI)
{
	VALIDATE_BOUND_SHADER(PixelShaderRHI);

	FD3D11TextureBase* NewTexture = GetD3D11TextureFromRHITexture(NewTextureRHI);
	ID3D11ShaderResourceView* ShaderResourceView = NewTexture ? NewTexture->GetShaderResourceView() : NULL;
	if ( ( NewTexture == NULL) ||  ( NewTexture->GetRenderTargetView( 0, 0 ) !=NULL) || ( NewTexture->HasDepthStencilView()) )
		SetShaderResourceView<SF_Pixel>(NewTexture, ShaderResourceView, TextureIndex, FD3D11StateCache::SRV_Dynamic);
	else
		SetShaderResourceView<SF_Pixel>(NewTexture, ShaderResourceView, TextureIndex, FD3D11StateCache::SRV_Static);

}

void FD3D11DynamicRHI::RHISetShaderTexture(FComputeShaderRHIParamRef ComputeShaderRHI,uint32 TextureIndex,FTextureRHIParamRef NewTextureRHI)
{
	//VALIDATE_BOUND_SHADER(ComputeShaderRHI);

	FD3D11TextureBase* NewTexture = GetD3D11TextureFromRHITexture(NewTextureRHI);
	ID3D11ShaderResourceView* ShaderResourceView = NewTexture ? NewTexture->GetShaderResourceView() : NULL;

	if ( ( NewTexture == NULL) || ( NewTexture->GetRenderTargetView( 0, 0 ) !=NULL) || ( NewTexture->HasDepthStencilView()) )
		SetShaderResourceView<SF_Compute>(NewTexture, ShaderResourceView, TextureIndex, FD3D11StateCache::SRV_Dynamic);
	else
		SetShaderResourceView<SF_Compute>(NewTexture, ShaderResourceView, TextureIndex, FD3D11StateCache::SRV_Static);
}

void FD3D11DynamicRHI::RHISetUAVParameter(FComputeShaderRHIParamRef ComputeShaderRHI,uint32 UAVIndex,FUnorderedAccessViewRHIParamRef UAVRHI)
{
	//VALIDATE_BOUND_SHADER(ComputeShaderRHI);

	DYNAMIC_CAST_D3D11RESOURCE(UnorderedAccessView,UAV);
	
	if(UAV)
	{
		ConditionalClearShaderResource(UAV->Resource);
	}

	ID3D11UnorderedAccessView* D3D11UAV = UAV ? UAV->View : NULL;

	uint32 InitialCount = -1;
	Direct3DDeviceIMContext->CSSetUnorderedAccessViews(UAVIndex,1,&D3D11UAV, &InitialCount );
}

void FD3D11DynamicRHI::RHISetUAVParameter(FComputeShaderRHIParamRef ComputeShaderRHI,uint32 UAVIndex,FUnorderedAccessViewRHIParamRef UAVRHI, uint32 InitialCount )
{
	//VALIDATE_BOUND_SHADER(ComputeShaderRHI);

	DYNAMIC_CAST_D3D11RESOURCE(UnorderedAccessView,UAV);

	if(UAV)
	{
		ConditionalClearShaderResource(UAV->Resource);
	}

	ID3D11UnorderedAccessView* D3D11UAV = UAV ? UAV->View : NULL;
	Direct3DDeviceIMContext->CSSetUnorderedAccessViews(UAVIndex,1,&D3D11UAV, &InitialCount );
}

void FD3D11DynamicRHI::RHISetShaderResourceViewParameter(FPixelShaderRHIParamRef PixelShaderRHI,uint32 TextureIndex,FShaderResourceViewRHIParamRef SRVRHI)
{
	VALIDATE_BOUND_SHADER(PixelShaderRHI);

	DYNAMIC_CAST_D3D11RESOURCE(ShaderResourceView,SRV);

	FD3D11BaseShaderResource* Resource = nullptr;
	ID3D11ShaderResourceView* D3D11SRV = nullptr;

	if (SRV)
	{
		Resource = SRV->Resource;
		D3D11SRV = SRV->View;
	}

	SetShaderResourceView<SF_Pixel>(Resource, D3D11SRV, TextureIndex);
}

void FD3D11DynamicRHI::RHISetShaderResourceViewParameter(FVertexShaderRHIParamRef VertexShaderRHI,uint32 TextureIndex,FShaderResourceViewRHIParamRef SRVRHI)
{
	VALIDATE_BOUND_SHADER(VertexShaderRHI);

	DYNAMIC_CAST_D3D11RESOURCE(ShaderResourceView,SRV);

	FD3D11BaseShaderResource* Resource = nullptr;
	ID3D11ShaderResourceView* D3D11SRV = nullptr;

	if (SRV)
	{
		Resource = SRV->Resource;
		D3D11SRV = SRV->View;
	}

	SetShaderResourceView<SF_Vertex>(Resource, D3D11SRV, TextureIndex);
}

void FD3D11DynamicRHI::RHISetShaderResourceViewParameter(FComputeShaderRHIParamRef ComputeShaderRHI,uint32 TextureIndex,FShaderResourceViewRHIParamRef SRVRHI)
{
	//VALIDATE_BOUND_SHADER(ComputeShaderRHI);

	DYNAMIC_CAST_D3D11RESOURCE(ShaderResourceView,SRV);

	FD3D11BaseShaderResource* Resource = nullptr;
	ID3D11ShaderResourceView* D3D11SRV = nullptr;

	if (SRV)
	{
		Resource = SRV->Resource;
		D3D11SRV = SRV->View;
	}

	SetShaderResourceView<SF_Compute>(Resource, D3D11SRV, TextureIndex);
}

void FD3D11DynamicRHI::RHISetShaderResourceViewParameter(FHullShaderRHIParamRef HullShaderRHI,uint32 TextureIndex,FShaderResourceViewRHIParamRef SRVRHI)
{
	VALIDATE_BOUND_SHADER(HullShaderRHI);

	DYNAMIC_CAST_D3D11RESOURCE(ShaderResourceView,SRV);

	FD3D11BaseShaderResource* Resource = nullptr;
	ID3D11ShaderResourceView* D3D11SRV = nullptr;

	if (SRV)
	{
		Resource = SRV->Resource;
		D3D11SRV = SRV->View;
	}

	SetShaderResourceView<SF_Hull>(Resource, D3D11SRV, TextureIndex);
}

void FD3D11DynamicRHI::RHISetShaderResourceViewParameter(FDomainShaderRHIParamRef DomainShaderRHI,uint32 TextureIndex,FShaderResourceViewRHIParamRef SRVRHI)
{
	VALIDATE_BOUND_SHADER(DomainShaderRHI);

	DYNAMIC_CAST_D3D11RESOURCE(ShaderResourceView,SRV);

	FD3D11BaseShaderResource* Resource = nullptr;
	ID3D11ShaderResourceView* D3D11SRV = nullptr;

	if (SRV)
	{
		Resource = SRV->Resource;
		D3D11SRV = SRV->View;
	}

	SetShaderResourceView<SF_Domain>(Resource, D3D11SRV, TextureIndex);
}

void FD3D11DynamicRHI::RHISetShaderResourceViewParameter(FGeometryShaderRHIParamRef GeometryShaderRHI,uint32 TextureIndex,FShaderResourceViewRHIParamRef SRVRHI)
{
	VALIDATE_BOUND_SHADER(GeometryShaderRHI);

	DYNAMIC_CAST_D3D11RESOURCE(ShaderResourceView,SRV);

	FD3D11BaseShaderResource* Resource = nullptr;
	ID3D11ShaderResourceView* D3D11SRV = nullptr;

	if (SRV)
	{
		Resource = SRV->Resource;
		D3D11SRV = SRV->View;
	}

	SetShaderResourceView<SF_Geometry>(Resource, D3D11SRV, TextureIndex);
}

void FD3D11DynamicRHI::RHISetShaderSampler(FVertexShaderRHIParamRef VertexShaderRHI,uint32 SamplerIndex,FSamplerStateRHIParamRef NewStateRHI)
{
	VALIDATE_BOUND_SHADER(VertexShaderRHI);

	DYNAMIC_CAST_D3D11RESOURCE(VertexShader,VertexShader);
	DYNAMIC_CAST_D3D11RESOURCE(SamplerState,NewState);

	ID3D11SamplerState* StateResource = NewState->Resource;
	StateCache.SetSamplerState<SF_Vertex>(StateResource, SamplerIndex);
}

void FD3D11DynamicRHI::RHISetShaderSampler(FHullShaderRHIParamRef HullShaderRHI,uint32 SamplerIndex,FSamplerStateRHIParamRef NewStateRHI)
{
	VALIDATE_BOUND_SHADER(HullShaderRHI);

	DYNAMIC_CAST_D3D11RESOURCE(HullShader,HullShader);
	DYNAMIC_CAST_D3D11RESOURCE(SamplerState,NewState);

	ID3D11SamplerState* StateResource = NewState->Resource;
	StateCache.SetSamplerState<SF_Hull>(StateResource, SamplerIndex);
}

void FD3D11DynamicRHI::RHISetShaderSampler(FDomainShaderRHIParamRef DomainShaderRHI,uint32 SamplerIndex,FSamplerStateRHIParamRef NewStateRHI)
{
	VALIDATE_BOUND_SHADER(DomainShaderRHI);

	DYNAMIC_CAST_D3D11RESOURCE(DomainShader,DomainShader);
	DYNAMIC_CAST_D3D11RESOURCE(SamplerState,NewState);

	ID3D11SamplerState* StateResource = NewState->Resource;
	StateCache.SetSamplerState<SF_Domain>(StateResource, SamplerIndex);
}

void FD3D11DynamicRHI::RHISetShaderSampler(FGeometryShaderRHIParamRef GeometryShaderRHI,uint32 SamplerIndex,FSamplerStateRHIParamRef NewStateRHI)
{
	VALIDATE_BOUND_SHADER(GeometryShaderRHI);

	DYNAMIC_CAST_D3D11RESOURCE(GeometryShader,GeometryShader);
	DYNAMIC_CAST_D3D11RESOURCE(SamplerState,NewState);

	ID3D11SamplerState* StateResource = NewState->Resource;
	StateCache.SetSamplerState<SF_Geometry>(StateResource, SamplerIndex);
}

void FD3D11DynamicRHI::RHISetShaderSampler(FPixelShaderRHIParamRef PixelShaderRHI,uint32 SamplerIndex,FSamplerStateRHIParamRef NewStateRHI)
{
	VALIDATE_BOUND_SHADER(PixelShaderRHI);

	DYNAMIC_CAST_D3D11RESOURCE(PixelShader,PixelShader);
	DYNAMIC_CAST_D3D11RESOURCE(SamplerState,NewState);

	ID3D11SamplerState* StateResource = NewState->Resource;
	StateCache.SetSamplerState<SF_Pixel>(StateResource, SamplerIndex);
}

void FD3D11DynamicRHI::RHISetShaderSampler(FComputeShaderRHIParamRef ComputeShaderRHI,uint32 SamplerIndex,FSamplerStateRHIParamRef NewStateRHI)
{
	//VALIDATE_BOUND_SHADER(ComputeShaderRHI);
	DYNAMIC_CAST_D3D11RESOURCE(ComputeShader,ComputeShader);
	DYNAMIC_CAST_D3D11RESOURCE(SamplerState,NewState);

	ID3D11SamplerState* StateResource = NewState->Resource;
	StateCache.SetSamplerState<SF_Compute>(StateResource, SamplerIndex);
}

void FD3D11DynamicRHI::RHISetShaderUniformBuffer(FVertexShaderRHIParamRef VertexShader,uint32 BufferIndex,FUniformBufferRHIParamRef BufferRHI)
{
	VALIDATE_BOUND_SHADER(VertexShader);
	DYNAMIC_CAST_D3D11RESOURCE(UniformBuffer,Buffer);
	ID3D11Buffer* ConstantBuffer = Buffer ? Buffer->Resource : NULL;
	StateCache.SetConstantBuffer<SF_Vertex>(ConstantBuffer, BufferIndex);
}

void FD3D11DynamicRHI::RHISetShaderUniformBuffer(FHullShaderRHIParamRef HullShader,uint32 BufferIndex,FUniformBufferRHIParamRef BufferRHI)
{
	VALIDATE_BOUND_SHADER(HullShader);
	DYNAMIC_CAST_D3D11RESOURCE(UniformBuffer,Buffer);
	ID3D11Buffer* ConstantBuffer = Buffer ? Buffer->Resource : NULL;
	StateCache.SetConstantBuffer<SF_Hull>(ConstantBuffer, BufferIndex);
}

void FD3D11DynamicRHI::RHISetShaderUniformBuffer(FDomainShaderRHIParamRef DomainShader,uint32 BufferIndex,FUniformBufferRHIParamRef BufferRHI)
{
	VALIDATE_BOUND_SHADER(DomainShader);
	DYNAMIC_CAST_D3D11RESOURCE(UniformBuffer,Buffer);
	ID3D11Buffer* ConstantBuffer = Buffer ? Buffer->Resource : NULL;
	StateCache.SetConstantBuffer<SF_Domain>(ConstantBuffer, BufferIndex);
}

void FD3D11DynamicRHI::RHISetShaderUniformBuffer(FGeometryShaderRHIParamRef GeometryShader,uint32 BufferIndex,FUniformBufferRHIParamRef BufferRHI)
{
	VALIDATE_BOUND_SHADER(GeometryShader);
	DYNAMIC_CAST_D3D11RESOURCE(UniformBuffer,Buffer);
	ID3D11Buffer* ConstantBuffer = Buffer ? Buffer->Resource : NULL;
	StateCache.SetConstantBuffer<SF_Geometry>(ConstantBuffer, BufferIndex);
}

void FD3D11DynamicRHI::RHISetShaderUniformBuffer(FPixelShaderRHIParamRef PixelShader,uint32 BufferIndex,FUniformBufferRHIParamRef BufferRHI)
{
	VALIDATE_BOUND_SHADER(PixelShader);
	DYNAMIC_CAST_D3D11RESOURCE(UniformBuffer,Buffer);
	ID3D11Buffer* ConstantBuffer = Buffer ? Buffer->Resource : NULL;
	StateCache.SetConstantBuffer<SF_Pixel>(ConstantBuffer, BufferIndex);
}

void FD3D11DynamicRHI::RHISetShaderUniformBuffer(FComputeShaderRHIParamRef ComputeShader,uint32 BufferIndex,FUniformBufferRHIParamRef BufferRHI)
{
	//VALIDATE_BOUND_SHADER(ComputeShader);
	DYNAMIC_CAST_D3D11RESOURCE(UniformBuffer,Buffer);
	ID3D11Buffer* ConstantBuffer = Buffer ? Buffer->Resource : NULL;
	StateCache.SetConstantBuffer<SF_Compute>(ConstantBuffer, BufferIndex);
}

void FD3D11DynamicRHI::RHISetShaderParameter(FHullShaderRHIParamRef HullShaderRHI,uint32 BufferIndex,uint32 BaseIndex,uint32 NumBytes,const void* NewValue)
{
	VALIDATE_BOUND_SHADER(HullShaderRHI);
	checkSlow(HSConstantBuffers[BufferIndex]);
	HSConstantBuffers[BufferIndex]->UpdateConstant((const uint8*)NewValue,BaseIndex,NumBytes);
}

void FD3D11DynamicRHI::RHISetShaderParameter(FDomainShaderRHIParamRef DomainShaderRHI,uint32 BufferIndex,uint32 BaseIndex,uint32 NumBytes,const void* NewValue)
{
	VALIDATE_BOUND_SHADER(DomainShaderRHI);
	checkSlow(DSConstantBuffers[BufferIndex]);
	DSConstantBuffers[BufferIndex]->UpdateConstant((const uint8*)NewValue,BaseIndex,NumBytes);
}

void FD3D11DynamicRHI::RHISetShaderParameter(FVertexShaderRHIParamRef VertexShaderRHI,uint32 BufferIndex,uint32 BaseIndex,uint32 NumBytes,const void* NewValue)
{
	VALIDATE_BOUND_SHADER(VertexShaderRHI);
	checkSlow(VSConstantBuffers[BufferIndex]);
	VSConstantBuffers[BufferIndex]->UpdateConstant((const uint8*)NewValue,BaseIndex,NumBytes);
}

void FD3D11DynamicRHI::RHISetShaderParameter(FPixelShaderRHIParamRef PixelShaderRHI,uint32 BufferIndex,uint32 BaseIndex,uint32 NumBytes,const void* NewValue)
{
	VALIDATE_BOUND_SHADER(PixelShaderRHI);
	checkSlow(PSConstantBuffers[BufferIndex]);
	PSConstantBuffers[BufferIndex]->UpdateConstant((const uint8*)NewValue,BaseIndex,NumBytes);
}

void FD3D11DynamicRHI::RHISetShaderParameter(FGeometryShaderRHIParamRef GeometryShaderRHI,uint32 BufferIndex,uint32 BaseIndex,uint32 NumBytes,const void* NewValue)
{
	VALIDATE_BOUND_SHADER(GeometryShaderRHI);
	checkSlow(GSConstantBuffers[BufferIndex]);
	GSConstantBuffers[BufferIndex]->UpdateConstant((const uint8*)NewValue,BaseIndex,NumBytes);
}

void FD3D11DynamicRHI::RHISetShaderParameter(FComputeShaderRHIParamRef ComputeShaderRHI,uint32 BufferIndex,uint32 BaseIndex,uint32 NumBytes,const void* NewValue)
{
	//VALIDATE_BOUND_SHADER(ComputeShaderRHI);
	checkSlow(CSConstantBuffers[BufferIndex]);
	CSConstantBuffers[BufferIndex]->UpdateConstant((const uint8*)NewValue,BaseIndex,NumBytes);
}

void FD3D11DynamicRHI::RHISetDepthStencilState(FDepthStencilStateRHIParamRef NewStateRHI,uint32 StencilRef)
{
	DYNAMIC_CAST_D3D11RESOURCE(DepthStencilState,NewState);

	if (CurrentDepthTexture && NewState->AccessType != CurrentDSVAccessType)
	{
		CurrentDSVAccessType = NewState->AccessType;
		CurrentDepthStencilTarget = CurrentDepthTexture->GetDepthStencilView(CurrentDSVAccessType);

		// Unbind any shader views of the depth stencil target that are bound.
		ConditionalClearShaderResource(CurrentDepthTexture);

		CommitRenderTargetsAndUAVs();
	}

	StateCache.SetDepthStencilState(NewState->Resource, StencilRef);
}

void FD3D11DynamicRHI::RHISetBlendState(FBlendStateRHIParamRef NewStateRHI,const FLinearColor& BlendFactor)
{
	DYNAMIC_CAST_D3D11RESOURCE(BlendState,NewState);
	StateCache.SetBlendState(NewState->Resource, (const float*)&BlendFactor, 0xffffffff);
}

void FD3D11DynamicRHI::CommitRenderTargetsAndUAVs()
{
	ID3D11RenderTargetView* RTArray[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
	for(uint32 RenderTargetIndex = 0;RenderTargetIndex < NumSimultaneousRenderTargets;++RenderTargetIndex)
	{
		RTArray[RenderTargetIndex] = CurrentRenderTargets[RenderTargetIndex];
	}
	ID3D11UnorderedAccessView* UAVArray[D3D11_PS_CS_UAV_REGISTER_COUNT];
	uint32 UAVInitialCountArray[D3D11_PS_CS_UAV_REGISTER_COUNT];
	for(uint32 UAVIndex = 0;UAVIndex < NumUAVs;++UAVIndex)
	{
		UAVArray[UAVIndex] = CurrentUAVs[UAVIndex];
		// Using the value that indicates to keep the current UAV counter
		UAVInitialCountArray[UAVIndex] = -1;
	}

	if (NumUAVs > 0)
	{
		Direct3DDeviceIMContext->OMSetRenderTargetsAndUnorderedAccessViews(
			NumSimultaneousRenderTargets,
			RTArray,
			CurrentDepthStencilTarget,
			NumSimultaneousRenderTargets,
			NumUAVs,
			UAVArray,
			UAVInitialCountArray
			);
	}
	else
	{
		// Use OMSetRenderTargets if there are no UAVs, works around a crash in PIX
		Direct3DDeviceIMContext->OMSetRenderTargets(
			NumSimultaneousRenderTargets,
			RTArray,
			CurrentDepthStencilTarget
			);
	}
}

void FD3D11DynamicRHI::RHISetRenderTargets(
	uint32 NewNumSimultaneousRenderTargets,
	const FRHIRenderTargetView* NewRenderTargetsRHI,
	FTextureRHIParamRef NewDepthStencilTargetRHI,
	uint32 NewNumUAVs,
	const FUnorderedAccessViewRHIParamRef* UAVs
	)
{
	FD3D11TextureBase* NewDepthStencilTarget = GetD3D11TextureFromRHITexture(NewDepthStencilTargetRHI);

	check(NewNumSimultaneousRenderTargets + NewNumUAVs <= MaxSimultaneousRenderTargets);

	bool bTargetChanged = false;

	// Set the appropriate depth stencil view depending on whether depth writes are enabled or not
	ID3D11DepthStencilView* DepthStencilView = NULL;
	if(NewDepthStencilTarget)
	{
		DepthStencilView = NewDepthStencilTarget->GetDepthStencilView(CurrentDSVAccessType);
		
		// Unbind any shader views of the depth stencil target that are bound.
		ConditionalClearShaderResource(NewDepthStencilTarget);
	}

	// Check if the depth stencil target is different from the old state.
	if(CurrentDepthStencilTarget != DepthStencilView)
	{
		CurrentDepthTexture = NewDepthStencilTarget;
		CurrentDepthStencilTarget = DepthStencilView;
		bTargetChanged = true;
	}

	// Gather the render target views for the new render targets.
	ID3D11RenderTargetView* NewRenderTargetViews[MaxSimultaneousRenderTargets];
	for(uint32 RenderTargetIndex = 0;RenderTargetIndex < MaxSimultaneousRenderTargets;++RenderTargetIndex)
	{
		ID3D11RenderTargetView* RenderTargetView = NULL;
		if(RenderTargetIndex < NewNumSimultaneousRenderTargets && IsValidRef(NewRenderTargetsRHI[RenderTargetIndex].Texture))
		{
			FD3D11TextureBase* NewRenderTarget = GetD3D11TextureFromRHITexture(NewRenderTargetsRHI[RenderTargetIndex].Texture);
			RenderTargetView = NewRenderTarget->GetRenderTargetView(NewRenderTargetsRHI[RenderTargetIndex].MipIndex, NewRenderTargetsRHI[RenderTargetIndex].ArraySliceIndex);
			
			// Unbind any shader views of the render target that are bound.
			ConditionalClearShaderResource(NewRenderTarget);

#if UE_BUILD_DEBUG	
			// A check to allow you to pinpoint what is using mismatching targets
			// We filter our d3ddebug spew that checks for this as the d3d runtime's check is wrong.
			// For filter code, see D3D11Device.cpp look for "OMSETRENDERTARGETS_INVALIDVIEW"
			if(RenderTargetView && DepthStencilView)
			{
				// Set the viewport to the full size of the surface
				TRefCountPtr<ID3D11Texture2D> RenderTargetTexture;
				RenderTargetView->GetResource((ID3D11Resource**)RenderTargetTexture.GetInitReference());

				TRefCountPtr<ID3D11Texture2D> DepthTargetTexture;
				DepthStencilView->GetResource((ID3D11Resource**)DepthTargetTexture.GetInitReference());

				D3D11_TEXTURE2D_DESC RTTDesc;
				RenderTargetTexture->GetDesc(&RTTDesc);

				D3D11_TEXTURE2D_DESC DTTDesc;
				DepthTargetTexture->GetDesc(&DTTDesc);

				// enforce color target is <= depth and MSAA settings match
				if(RTTDesc.Width > DTTDesc.Width || RTTDesc.Height > DTTDesc.Height || 
					RTTDesc.SampleDesc.Count != DTTDesc.SampleDesc.Count || 
					RTTDesc.SampleDesc.Quality != DTTDesc.SampleDesc.Quality)
				{
					UE_LOG(LogD3D11RHI, Fatal,TEXT("RTV(%i,%i c=%i,q=%i) and DSV(%i,%i c=%i,q=%i) have mismatching dimensions and/or MSAA levels!"),
						RTTDesc.Width,RTTDesc.Height,RTTDesc.SampleDesc.Count,RTTDesc.SampleDesc.Quality,
						DTTDesc.Width,DTTDesc.Height,DTTDesc.SampleDesc.Count,DTTDesc.SampleDesc.Quality);
				}
			}
#endif
		}

		NewRenderTargetViews[RenderTargetIndex] = RenderTargetView;

		// Check if the render target is different from the old state.
		if(CurrentRenderTargets[RenderTargetIndex] != RenderTargetView)
		{
			CurrentRenderTargets[RenderTargetIndex] = RenderTargetView;
			bTargetChanged = true;
		}
	}
	if(NumSimultaneousRenderTargets != NewNumSimultaneousRenderTargets)
	{
		NumSimultaneousRenderTargets = NewNumSimultaneousRenderTargets;
		bTargetChanged = true;
	}

	// Gather the new UAVs.
	for(uint32 UAVIndex = 0;UAVIndex < MaxSimultaneousUAVs;++UAVIndex)
	{
		ID3D11UnorderedAccessView* UAV = NULL;
		if(UAVIndex < NewNumUAVs && UAVs[UAVIndex] != NULL)
		{
			FD3D11UnorderedAccessView* RHIUAV = (FD3D11UnorderedAccessView*)UAVs[UAVIndex];
			UAV = RHIUAV->View;

			// Unbind any shader views of the UAV's resource.
			ConditionalClearShaderResource(RHIUAV->Resource);
		}

		if(CurrentUAVs[UAVIndex] != UAV)
		{
			CurrentUAVs[UAVIndex] = UAV;
			bTargetChanged = true;
		}
	}
	if(NumUAVs != NewNumUAVs)
	{
		NumUAVs = NewNumUAVs;
		bTargetChanged = true;
	}

	// Only make the D3D call to change render targets if something actually changed.
	if(bTargetChanged)
	{
		CommitRenderTargetsAndUAVs();
	}
	
	// Set the viewport to the full size of render target 0.
	if(NewRenderTargetViews[0])
	{
		TRefCountPtr<ID3D11Texture2D> BaseResource;
		NewRenderTargetViews[0]->GetResource((ID3D11Resource**)BaseResource.GetInitReference());

		D3D11_TEXTURE2D_DESC Desc;
		BaseResource->GetDesc(&Desc);

		RHISetViewport(0,0,0.0f,Desc.Width,Desc.Height,1.0f);
	}
}

void FD3D11DynamicRHI::RHIDiscardRenderTargets(bool Depth, bool Stencil, uint32 ColorBitMask)
{
	// Could support in DX11.1 via ID3D11DeviceContext1::Discard*() functions.
}

// Occlusion/Timer queries.
void FD3D11DynamicRHI::RHIBeginRenderQuery(FRenderQueryRHIParamRef QueryRHI)
{
	DYNAMIC_CAST_D3D11RESOURCE(OcclusionQuery,Query);

	if(Query->QueryType == RQT_Occlusion)
	{
		Direct3DDeviceIMContext->Begin(Query->Resource);
	}
	else
	{
		// not supported/needed for RQT_AbsoluteTime
		check(0);
	}
}
void FD3D11DynamicRHI::RHIEndRenderQuery(FRenderQueryRHIParamRef QueryRHI)
{
	DYNAMIC_CAST_D3D11RESOURCE(OcclusionQuery,Query);
	Direct3DDeviceIMContext->End(Query->Resource);

	//@todo - d3d debug spews warnings about OQ's that are being issued but not polled, need to investigate
}

// Primitive drawing.

static D3D11_PRIMITIVE_TOPOLOGY GetD3D11PrimitiveType(uint32 PrimitiveType, bool bUsingTessellation)
{
	if(bUsingTessellation)
	{
		switch(PrimitiveType)
		{
		case PT_1_ControlPointPatchList: return D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST;
		case PT_2_ControlPointPatchList: return D3D11_PRIMITIVE_TOPOLOGY_2_CONTROL_POINT_PATCHLIST;

		// This is the case for tessellation without AEN or other buffers, so just flip to 3 CPs
		case PT_TriangleList: return D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;

		case PT_LineList:
		case PT_TriangleStrip:
		case PT_QuadList:
		case PT_PointList:
			UE_LOG(LogD3D11RHI, Fatal,TEXT("Invalid type specified for tessellated render, probably missing a case in FSkeletalMeshSceneProxy::DrawDynamicElementsByMaterial or FStaticMeshSceneProxy::GetMeshElement"));
			break;
		default:
			// Other cases are valid.
			break;
		};
	}

	switch(PrimitiveType)
	{
	case PT_TriangleList: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	case PT_TriangleStrip: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
	case PT_LineList: return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
	case PT_PointList: return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;

	// ControlPointPatchList types will pretend to be TRIANGLELISTS with a stride of N 
	// (where N is the number of control points specified), so we can return them for
	// tessellation and non-tessellation. This functionality is only used when rendering a 
	// default material with something that claims to be tessellated, generally because the 
	// tessellation material failed to compile for some reason.
	case PT_3_ControlPointPatchList: return D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
	case PT_4_ControlPointPatchList: return D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST;
	case PT_5_ControlPointPatchList: return D3D11_PRIMITIVE_TOPOLOGY_5_CONTROL_POINT_PATCHLIST;
	case PT_6_ControlPointPatchList: return D3D11_PRIMITIVE_TOPOLOGY_6_CONTROL_POINT_PATCHLIST;
	case PT_7_ControlPointPatchList: return D3D11_PRIMITIVE_TOPOLOGY_7_CONTROL_POINT_PATCHLIST;
	case PT_8_ControlPointPatchList: return D3D11_PRIMITIVE_TOPOLOGY_8_CONTROL_POINT_PATCHLIST; 
	case PT_9_ControlPointPatchList: return D3D11_PRIMITIVE_TOPOLOGY_9_CONTROL_POINT_PATCHLIST; 
	case PT_10_ControlPointPatchList: return D3D11_PRIMITIVE_TOPOLOGY_10_CONTROL_POINT_PATCHLIST; 
	case PT_11_ControlPointPatchList: return D3D11_PRIMITIVE_TOPOLOGY_11_CONTROL_POINT_PATCHLIST; 
	case PT_12_ControlPointPatchList: return D3D11_PRIMITIVE_TOPOLOGY_12_CONTROL_POINT_PATCHLIST; 
	case PT_13_ControlPointPatchList: return D3D11_PRIMITIVE_TOPOLOGY_13_CONTROL_POINT_PATCHLIST; 
	case PT_14_ControlPointPatchList: return D3D11_PRIMITIVE_TOPOLOGY_14_CONTROL_POINT_PATCHLIST; 
	case PT_15_ControlPointPatchList: return D3D11_PRIMITIVE_TOPOLOGY_15_CONTROL_POINT_PATCHLIST; 
	case PT_16_ControlPointPatchList: return D3D11_PRIMITIVE_TOPOLOGY_16_CONTROL_POINT_PATCHLIST; 
	case PT_17_ControlPointPatchList: return D3D11_PRIMITIVE_TOPOLOGY_17_CONTROL_POINT_PATCHLIST; 
	case PT_18_ControlPointPatchList: return D3D11_PRIMITIVE_TOPOLOGY_18_CONTROL_POINT_PATCHLIST; 
	case PT_19_ControlPointPatchList: return D3D11_PRIMITIVE_TOPOLOGY_19_CONTROL_POINT_PATCHLIST; 
	case PT_20_ControlPointPatchList: return D3D11_PRIMITIVE_TOPOLOGY_20_CONTROL_POINT_PATCHLIST; 
	case PT_21_ControlPointPatchList: return D3D11_PRIMITIVE_TOPOLOGY_21_CONTROL_POINT_PATCHLIST; 
	case PT_22_ControlPointPatchList: return D3D11_PRIMITIVE_TOPOLOGY_22_CONTROL_POINT_PATCHLIST; 
	case PT_23_ControlPointPatchList: return D3D11_PRIMITIVE_TOPOLOGY_23_CONTROL_POINT_PATCHLIST; 
	case PT_24_ControlPointPatchList: return D3D11_PRIMITIVE_TOPOLOGY_24_CONTROL_POINT_PATCHLIST; 
	case PT_25_ControlPointPatchList: return D3D11_PRIMITIVE_TOPOLOGY_25_CONTROL_POINT_PATCHLIST; 
	case PT_26_ControlPointPatchList: return D3D11_PRIMITIVE_TOPOLOGY_26_CONTROL_POINT_PATCHLIST; 
	case PT_27_ControlPointPatchList: return D3D11_PRIMITIVE_TOPOLOGY_27_CONTROL_POINT_PATCHLIST; 
	case PT_28_ControlPointPatchList: return D3D11_PRIMITIVE_TOPOLOGY_28_CONTROL_POINT_PATCHLIST; 
	case PT_29_ControlPointPatchList: return D3D11_PRIMITIVE_TOPOLOGY_29_CONTROL_POINT_PATCHLIST; 
	case PT_30_ControlPointPatchList: return D3D11_PRIMITIVE_TOPOLOGY_30_CONTROL_POINT_PATCHLIST; 
	case PT_31_ControlPointPatchList: return D3D11_PRIMITIVE_TOPOLOGY_31_CONTROL_POINT_PATCHLIST; 
	case PT_32_ControlPointPatchList: return D3D11_PRIMITIVE_TOPOLOGY_32_CONTROL_POINT_PATCHLIST; 
	default: UE_LOG(LogD3D11RHI, Fatal,TEXT("Unknown primitive type: %u"),PrimitiveType);
	};

	return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
}

static uint32 GetVertexCountForPrimitiveCount(uint32 NumPrimitives, uint32 PrimitiveType)
{
	uint32 VertexCount = 0;
	switch(PrimitiveType)
	{
	case PT_TriangleList: VertexCount = NumPrimitives*3; break;
	case PT_TriangleStrip: VertexCount = NumPrimitives+2; break;
	case PT_LineList: VertexCount = NumPrimitives*2; break;

	case PT_1_ControlPointPatchList:
	case PT_2_ControlPointPatchList:
	case PT_3_ControlPointPatchList: 
	case PT_4_ControlPointPatchList: 
	case PT_5_ControlPointPatchList:
	case PT_6_ControlPointPatchList:
	case PT_7_ControlPointPatchList: 
	case PT_8_ControlPointPatchList: 
	case PT_9_ControlPointPatchList: 
	case PT_10_ControlPointPatchList: 
	case PT_11_ControlPointPatchList: 
	case PT_12_ControlPointPatchList: 
	case PT_13_ControlPointPatchList: 
	case PT_14_ControlPointPatchList: 
	case PT_15_ControlPointPatchList: 
	case PT_16_ControlPointPatchList: 
	case PT_17_ControlPointPatchList: 
	case PT_18_ControlPointPatchList: 
	case PT_19_ControlPointPatchList: 
	case PT_20_ControlPointPatchList: 
	case PT_21_ControlPointPatchList: 
	case PT_22_ControlPointPatchList: 
	case PT_23_ControlPointPatchList: 
	case PT_24_ControlPointPatchList: 
	case PT_25_ControlPointPatchList: 
	case PT_26_ControlPointPatchList: 
	case PT_27_ControlPointPatchList: 
	case PT_28_ControlPointPatchList: 
	case PT_29_ControlPointPatchList: 
	case PT_30_ControlPointPatchList: 
	case PT_31_ControlPointPatchList: 
	case PT_32_ControlPointPatchList: 
		VertexCount = (PrimitiveType - PT_1_ControlPointPatchList + 1) * NumPrimitives;
		break;
	default: UE_LOG(LogD3D11RHI, Fatal,TEXT("Unknown primitive type: %u"),PrimitiveType);
	};

	return VertexCount;
}

void FD3D11DynamicRHI::CommitNonComputeShaderConstants()
{
	FD3D11BoundShaderState* CurrentBoundShaderState = (FD3D11BoundShaderState*)BoundShaderStateHistory.GetLast();
	check(CurrentBoundShaderState);

	// Only set the constant buffer if this shader needs the global constant buffer bound
	// Otherwise we will overwrite a different constant buffer
	if (CurrentBoundShaderState->bShaderNeedsGlobalConstantBuffer[SF_Vertex])
	{
		// Commit and bind vertex shader constants
		for(uint32 i=0;i<MAX_CONSTANT_BUFFER_SLOTS; i++)
		{
			FD3D11ConstantBuffer* ConstantBuffer = VSConstantBuffers[i];
			FD3DRHIUtil::CommitConstants<SF_Vertex>(ConstantBuffer, StateCache, i, bDiscardSharedConstants);
		}
	}

	// Skip HS/DS CB updates in cases where tessellation isn't being used
	// Note that this is *potentially* unsafe because bDiscardSharedConstants is cleared at the
	// end of the function, however we're OK for now because bDiscardSharedConstants
	// is always reset whenever bUsingTessellation changes in SetBoundShaderState()
	if(bUsingTessellation)
	{
		if (CurrentBoundShaderState->bShaderNeedsGlobalConstantBuffer[SF_Hull])
		{
			// Commit and bind hull shader constants
			for(uint32 i=0;i<MAX_CONSTANT_BUFFER_SLOTS; i++)
			{
				FD3D11ConstantBuffer* ConstantBuffer = HSConstantBuffers[i];
				FD3DRHIUtil::CommitConstants<SF_Hull>(ConstantBuffer, StateCache, i, bDiscardSharedConstants);
			}
		}

		if (CurrentBoundShaderState->bShaderNeedsGlobalConstantBuffer[SF_Domain])
		{
			// Commit and bind domain shader constants
			for(uint32 i=0;i<MAX_CONSTANT_BUFFER_SLOTS; i++)
			{
				FD3D11ConstantBuffer* ConstantBuffer = DSConstantBuffers[i];
				FD3DRHIUtil::CommitConstants<SF_Domain>(ConstantBuffer, StateCache, i, bDiscardSharedConstants);
			}
		}
	}

	if (CurrentBoundShaderState->bShaderNeedsGlobalConstantBuffer[SF_Geometry])
	{
		// Commit and bind geometry shader constants
		for(uint32 i=0;i<MAX_CONSTANT_BUFFER_SLOTS; i++)
		{
			FD3D11ConstantBuffer* ConstantBuffer = GSConstantBuffers[i];
			FD3DRHIUtil::CommitConstants<SF_Geometry>(ConstantBuffer, StateCache, i, bDiscardSharedConstants);
		}
	}

	if (CurrentBoundShaderState->bShaderNeedsGlobalConstantBuffer[SF_Pixel])
	{
		// Commit and bind pixel shader constants
		for(uint32 i=0;i<MAX_CONSTANT_BUFFER_SLOTS; i++)
		{
			FD3D11ConstantBuffer* ConstantBuffer = PSConstantBuffers[i];
			FD3DRHIUtil::CommitConstants<SF_Pixel>(ConstantBuffer, StateCache, i, bDiscardSharedConstants);
		}
	}

	bDiscardSharedConstants = false;
}

void FD3D11DynamicRHI::CommitComputeShaderConstants()
{
	bool bLocalDiscardSharedConstants = true;

	// Commit and bind compute shader constants
	for(uint32 i=0;i<MAX_CONSTANT_BUFFER_SLOTS; i++)
	{
		FD3D11ConstantBuffer* ConstantBuffer = CSConstantBuffers[i];
		FD3DRHIUtil::CommitConstants<SF_Compute>(ConstantBuffer, StateCache, i, bDiscardSharedConstants);
	}
}

void FD3D11DynamicRHI::RHIDrawPrimitive(uint32 PrimitiveType,uint32 BaseVertexIndex,uint32 NumPrimitives,uint32 NumInstances)
{
	RHI_DRAW_CALL_STATS(PrimitiveType,NumInstances*NumPrimitives);

	CommitNonComputeShaderConstants();
	uint32 VertexCount = RHIGetVertexCountForPrimitiveCount(NumPrimitives,PrimitiveType);

	GPUProfilingData.RegisterGPUWork(NumPrimitives * NumInstances, VertexCount * NumInstances);

	StateCache.SetPrimitiveTopology(GetD3D11PrimitiveType(PrimitiveType,bUsingTessellation));
	if(NumInstances > 1)
	{
		Direct3DDeviceIMContext->DrawInstanced(VertexCount,NumInstances,BaseVertexIndex,0);
	}
	else
	{
		Direct3DDeviceIMContext->Draw(VertexCount,BaseVertexIndex);
	}
}

void FD3D11DynamicRHI::RHIDrawPrimitiveIndirect(uint32 PrimitiveType,FVertexBufferRHIParamRef ArgumentBufferRHI,uint32 ArgumentOffset)
{
	DYNAMIC_CAST_D3D11RESOURCE(VertexBuffer,ArgumentBuffer);

	RHI_DRAW_CALL_INC();

	GPUProfilingData.RegisterGPUWork(0);

	CommitNonComputeShaderConstants();

	StateCache.SetPrimitiveTopology(GetD3D11PrimitiveType(PrimitiveType,bUsingTessellation));
	Direct3DDeviceIMContext->DrawInstancedIndirect(ArgumentBuffer->Resource,ArgumentOffset);
}

void FD3D11DynamicRHI::RHIDrawIndexedIndirect(FIndexBufferRHIParamRef IndexBufferRHI, uint32 PrimitiveType, FStructuredBufferRHIParamRef ArgumentsBufferRHI, int32 DrawArgumentsIndex, uint32 NumInstances)
{
	DYNAMIC_CAST_D3D11RESOURCE(IndexBuffer,IndexBuffer);
	DYNAMIC_CAST_D3D11RESOURCE(StructuredBuffer,ArgumentsBuffer);

	RHI_DRAW_CALL_INC();

	GPUProfilingData.RegisterGPUWork(1);

	CommitNonComputeShaderConstants();
	// determine 16bit vs 32bit indices
	uint32 SizeFormat = sizeof(DXGI_FORMAT);
	const DXGI_FORMAT Format = (IndexBuffer->GetStride() == sizeof(uint16) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT);

	StateCache.SetIndexBuffer(IndexBuffer->Resource, Format, 0);
	StateCache.SetPrimitiveTopology(GetD3D11PrimitiveType(PrimitiveType,bUsingTessellation));

	if(NumInstances > 1)
	{
		Direct3DDeviceIMContext->DrawIndexedInstancedIndirect(ArgumentsBuffer->Resource, DrawArgumentsIndex * 5 * sizeof(uint32));
	}
	else
	{
		check(0);
	}
}

void FD3D11DynamicRHI::RHIDrawIndexedPrimitive(FIndexBufferRHIParamRef IndexBufferRHI,uint32 PrimitiveType,int32 BaseVertexIndex,uint32 MinIndex,uint32 NumVertices,uint32 StartIndex,uint32 NumPrimitives,uint32 NumInstances)
{
	DYNAMIC_CAST_D3D11RESOURCE(IndexBuffer,IndexBuffer);

	// called should make sure the input is valid, this avoid hidden bugs
	ensure(NumPrimitives > 0);

	RHI_DRAW_CALL_STATS(PrimitiveType,NumInstances*NumPrimitives);

	GPUProfilingData.RegisterGPUWork(NumPrimitives * NumInstances, NumVertices * NumInstances);

	CommitNonComputeShaderConstants();
	// determine 16bit vs 32bit indices
	uint32 SizeFormat = sizeof(DXGI_FORMAT);
	const DXGI_FORMAT Format = (IndexBuffer->GetStride() == sizeof(uint16) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT);

	uint32 IndexCount = RHIGetVertexCountForPrimitiveCount(NumPrimitives,PrimitiveType);

	// Verify that we are not trying to read outside the index buffer range
	checkf(StartIndex + IndexCount <= IndexBuffer->GetSize() / IndexBuffer->GetStride(), 
		TEXT("Start %u, Count %u, Type %u, Buffer Size %u, Buffer stride %u"), StartIndex, IndexCount, PrimitiveType, IndexBuffer->GetSize(), IndexBuffer->GetStride());

	StateCache.SetIndexBuffer(IndexBuffer->Resource, Format, 0);
	StateCache.SetPrimitiveTopology(GetD3D11PrimitiveType(PrimitiveType,bUsingTessellation));

	if(NumInstances > 1)
	{
		Direct3DDeviceIMContext->DrawIndexedInstanced(IndexCount,NumInstances,StartIndex,BaseVertexIndex,0);
	}
	else
	{
		Direct3DDeviceIMContext->DrawIndexed(IndexCount,StartIndex,BaseVertexIndex);
	}
}

void FD3D11DynamicRHI::RHIDrawIndexedPrimitiveIndirect(uint32 PrimitiveType,FIndexBufferRHIParamRef IndexBufferRHI,FVertexBufferRHIParamRef ArgumentBufferRHI,uint32 ArgumentOffset)
{
	DYNAMIC_CAST_D3D11RESOURCE(IndexBuffer,IndexBuffer);
	DYNAMIC_CAST_D3D11RESOURCE(VertexBuffer,ArgumentBuffer);

	RHI_DRAW_CALL_INC();

	GPUProfilingData.RegisterGPUWork(0);
	
	CommitNonComputeShaderConstants();
	
	// Set the index buffer.
	const uint32 SizeFormat = sizeof(DXGI_FORMAT);
	const DXGI_FORMAT Format = (IndexBuffer->GetStride() == sizeof(uint16) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT);
	StateCache.SetIndexBuffer(IndexBuffer->Resource, Format, 0);
	StateCache.SetPrimitiveTopology(GetD3D11PrimitiveType(PrimitiveType,bUsingTessellation));
	Direct3DDeviceIMContext->DrawIndexedInstancedIndirect(ArgumentBuffer->Resource,ArgumentOffset);
}

/**
 * Preallocate memory or get a direct command stream pointer to fill up for immediate rendering . This avoids memcpys below in DrawPrimitiveUP
 * @param PrimitiveType The type (triangles, lineloop, etc) of primitive to draw
 * @param NumPrimitives The number of primitives in the VertexData buffer
 * @param NumVertices The number of vertices to be written
 * @param VertexDataStride Size of each vertex 
 * @param OutVertexData Reference to the allocated vertex memory
 */
void FD3D11DynamicRHI::RHIBeginDrawPrimitiveUP( uint32 PrimitiveType, uint32 NumPrimitives, uint32 NumVertices, uint32 VertexDataStride, void*& OutVertexData)
{
	checkSlow( PendingNumVertices == 0 );

	// Remember the parameters for the draw call.
	PendingPrimitiveType = PrimitiveType;
	PendingNumPrimitives = NumPrimitives;
	PendingNumVertices = NumVertices;
	PendingVertexDataStride = VertexDataStride;

	// Map the dynamic buffer.
	OutVertexData = DynamicVB->Lock(NumVertices * VertexDataStride);
}

/**
 * Draw a primitive using the vertex data populated since RHIBeginDrawPrimitiveUP and clean up any memory as needed
 */
void FD3D11DynamicRHI::RHIEndDrawPrimitiveUP()
{
	RHI_DRAW_CALL_STATS(PendingPrimitiveType,PendingNumPrimitives);

	checkSlow(!bUsingTessellation || PendingPrimitiveType == PT_TriangleList);

	GPUProfilingData.RegisterGPUWork(PendingNumPrimitives,PendingNumVertices);

	// Unmap the dynamic vertex buffer.
	ID3D11Buffer* D3DBuffer = DynamicVB->Unlock();
	uint32 VBOffset = 0;

	// Issue the draw call.
	CommitNonComputeShaderConstants();
	StateCache.SetStreamSource(D3DBuffer, 0, PendingVertexDataStride, VBOffset);
	StateCache.SetPrimitiveTopology(GetD3D11PrimitiveType(PendingPrimitiveType,bUsingTessellation));
	Direct3DDeviceIMContext->Draw(PendingNumVertices,0);

	// Clear these parameters.
	PendingPrimitiveType = 0;
	PendingNumPrimitives = 0;
	PendingNumVertices = 0;
	PendingVertexDataStride = 0;
}

/**
 * Preallocate memory or get a direct command stream pointer to fill up for immediate rendering . This avoids memcpys below in DrawIndexedPrimitiveUP
 * @param PrimitiveType The type (triangles, lineloop, etc) of primitive to draw
 * @param NumPrimitives The number of primitives in the VertexData buffer
 * @param NumVertices The number of vertices to be written
 * @param VertexDataStride Size of each vertex
 * @param OutVertexData Reference to the allocated vertex memory
 * @param MinVertexIndex The lowest vertex index used by the index buffer
 * @param NumIndices Number of indices to be written
 * @param IndexDataStride Size of each index (either 2 or 4 bytes)
 * @param OutIndexData Reference to the allocated index memory
 */
void FD3D11DynamicRHI::RHIBeginDrawIndexedPrimitiveUP( uint32 PrimitiveType, uint32 NumPrimitives, uint32 NumVertices, uint32 VertexDataStride, void*& OutVertexData, uint32 MinVertexIndex, uint32 NumIndices, uint32 IndexDataStride, void*& OutIndexData)
{
	checkSlow((sizeof(uint16) == IndexDataStride) || (sizeof(uint32) == IndexDataStride));

	// Store off information needed for the draw call.
	PendingPrimitiveType = PrimitiveType;
	PendingNumPrimitives = NumPrimitives;
	PendingMinVertexIndex = MinVertexIndex;
	PendingIndexDataStride = IndexDataStride;
	PendingNumVertices = NumVertices;
	PendingNumIndices = NumIndices;
	PendingVertexDataStride = VertexDataStride;

	// Map dynamic vertex and index buffers.
	OutVertexData = DynamicVB->Lock(NumVertices * VertexDataStride);
	OutIndexData = DynamicIB->Lock(NumIndices * IndexDataStride);
}

/**
 * Draw a primitive using the vertex and index data populated since RHIBeginDrawIndexedPrimitiveUP and clean up any memory as needed
 */
void FD3D11DynamicRHI::RHIEndDrawIndexedPrimitiveUP()
{
	// tessellation only supports trilists
	checkSlow(!bUsingTessellation || PendingPrimitiveType == PT_TriangleList);

	RHI_DRAW_CALL_STATS(PendingPrimitiveType,PendingNumPrimitives);

	GPUProfilingData.RegisterGPUWork(PendingNumPrimitives,PendingNumVertices);

	// Unmap the dynamic buffers.
	ID3D11Buffer* VertexBuffer = DynamicVB->Unlock();
	ID3D11Buffer* IndexBuffer = DynamicIB->Unlock();
	uint32 VBOffset = 0;

	// Issue the draw call.
	CommitNonComputeShaderConstants();
	StateCache.SetStreamSource(VertexBuffer, 0, PendingVertexDataStride, VBOffset);
	StateCache.SetIndexBuffer(IndexBuffer, PendingIndexDataStride == sizeof(uint16) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, 0);
	StateCache.SetPrimitiveTopology(GetD3D11PrimitiveType(PendingPrimitiveType,bUsingTessellation));
	Direct3DDeviceIMContext->DrawIndexed(PendingNumIndices,0,PendingMinVertexIndex);

	// Clear these parameters.
	PendingPrimitiveType = 0;
	PendingNumPrimitives = 0;
	PendingMinVertexIndex = 0;
	PendingIndexDataStride = 0;
	PendingNumVertices = 0;
	PendingNumIndices = 0;
	PendingVertexDataStride = 0;
}

// Raster operations.
void FD3D11DynamicRHI::RHIClear(bool bClearColor,const FLinearColor& Color,bool bClearDepth,float Depth,bool bClearStencil,uint32 Stencil, FIntRect ExcludeRect)
{
	FD3D11DynamicRHI::RHIClearMRT(bClearColor, 1, &Color, bClearDepth, Depth, bClearStencil, Stencil, ExcludeRect);
}

void FD3D11DynamicRHI::RHIClearMRT(bool bClearColor,int32 NumClearColors,const FLinearColor* ClearColorArray,bool bClearDepth,float Depth,bool bClearStencil,uint32 Stencil, FIntRect ExcludeRect)
{
	// Helper struct to record and restore device states RHIClearMRT modifies.
	class FDeviceStateHelper
	{
		// New Monolithic Graphics drivers have optional "fast calls" replacing various D3d functions
		// Note that the FastXXX calls are in the new ID3D11DeviceContextX (derived from ID3D11DeviceContext1 which is derived from ID3D11DeviceContext)
		/** The global D3D device's immediate context */
		TRefCountPtr<FD3D11DeviceContext> Direct3DDeviceIMContext;

		enum { ResourceCount = D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT };
		//////////////////////////////////////////////////////////////////////////
		// Relevant recorded states:
		ID3D11ShaderResourceView* VertResources[ResourceCount];
		ID3D11VertexShader* VSOld;
		ID3D11PixelShader* PSOld;
		ID3D11DepthStencilState* OldDepthStencilState;
		ID3D11RasterizerState* OldRasterizerState;
		ID3D11BlendState* OldBlendState;
		uint32 StencilRef;
		float BlendFactor[4];
		uint32 SampleMask;
		//////////////////////////////////////////////////////////////////////////
		void ReleaseResources()
		{
			SAFE_RELEASE(VSOld);
			SAFE_RELEASE(PSOld);

			ID3D11ShaderResourceView** Resources = VertResources;
			for (int32 i = 0 ; i < ResourceCount; i++, Resources++)
			{
				SAFE_RELEASE(*Resources);
			}

			SAFE_RELEASE(OldDepthStencilState);
			SAFE_RELEASE(OldBlendState);
			SAFE_RELEASE(OldRasterizerState);
		}
	public:
		/** The global D3D device's immediate context */
		FDeviceStateHelper(TRefCountPtr<FD3D11DeviceContext> InDirect3DDeviceIMContext) : Direct3DDeviceIMContext(InDirect3DDeviceIMContext) {}

		void CaptureDeviceState(FD3D11StateCache& StateCache)
		{
			StateCache.GetVertexShader(&VSOld);
			StateCache.GetPixelShader(&PSOld);
			StateCache.GetShaderResourceViews<SF_Vertex>(0, ResourceCount, &VertResources[0]);
			StateCache.GetDepthStencilState(&OldDepthStencilState, &StencilRef);
			StateCache.GetBlendState(&OldBlendState, BlendFactor, &SampleMask);
			StateCache.GetRasterizerState(&OldRasterizerState);
		}

		void ClearCurrentVertexResources(FD3D11StateCache& StateCache)
		{
			static ID3D11ShaderResourceView* NullResources[ResourceCount] = {};
			for (int ResourceLoop = 0 ; ResourceLoop < ResourceCount; ResourceLoop++)
			{
				StateCache.SetShaderResourceView<SF_Vertex>(NullResources[0],0);
			}
		}

		void RestoreDeviceState(FD3D11StateCache& StateCache) 
		{

			// Restore the old shaders
			StateCache.SetVertexShader(VSOld);
			StateCache.SetPixelShader(PSOld);
			for (int ResourceLoop = 0 ; ResourceLoop < ResourceCount; ResourceLoop++)
				StateCache.SetShaderResourceView<SF_Vertex>(VertResources[ResourceLoop],ResourceLoop);
			StateCache.SetDepthStencilState(OldDepthStencilState, StencilRef);
			StateCache.SetBlendState(OldBlendState, BlendFactor, SampleMask);
			StateCache.SetRasterizerState(OldRasterizerState);

			ReleaseResources();
		}
	};

	{
		// <0: Auto
		int32 ClearWithExcludeRects = 2;
		
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		static const auto ExcludeRectCVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.ClearWithExcludeRects"));
		ClearWithExcludeRects = ExcludeRectCVar->GetValueOnRenderThread();
#endif

		if(ClearWithExcludeRects >= 2)
		{
			// by default use the exclude rect
			ClearWithExcludeRects = 1;

			if(IsRHIDeviceIntel())
			{
				// Disable exclude rect (Intel has fast clear so better we disable)
				ClearWithExcludeRects = 0;
			}
		}

		if(!ClearWithExcludeRects)
		{
			// Disable exclude rect
			ExcludeRect = FIntRect();
		}
	}

	FD3D11BoundRenderTargets BoundRenderTargets(Direct3DDeviceIMContext);

	// Must specify enough clear colors for all active RTs
	check(!bClearColor || NumClearColors >= BoundRenderTargets.GetNumActiveTargets());

	ID3D11DepthStencilView* DepthStencilView = BoundRenderTargets.GetDepthStencilView();

	// If we're clearing depth or stencil and we have a readonly depth/stencil view bound, we need to use a writable depth/stencil view instead
	if (CurrentDepthTexture)
	{
		// Create an access type mask by setting the readonly bits according to the bClearDepth/bClearStencil bools.
		EDepthStencilAccessType AccessMask = (EDepthStencilAccessType)((bClearDepth ? DSAT_Writable : DSAT_ReadOnlyDepth) | (bClearStencil ? DSAT_Writable : DSAT_ReadOnlyStencil));

		// Apply the mask to the current access state
		EDepthStencilAccessType RequiredAccess = (EDepthStencilAccessType)(CurrentDSVAccessType & AccessMask);

		if (RequiredAccess != CurrentDSVAccessType)
		{
			// A different access type is required than is currently set. Replace the DSV used for the clear.
			DepthStencilView = CurrentDepthTexture->GetDepthStencilView(RequiredAccess);
		}
	}

	// Determine if we're trying to clear a subrect of the screen
	bool UseDrawClear = false;
	uint32 NumViews = 1;
	D3D11_VIEWPORT Viewport;
	StateCache.GetViewports(&NumViews,&Viewport);
	if (Viewport.TopLeftX > 0 || Viewport.TopLeftY > 0)
	{
		UseDrawClear = true;
	}

/*	// possible optimization
	if(ExcludeRect.Width() > 0 && ExcludeRect.Height() > 0 && HardwareHasLinearClearPerformance) 
	{
		UseDrawClear = true;
	}
*/
	if(ExcludeRect.Min.X == 0 && ExcludeRect.Width() == Viewport.Width && ExcludeRect.Min.Y == 0 && ExcludeRect.Height() == Viewport.Height)
	{
		// no need to do anything
		return;
	}
	
	D3D11_RECT ScissorRect;
	uint32 NumRects = 1;
	Direct3DDeviceIMContext->RSGetScissorRects(&NumRects,&ScissorRect);
	if (ScissorRect.left > 0
		|| ScissorRect.right < Viewport.TopLeftX + Viewport.Width
		|| ScissorRect.top > 0
		|| ScissorRect.bottom < Viewport.TopLeftY + Viewport.Height)
	{
		UseDrawClear = true;
	}

	if (!UseDrawClear)
	{
		uint32 Width = 0;
		uint32 Height = 0;
		if (BoundRenderTargets.GetRenderTargetView(0))
		{
			ID3D11Texture2D* BaseTexture = NULL;
			BoundRenderTargets.GetRenderTargetView(0)->GetResource((ID3D11Resource**)&BaseTexture);
			D3D11_TEXTURE2D_DESC Desc;
			BaseTexture->GetDesc(&Desc);
			Width = Desc.Width;
			Height = Desc.Height;
			BaseTexture->Release();

			D3D11_RENDER_TARGET_VIEW_DESC RTVDesc;
			BoundRenderTargets.GetRenderTargetView(0)->GetDesc(&RTVDesc);
			if (RTVDesc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE1D ||
				RTVDesc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE1DARRAY ||
				RTVDesc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2D ||
				RTVDesc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2DARRAY ||
				RTVDesc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE3D)
			{
				// All the non-multisampled texture types have their mip-slice in the same position.
				uint32 MipIndex = RTVDesc.Texture2D.MipSlice;
				Width >>= MipIndex;
				Height >>= MipIndex;
			}
		}
		else if (DepthStencilView)
		{
			ID3D11Texture2D* BaseTexture = NULL;
			DepthStencilView->GetResource((ID3D11Resource**)&BaseTexture);
			D3D11_TEXTURE2D_DESC Desc;
			BaseTexture->GetDesc(&Desc);
			Width = Desc.Width;
			Height = Desc.Height;
			BaseTexture->Release();
		}

		if ((Viewport.Width < Width || Viewport.Height < Height) 
			&& (Viewport.Width > 1 && Viewport.Height > 1))
		{
			UseDrawClear = true;
		}
	}

	if (UseDrawClear)
	{
		if (CurrentDepthTexture)
		{
			// Clear all texture references to this depth buffer
			ConditionalClearShaderResource(CurrentDepthTexture);
		}

		// Build new states
		FBlendStateRHIParamRef BlendStateRHI;
		
		if (BoundRenderTargets.GetNumActiveTargets() <= 1)
		{
			BlendStateRHI = (bClearColor && BoundRenderTargets.GetRenderTargetView(0))
			? TStaticBlendState<>::GetRHI()
			: TStaticBlendState<CW_NONE>::GetRHI();
		}
		else
		{
			BlendStateRHI = (bClearColor && BoundRenderTargets.GetRenderTargetView(0))
				? TStaticBlendState<>::GetRHI()
				: TStaticBlendStateWriteMask<CW_NONE,CW_NONE,CW_NONE,CW_NONE,CW_NONE,CW_NONE,CW_NONE,CW_NONE>::GetRHI();
		}

		FRasterizerStateRHIParamRef RasterizerStateRHI = TStaticRasterizerState<FM_Solid,CM_None>::GetRHI();
		float BF[4] = {0,0,0,0};
		
		const FDepthStencilStateRHIParamRef DepthStencilStateRHI = 
			(bClearDepth && bClearStencil)
				? TStaticDepthStencilState<
					true, CF_Always,
					true,CF_Always,SO_Replace,SO_Replace,SO_Replace,
					false,CF_Always,SO_Replace,SO_Replace,SO_Replace,
					0xff,0xff
					>::GetRHI()
			: bClearDepth
				? TStaticDepthStencilState<true, CF_Always>::GetRHI()
			: bClearStencil
				? TStaticDepthStencilState<
					false, CF_Always,
					true,CF_Always,SO_Replace,SO_Replace,SO_Replace,
					false,CF_Always,SO_Replace,SO_Replace,SO_Replace,
					0xff,0xff
					>::GetRHI()
			:     TStaticDepthStencilState<false, CF_Always>::GetRHI();

		bool bChangedDepthStencilTarget = false;
		if (CurrentDepthTexture)
		{
			// Create an access type mask by setting the readonly bits according to the bClearDepth/bClearStencil bools.
			EDepthStencilAccessType AccessMask = (EDepthStencilAccessType)((bClearDepth ? DSAT_Writable : DSAT_ReadOnlyDepth) | (bClearStencil ? DSAT_Writable : DSAT_ReadOnlyStencil));

			// Apply the mask to the current access state
			EDepthStencilAccessType RequiredAccess = (EDepthStencilAccessType)(CurrentDSVAccessType & AccessMask);

			if (RequiredAccess != CurrentDSVAccessType)
			{
				bChangedDepthStencilTarget = true;
				CurrentDepthStencilTarget = CurrentDepthTexture->GetDepthStencilView(RequiredAccess);
				CommitRenderTargetsAndUAVs();
			} 
		}

		DYNAMIC_CAST_D3D11RESOURCE(BlendState,BlendState);
		DYNAMIC_CAST_D3D11RESOURCE(RasterizerState,RasterizerState);
		DYNAMIC_CAST_D3D11RESOURCE(DepthStencilState,DepthStencilState);

		// Store the current device state
		FDeviceStateHelper OriginalResourceState(Direct3DDeviceIMContext);
		OriginalResourceState.CaptureDeviceState(StateCache);

		// Set the cached state objects
		StateCache.SetBlendState(BlendState->Resource, BF, 0xffffffff);
		StateCache.SetDepthStencilState(DepthStencilState->Resource, Stencil);
		StateCache.SetRasterizerState(RasterizerState->Resource);
		OriginalResourceState.ClearCurrentVertexResources(StateCache);		

		// Set the new shaders
		TShaderMapRef<FOneColorVS> VertexShader(GetGlobalShaderMap());

		FOneColorPS* PixelShader = NULL;

		// Set the shader to write to the appropriate number of render targets
		// On AMD PC hardware, outputting to a color index in the shader without a matching render target set has a significant performance hit
		if (BoundRenderTargets.GetNumActiveTargets() <= 1)
		{
			TShaderMapRef<TOneColorPixelShaderMRT<1> > MRTPixelShader(GetGlobalShaderMap());
			PixelShader = *MRTPixelShader;
		}
		else if (BoundRenderTargets.GetNumActiveTargets() == 2)
		{
			TShaderMapRef<TOneColorPixelShaderMRT<2> > MRTPixelShader(GetGlobalShaderMap());
			PixelShader = *MRTPixelShader;
		}
		else if (BoundRenderTargets.GetNumActiveTargets() == 3)
		{
			TShaderMapRef<TOneColorPixelShaderMRT<3> > MRTPixelShader(GetGlobalShaderMap());
			PixelShader = *MRTPixelShader;
		}
		else if (BoundRenderTargets.GetNumActiveTargets() == 4)
		{
			TShaderMapRef<TOneColorPixelShaderMRT<4> > MRTPixelShader(GetGlobalShaderMap());
			PixelShader = *MRTPixelShader;
		}
		else if (BoundRenderTargets.GetNumActiveTargets() == 5)
		{
			TShaderMapRef<TOneColorPixelShaderMRT<5> > MRTPixelShader(GetGlobalShaderMap());
			PixelShader = *MRTPixelShader;
		}
		else if (BoundRenderTargets.GetNumActiveTargets() == 6)
		{
			TShaderMapRef<TOneColorPixelShaderMRT<6> > MRTPixelShader(GetGlobalShaderMap());
			PixelShader = *MRTPixelShader;
		}
		else if (BoundRenderTargets.GetNumActiveTargets() == 7)
		{
			TShaderMapRef<TOneColorPixelShaderMRT<7> > MRTPixelShader(GetGlobalShaderMap());
			PixelShader = *MRTPixelShader;
		}
		else if (BoundRenderTargets.GetNumActiveTargets() == 8)
		{
			TShaderMapRef<TOneColorPixelShaderMRT<8> > MRTPixelShader(GetGlobalShaderMap());
			PixelShader = *MRTPixelShader;
		}

		SetGlobalBoundShaderState(GD3D11ClearMRTBoundShaderState[FMath::Max(BoundRenderTargets.GetNumActiveTargets() - 1, 0)], GD3D11Vector4VertexDeclaration.VertexDeclarationRHI, *VertexShader, PixelShader);
		FLinearColor ShaderClearColors[MaxSimultaneousRenderTargets];
		FMemory::MemZero(ShaderClearColors);

		for (int32 i = 0; i < NumClearColors; i++)
		{
			ShaderClearColors[i] = ClearColorArray[i];
		}

		SetShaderValueArray(PixelShader->GetPixelShader(),PixelShader->ColorParameter,ShaderClearColors,NumClearColors);
		
		{
			// Draw a fullscreen quad
			if(ExcludeRect.Width() > 0 && ExcludeRect.Height() > 0)
			{
				// with a hole in it (optimization in case the hardware has non constant clear performance)
				FVector4 OuterVertices[4];
				OuterVertices[0].Set( -1.0f,  1.0f, Depth, 1.0f );
				OuterVertices[1].Set(  1.0f,  1.0f, Depth, 1.0f );
				OuterVertices[2].Set(  1.0f, -1.0f, Depth, 1.0f );
				OuterVertices[3].Set( -1.0f, -1.0f, Depth, 1.0f );

				float InvViewWidth = 1.0f / Viewport.Width;
				float InvViewHeight = 1.0f / Viewport.Height;
				FVector4 FractionRect = FVector4(ExcludeRect.Min.X * InvViewWidth, ExcludeRect.Min.Y * InvViewHeight, (ExcludeRect.Max.X - 1) * InvViewWidth, (ExcludeRect.Max.Y - 1) * InvViewHeight);

				FVector4 InnerVertices[4];
				InnerVertices[0].Set( FMath::Lerp(-1.0f,  1.0f, FractionRect.X), FMath::Lerp(1.0f, -1.0f, FractionRect.Y), Depth, 1.0f );
				InnerVertices[1].Set( FMath::Lerp(-1.0f,  1.0f, FractionRect.Z), FMath::Lerp(1.0f, -1.0f, FractionRect.Y), Depth, 1.0f );
				InnerVertices[2].Set( FMath::Lerp(-1.0f,  1.0f, FractionRect.Z), FMath::Lerp(1.0f, -1.0f, FractionRect.W), Depth, 1.0f );
				InnerVertices[3].Set( FMath::Lerp(-1.0f,  1.0f, FractionRect.X), FMath::Lerp(1.0f, -1.0f, FractionRect.W), Depth, 1.0f );
				
				FVector4 Vertices[10];
				Vertices[0] = OuterVertices[0];
				Vertices[1] = InnerVertices[0];
				Vertices[2] = OuterVertices[1];
				Vertices[3] = InnerVertices[1];
				Vertices[4] = OuterVertices[2];
				Vertices[5] = InnerVertices[2];
				Vertices[6] = OuterVertices[3];
				Vertices[7] = InnerVertices[3];
				Vertices[8] = OuterVertices[0];
				Vertices[9] = InnerVertices[0];

				RHIDrawPrimitiveUP(PT_TriangleStrip, 8, Vertices, sizeof(Vertices[0]) );
			}
			else
			{
				// without a hole
				FVector4 Vertices[4];
				Vertices[0].Set( -1.0f,  1.0f, Depth, 1.0f );
				Vertices[1].Set(  1.0f,  1.0f, Depth, 1.0f );
				Vertices[2].Set( -1.0f, -1.0f, Depth, 1.0f );
				Vertices[3].Set(  1.0f, -1.0f, Depth, 1.0f );
				RHIDrawPrimitiveUP(PT_TriangleStrip, 2, Vertices, sizeof(Vertices[0]) );
			}
		}

		if (bChangedDepthStencilTarget)
		{
			// Restore the DST
			CurrentDepthStencilTarget = CurrentDepthTexture->GetDepthStencilView(CurrentDSVAccessType);

			CommitRenderTargetsAndUAVs();
		}

		// Restore the original device state
		OriginalResourceState.RestoreDeviceState(StateCache); 
	}
	else
	{
		if (bClearColor && BoundRenderTargets.GetNumActiveTargets() > 0)
		{
			for (int32 TargetIndex = 0; TargetIndex < BoundRenderTargets.GetNumActiveTargets(); TargetIndex++)
			{
				Direct3DDeviceIMContext->ClearRenderTargetView(BoundRenderTargets.GetRenderTargetView(TargetIndex),(float*)&ClearColorArray[TargetIndex]);
			}
		}

		if ((bClearDepth || bClearStencil) && DepthStencilView)
		{
			uint32 ClearFlags = 0;
			if (bClearDepth)
			{
				ClearFlags |= D3D11_CLEAR_DEPTH;
			}
			if (bClearStencil)
			{
				ClearFlags |= D3D11_CLEAR_STENCIL;
			}
			Direct3DDeviceIMContext->ClearDepthStencilView(DepthStencilView,ClearFlags,Depth,Stencil);
		}
	}

	GPUProfilingData.RegisterGPUWork(0);
}

// Functions to yield and regain rendering control from D3D

void FD3D11DynamicRHI::RHISuspendRendering()
{
	// Not supported
}

void FD3D11DynamicRHI::RHIResumeRendering()
{
	// Not supported
}

bool FD3D11DynamicRHI::RHIIsRenderingSuspended()
{
	// Not supported
	return false;
}

// Blocks the CPU until the GPU catches up and goes idle.
void FD3D11DynamicRHI::RHIBlockUntilGPUIdle()
{
	// Not really supported
}

/*
 * Returns the total GPU time taken to render the last frame. Same metric as FPlatformTime::Cycles().
 */
uint32 FD3D11DynamicRHI::RHIGetGPUFrameCycles()
{
	return GGPUFrameTime;
}
