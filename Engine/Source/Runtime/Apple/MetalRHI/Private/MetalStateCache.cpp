// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "MetalRHIPrivate.h"
#include "MetalStateCache.h"
#include "MetalProfiler.h"
#include "ShaderCache.h"

static TAutoConsoleVariable<int32> CVarMetalVertexParameterSize(
	TEXT("r.MetalVertexParameterSize"),
	1024,
	TEXT("Amount of entries to use for VertexParameter space (multiples of 1024), defaults to 1024"),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarMetalPixelParameterSize(
	TEXT("r.MetalPixelParameterSize"),
	1024,
	TEXT("Amount of entries to use for PixelParameter space (multiples of 1024), defaults to 1024"),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarMetalComputeParameterSize(
	TEXT("r.MetalComputeParameterSize"),
	1024,
	TEXT("Amount of entries to use for ComputeParameter space (multiples of 1024), defaults to 1024"),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarMetalHullParameterSize(
	TEXT("r.MetalHullParameterSize"),
	1024,
	TEXT("Amount of entries to use for HullParameter space (multiples of 1024), defaults to 1024"),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarMetalDomainParameterSize(
	TEXT("r.MetalDomainParameterSize"),
	1024,
	TEXT("Amount of entries to use for DomainParameter space (multiples of 1024), defaults to 1024"),
	ECVF_Default);

static MTLTriangleFillMode TranslateFillMode(ERasterizerFillMode FillMode)
{
	switch (FillMode)
	{
		case FM_Wireframe:	return MTLTriangleFillModeLines;
		case FM_Point:		return MTLTriangleFillModeFill;
		default:			return MTLTriangleFillModeFill;
	};
}

static MTLCullMode TranslateCullMode(ERasterizerCullMode CullMode)
{
	switch (CullMode)
	{
		case CM_CCW:	return MTLCullModeFront;
		case CM_CW:		return MTLCullModeBack;
		default:		return MTLCullModeNone;
	}
}

FORCEINLINE MTLStoreAction GetMetalRTStoreAction(ERenderTargetStoreAction StoreAction)
{
	switch(StoreAction)
	{
		case ERenderTargetStoreAction::ENoAction: return MTLStoreActionDontCare;
		case ERenderTargetStoreAction::EStore: return MTLStoreActionStore;
		case ERenderTargetStoreAction::EMultisampleResolve: return MTLStoreActionMultisampleResolve;
		default: return MTLStoreActionDontCare;
	}
}

FORCEINLINE MTLStoreAction GetConditionalMetalRTStoreAction(ERenderTargetStoreAction StoreAction)
{
	switch(StoreAction)
	{
		case ERenderTargetStoreAction::EMultisampleResolve:
			return MTLStoreActionStoreAndMultisampleResolve;
			break;
		case ERenderTargetStoreAction::ENoAction:
		default:
			return MTLStoreActionStore;
			break;
	}
}

FMetalStateCache::FMetalStateCache(bool const bInImmediate)
: DepthStore(MTLStoreActionUnknown)
, StencilStore(MTLStoreActionUnknown)
, VisibilityResults(nil)
, VisibilityMode(MTLVisibilityResultModeDisabled)
, VisibilityOffset(0)
, BlendState(nullptr)
, DepthStencilState(nullptr)
, RasterizerState(nullptr)
, BoundShaderState(nullptr)
, StencilRef(0)
, BlendFactor(FLinearColor::Transparent)
, FrameBufferSize(CGSizeMake(0.0, 0.0))
, RenderTargetArraySize(1)
, RenderPassDesc(nil)
, RasterBits(0)
, bIsRenderTargetActive(false)
, bHasValidRenderTarget(false)
, bHasValidColorTarget(false)
, bScissorRectEnabled(false)
, bUsingTessellation(false)
, bCanRestartRenderPass(false)
, bImmediate(bInImmediate)
{
	Viewport.originX = Viewport.originY = Viewport.width = Viewport.height = Viewport.znear = Viewport.zfar = 0.0;
	Scissor.x = Scissor.y = Scissor.width = Scissor.height;
	
	for (uint32 i = 0; i < MaxMetalRenderTargets; i++)
	{
		ColorStore[i] = MTLStoreActionUnknown;
	}
	
	FMemory::Memzero(VertexBuffers, sizeof(VertexBuffers));
	FMemory::Memzero(RenderTargetsInfo);	
	FMemory::Memzero(DirtyUniformBuffers);
	
	//@todo-rco: What Size???
	// make a buffer for each shader type
	static IConsoleVariable* CVarMetalVertexParameterSize = IConsoleManager::Get().FindConsoleVariable(TEXT("r.MetalVertexParameterSize"));
	int SizeMult = CVarMetalVertexParameterSize->GetInt();
	ShaderParameters[CrossCompiler::SHADER_STAGE_VERTEX].InitializeResources(SizeMult * 1024);
	static IConsoleVariable* CVarMetalPixelParameterSize = IConsoleManager::Get().FindConsoleVariable(TEXT("r.MetalPixelParameterSize"));
	SizeMult = CVarMetalPixelParameterSize->GetInt();
	ShaderParameters[CrossCompiler::SHADER_STAGE_PIXEL].InitializeResources(SizeMult * 1024);
	if (GMaxRHIFeatureLevel >= ERHIFeatureLevel::SM4 )
	{
		static IConsoleVariable* CVarMetalComputeParameterSize = IConsoleManager::Get().FindConsoleVariable(TEXT("r.MetalComputeParameterSize"));
		SizeMult = CVarMetalComputeParameterSize->GetInt();
		ShaderParameters[CrossCompiler::SHADER_STAGE_COMPUTE].InitializeResources(SizeMult * 1024);
	}
	if (GMaxRHIFeatureLevel >= ERHIFeatureLevel::SM5)
	{
		static IConsoleVariable* CVarMetalHullParameterSize = IConsoleManager::Get().FindConsoleVariable(TEXT("r.MetalHullParameterSize"));
		SizeMult = CVarMetalHullParameterSize->GetInt();
		ShaderParameters[CrossCompiler::SHADER_STAGE_HULL].InitializeResources(SizeMult * 1024);
		
		static IConsoleVariable* CVarMetalDomainParameterSize = IConsoleManager::Get().FindConsoleVariable(TEXT("r.MetalDomainParameterSize"));
		SizeMult = CVarMetalDomainParameterSize->GetInt();
		ShaderParameters[CrossCompiler::SHADER_STAGE_DOMAIN].InitializeResources(SizeMult * 1024);
	}
	
	FMemory::Memzero(ShaderBuffers, sizeof(ShaderBuffers));
	FMemory::Memzero(ShaderTextures, sizeof(ShaderTextures));
	
	for (uint32 Frequency = 0; Frequency < SF_NumFrequencies; Frequency++)
	{
		ShaderSamplers[Frequency].Bound = 0;
		for (uint32 i = 0; i < ML_MaxSamplers; i++)
		{
			ShaderSamplers[Frequency].Samplers[i].SafeRelease();
		}
	}
}

FMetalStateCache::~FMetalStateCache()
{
	[RenderPassDesc release];
	RenderPassDesc = nil;
}

void FMetalStateCache::Reset(void)
{
	for (uint32 i = 0; i < CrossCompiler::NUM_SHADER_STAGES; i++)
	{
		ShaderParameters[i].MarkAllDirty();
	}
	for (uint32 i = 0; i < SF_NumFrequencies; i++)
	{
		BoundUniformBuffers[i].Empty();
	}
	
	SetStateDirty();
	
	PipelineDesc.Hash.RasterBits = 0;
	PipelineDesc.Hash.TargetBits = 0;
	if (PipelineDesc.PipelineDescriptor)
	{
		PipelineDesc.PipelineDescriptor.depthAttachmentPixelFormat = MTLPixelFormatInvalid;
	}
	
	Viewport.originX = Viewport.originY = Viewport.width = Viewport.height = Viewport.znear = Viewport.zfar = 0.0;
	
	FMemory::Memzero(RenderTargetsInfo);
	bIsRenderTargetActive = false;
	bHasValidRenderTarget = false;
	bHasValidColorTarget = false;
	
	FMemory::Memzero(DirtyUniformBuffers);
	
	FMemory::Memzero(VertexBuffers, sizeof(VertexBuffers));
	FMemory::Memzero(ShaderBuffers, sizeof(ShaderBuffers));
	FMemory::Memzero(ShaderTextures, sizeof(ShaderTextures));
	
	for (uint32 Frequency = 0; Frequency < SF_NumFrequencies; Frequency++)
	{
		ShaderSamplers[Frequency].Bound = 0;
		for (uint32 i = 0; i < ML_MaxSamplers; i++)
		{
			ShaderSamplers[Frequency].Samplers[i].SafeRelease();
		}
	}
	
	VisibilityResults = nil;
	VisibilityMode = MTLVisibilityResultModeDisabled;
	VisibilityOffset = 0;
	
	BlendState.SafeRelease();
	DepthStencilState.SafeRelease();
	RasterizerState.SafeRelease();
	BoundShaderState.SafeRelease();
	ComputeShader.SafeRelease();
	DepthStencilSurface.SafeRelease();
	StencilRef = 0;
	
	[RenderPassDesc release];
	RenderPassDesc = nil;
	
	for (uint32 i = 0; i < MaxMetalRenderTargets; i++)
	{
		ColorStore[i] = MTLStoreActionUnknown;
	}
	DepthStore = MTLStoreActionUnknown;
	StencilStore = MTLStoreActionUnknown;
	
	BlendFactor = FLinearColor::Transparent;
	FrameBufferSize = CGSizeMake(0.0, 0.0);
	RenderTargetArraySize = 0;
    bUsingTessellation = false;
    bCanRestartRenderPass = false;
}

static bool MTLScissorRectEqual(MTLScissorRect const& Left, MTLScissorRect const& Right)
{
	return Left.x == Right.x && Left.y == Right.y && Left.width == Right.width && Left.height == Right.height;
}

void FMetalStateCache::SetScissorRect(bool const bEnable, MTLScissorRect const& Rect)
{
	if (bScissorRectEnabled != bEnable || !MTLScissorRectEqual(Scissor, Rect))
	{
		bScissorRectEnabled = bEnable;
		if (bEnable)
		{
			Scissor = Rect;
		}
		else
		{
			Scissor.x = Viewport.originX;
			Scissor.y = Viewport.originY;
			Scissor.width = Viewport.width;
			Scissor.height = Viewport.height;
		}
		
		// Clamp to framebuffer size - Metal doesn't allow scissor to be larger.
		Scissor.x = Scissor.x;
		Scissor.y = Scissor.y;
		Scissor.width = FMath::Max((Scissor.x + Scissor.width <= FMath::RoundToInt(FrameBufferSize.width)) ? Scissor.width : FMath::RoundToInt(FrameBufferSize.width) - Scissor.x, (NSUInteger)1u);
		Scissor.height = FMath::Max((Scissor.y + Scissor.height <= FMath::RoundToInt(FrameBufferSize.height)) ? Scissor.height : FMath::RoundToInt(FrameBufferSize.height) - Scissor.y, (NSUInteger)1u);
		
		RasterBits |= EMetalRenderFlagScissorRect;
	}
}

void FMetalStateCache::SetBlendFactor(FLinearColor const& InBlendFactor)
{
	if(BlendFactor != InBlendFactor) // @todo zebra
	{
		BlendFactor = InBlendFactor;
		RasterBits |= EMetalRenderFlagBlendColor;
	}
}

void FMetalStateCache::SetStencilRef(uint32 const InStencilRef)
{
	if(StencilRef != InStencilRef) // @todo zebra
	{
		StencilRef = InStencilRef;
		RasterBits |= EMetalRenderFlagStencilReferenceValue;
	}
}

void FMetalStateCache::SetBlendState(FMetalBlendState* InBlendState)
{
	//current equality operator is comparing pointers which can be re-used causing an incorrect comparison.
	//removing caching here until equality operator is updated.
	// if(BlendState != InBlendState) // @todo zebra
	{
		BlendState = InBlendState;
		if(InBlendState)
		{
			for(uint32 RenderTargetIndex = 0;RenderTargetIndex < MaxMetalRenderTargets; ++RenderTargetIndex)
			{
				MTLRenderPipelineColorAttachmentDescriptor* Blend = BlendState->RenderTargetStates[RenderTargetIndex].BlendState;
				MTLRenderPipelineColorAttachmentDescriptor* Dest = [PipelineDesc.PipelineDescriptor.colorAttachments objectAtIndexedSubscript:RenderTargetIndex];

				if(Blend && Dest)
				{
					// assign each property manually, would be nice if this was faster
					Dest.blendingEnabled = Blend.blendingEnabled;
					Dest.sourceRGBBlendFactor = Blend.sourceRGBBlendFactor;
					Dest.destinationRGBBlendFactor = Blend.destinationRGBBlendFactor;
					Dest.rgbBlendOperation = Blend.rgbBlendOperation;
					Dest.sourceAlphaBlendFactor = Blend.sourceAlphaBlendFactor;
					Dest.destinationAlphaBlendFactor = Blend.destinationAlphaBlendFactor;
					Dest.alphaBlendOperation = Blend.alphaBlendOperation;
					Dest.writeMask = Blend.writeMask;
				}
		
				// set the hash bits for this RT
				PipelineDesc.SetHashValue(FMetalRenderPipelineDesc::BlendBitOffsets[RenderTargetIndex], NumBits_BlendState, BlendState->RenderTargetStates[RenderTargetIndex].BlendStateKey);
			}
		}
	}
}

void FMetalStateCache::SetDepthStencilState(FMetalDepthStencilState* InDepthStencilState)
{
	if(DepthStencilState != InDepthStencilState) // @todo zebra
	{
		DepthStencilState = InDepthStencilState;
		RasterBits |= EMetalRenderFlagDepthStencilState;
	}
}

void FMetalStateCache::SetRasterizerState(FMetalRasterizerState* InRasterizerState)
{
	if(RasterizerState != InRasterizerState) // @todo zebra
	{
		RasterizerState = InRasterizerState;
		RasterBits |= EMetalRenderFlagFrontFacingWinding|EMetalRenderFlagCullMode|EMetalRenderFlagDepthBias|EMetalRenderFlagTriangleFillMode;
	}
}

void FMetalStateCache::SetBoundShaderState(FMetalBoundShaderState* InBoundShaderState)
{
	if(BoundShaderState != InBoundShaderState) // @todo zebra
	{
		BoundShaderState = InBoundShaderState;
		
		PipelineState = nil;
		RasterBits |= EMetalRenderFlagPipelineState;
        
        bool bNewUsingTessellation = (BoundShaderState && BoundShaderState->HullShader);
        if (bNewUsingTessellation != bUsingTessellation)
        {
        	for (uint32 i = 0; i < SF_NumFrequencies; i++)
			{
				ShaderBuffers[i].Bound = UINT32_MAX;
		#if PLATFORM_MAC
		#ifndef UINT128_MAX
		#define UINT128_MAX (((__uint128_t)1 << 127) - (__uint128_t)1 + ((__uint128_t)1 << 127))
		#endif
				ShaderTextures[i].Bound = UINT128_MAX;
		#else
				ShaderTextures[i].Bound = UINT32_MAX;
		#endif
				ShaderSamplers[i].Bound = UINT32_MAX;
			}
        }
		// Whenever the pipeline changes & a Hull shader is bound clear the Hull shader bindings, otherwise the Hull resources from a
		// previous pipeline with different binding table will overwrite the vertex shader bindings for the current pipeline.
		if (bNewUsingTessellation)
		{
			ShaderBuffers[SF_Hull].Bound = UINT32_MAX;
#if PLATFORM_MAC
			ShaderTextures[SF_Hull].Bound = UINT128_MAX;
#else
			ShaderTextures[SF_Hull].Bound = UINT32_MAX;
#endif
			ShaderSamplers[SF_Hull].Bound = UINT32_MAX;
			FMemory::Memzero(ShaderBuffers[SF_Hull].Buffers, sizeof(ShaderBuffers[SF_Hull].Buffers));
			FMemory::Memzero(ShaderTextures[SF_Hull].Textures, sizeof(ShaderTextures[SF_Hull].Textures));
			for (uint32 i = 0; i < ML_MaxSamplers; i++)
			{
				ShaderSamplers[SF_Hull].Samplers[i].SafeRelease();
			}
		}
        bUsingTessellation = bNewUsingTessellation;
		
		DirtyUniformBuffers[SF_Vertex] = 0xffffffff;
		DirtyUniformBuffers[SF_Pixel] = 0xffffffff;
		DirtyUniformBuffers[SF_Hull] = 0xffffffff;
		DirtyUniformBuffers[SF_Domain] = 0xffffffff;
		DirtyUniformBuffers[SF_Geometry] = 0xffffffff;
	}
}

void FMetalStateCache::SetComputeShader(FMetalComputeShader* InComputeShader)
{
	if(ComputeShader != InComputeShader) // @todo zebra
	{
		ComputeShader = InComputeShader;
		
		bUsingTessellation = false;
		
		DirtyUniformBuffers[SF_Compute] = 0xffffffff;
	}
}

bool FMetalStateCache::SetRenderTargetsInfo(FRHISetRenderTargetsInfo const& InRenderTargets, id<MTLBuffer> const QueryBuffer, bool const bReset)
{
	bool bNeedsSet = false;
	
	// see if our new Info matches our previous Info
	if (NeedsToSetRenderTarget(InRenderTargets) || QueryBuffer != VisibilityResults)
	{
		bool bNeedsClear = false;
		
		// Deferred store actions make life a bit easier...
		static bool bSupportsDeferredStore = GetMetalDeviceContext().GetCommandQueue().SupportsFeature(EMetalFeaturesDeferredStoreActions);
		
		//Create local store action states if we support deferred store 
		MTLStoreAction NewColorStore[MaxMetalRenderTargets];
		for (uint32 i = 0; i < MaxMetalRenderTargets; ++i)
		{
			NewColorStore[i] = MTLStoreActionUnknown;
		}
		
		MTLStoreAction NewDepthStore = MTLStoreActionUnknown;
		MTLStoreAction NewStencilStore = MTLStoreActionUnknown;
		
		// back this up for next frame
		RenderTargetsInfo = InRenderTargets;
		
		// at this point, we need to fully set up an encoder/command buffer, so make a new one (autoreleased)
		MTLRenderPassDescriptor* RenderPass = [MTLRenderPassDescriptor renderPassDescriptor];
		TRACK_OBJECT(STAT_MetalRenderPassDescriptorCount, RenderPass);
	
		// if we need to do queries, write to the supplied query buffer
		if (IsFeatureLevelSupported(GMaxRHIShaderPlatform, ERHIFeatureLevel::SM4))
		{
			VisibilityResults = QueryBuffer;
			RenderPass.visibilityResultBuffer = QueryBuffer;
		}
		else
		{
			VisibilityResults = NULL;
			RenderPass.visibilityResultBuffer = NULL;
		}
	
		// default to non-msaa
	    int32 OldCount = PipelineDesc.SampleCount;
		PipelineDesc.SampleCount = 0;
	
		bIsRenderTargetActive = false;
		bHasValidRenderTarget = false;
		bHasValidColorTarget = false;
		
		uint8 ArrayTargets = 0;
		uint8 BoundTargets = 0;
		uint32 ArrayRenderLayers = UINT_MAX;
		
		bool bFramebufferSizeSet = false;
		FrameBufferSize = CGSizeMake(0.f, 0.f);
		
		bCanRestartRenderPass = true;
		
		for (uint32 RenderTargetIndex = 0; RenderTargetIndex < MaxMetalRenderTargets; RenderTargetIndex++)
		{
			// default to invalid
			uint8 FormatKey = 0;
			// only try to set it if it was one that was set (ie less than RenderTargetsInfo.NumColorRenderTargets)
			if (RenderTargetIndex < RenderTargetsInfo.NumColorRenderTargets && RenderTargetsInfo.ColorRenderTarget[RenderTargetIndex].Texture != nullptr)
			{
				const FRHIRenderTargetView& RenderTargetView = RenderTargetsInfo.ColorRenderTarget[RenderTargetIndex];
				FMetalSurface& Surface = *GetMetalSurfaceFromRHITexture(RenderTargetView.Texture);
				FormatKey = Surface.FormatKey;
				
				uint32 Width = FMath::Max((uint32)(Surface.SizeX >> RenderTargetView.MipIndex), (uint32)1);
				uint32 Height = FMath::Max((uint32)(Surface.SizeY >> RenderTargetView.MipIndex), (uint32)1);
				if(!bFramebufferSizeSet)
				{
					bFramebufferSizeSet = true;
					FrameBufferSize.width = Width;
					FrameBufferSize.height = Height;
				}
				else
				{
					FrameBufferSize.width = FMath::Min(FrameBufferSize.width, (CGFloat)Width);
					FrameBufferSize.height = FMath::Min(FrameBufferSize.height, (CGFloat)Height);
				}
	
				// if this is the back buffer, make sure we have a usable drawable
				ConditionalUpdateBackBuffer(Surface);
	
				BoundTargets |= 1 << RenderTargetIndex;
            
#if !PLATFORM_MAC
                if (Surface.Texture == nil)
                {
                    PipelineDesc.SampleCount = OldCount;
                    bCanRestartRenderPass &= (OldCount <= 1);
                    return true;
                }
#endif
				
				// The surface cannot be nil - we have to have a valid render-target array after this call.
				check (Surface.Texture != nil);
	
				// user code generally passes -1 as a default, but we need 0
				uint32 ArraySliceIndex = RenderTargetView.ArraySliceIndex == 0xFFFFFFFF ? 0 : RenderTargetView.ArraySliceIndex;
				if (Surface.bIsCubemap)
				{
					ArraySliceIndex = GetMetalCubeFace((ECubeFace)ArraySliceIndex);
				}
				
				switch(Surface.Type)
				{
					case RRT_Texture2DArray:
					case RRT_Texture3D:
					case RRT_TextureCube:
						if(RenderTargetView.ArraySliceIndex == 0xFFFFFFFF)
						{
							ArrayTargets |= (1 << RenderTargetIndex);
							ArrayRenderLayers = FMath::Min(ArrayRenderLayers, Surface.GetNumFaces());
						}
						else
						{
							ArrayRenderLayers = 1;
						}
						break;
					default:
						ArrayRenderLayers = 1;
						break;
				}
	
				MTLRenderPassColorAttachmentDescriptor* ColorAttachment = [MTLRenderPassColorAttachmentDescriptor new];
				TRACK_OBJECT(STAT_MetalRenderPassColorAttachmentDescriptorCount, ColorAttachment);
	
				if (Surface.MSAATexture != nil)
				{
					// set up an MSAA attachment
					ColorAttachment.texture = Surface.MSAATexture;
					NewColorStore[RenderTargetIndex] = MTLStoreActionMultisampleResolve;
					ColorAttachment.storeAction = bSupportsDeferredStore ? MTLStoreActionUnknown : NewColorStore[RenderTargetIndex];
					ColorAttachment.resolveTexture = Surface.Texture;
					PipelineDesc.SampleCount = Surface.MSAATexture.sampleCount;
	
					// only allow one MRT with msaa
					checkf(RenderTargetsInfo.NumColorRenderTargets == 1, TEXT("Only expected one MRT when using MSAA"));
				}
				else
				{
					// set up non-MSAA attachment
					ColorAttachment.texture = Surface.Texture;
					NewColorStore[RenderTargetIndex] = GetMetalRTStoreAction(RenderTargetView.StoreAction);
					ColorAttachment.storeAction = bSupportsDeferredStore ? MTLStoreActionUnknown : NewColorStore[RenderTargetIndex];
					PipelineDesc.SampleCount = 1;
				}
				
				ColorAttachment.level = RenderTargetView.MipIndex;
				if(Surface.Type == RRT_Texture3D)
				{
					ColorAttachment.depthPlane = ArraySliceIndex;
				}
				else
				{
					ColorAttachment.slice = ArraySliceIndex;
				}
				
				ColorAttachment.loadAction = (Surface.Written || !bImmediate) ? GetMetalRTLoadAction(RenderTargetView.LoadAction) : MTLLoadActionClear;
				FPlatformAtomics::InterlockedExchange(&Surface.Written, 1);
				
				bNeedsClear |= (ColorAttachment.loadAction == MTLLoadActionClear);
				
				const FClearValueBinding& ClearValue = RenderTargetsInfo.ColorRenderTarget[RenderTargetIndex].Texture->GetClearBinding();
				if (ClearValue.ColorBinding == EClearBinding::EColorBound)
				{
					const FLinearColor& ClearColor = ClearValue.GetClearColor();
					ColorAttachment.clearColor = MTLClearColorMake(ClearColor.R, ClearColor.G, ClearColor.B, ClearColor.A);
				}

				// assign the attachment to the slot
				[RenderPass.colorAttachments setObject:ColorAttachment atIndexedSubscript:RenderTargetIndex];
				[PipelineDesc.PipelineDescriptor.colorAttachments objectAtIndexedSubscript:RenderTargetIndex].pixelFormat = ColorAttachment.texture.pixelFormat;
				
				bCanRestartRenderPass &= (PipelineDesc.SampleCount <= 1) && (ColorAttachment.loadAction == MTLLoadActionLoad) && (RenderTargetView.StoreAction == ERenderTargetStoreAction::EStore);
	
				UNTRACK_OBJECT(STAT_MetalRenderPassColorAttachmentDescriptorCount, ColorAttachment);
				[ColorAttachment release];
	
				bHasValidRenderTarget = true;
				bHasValidColorTarget = true;
			}
			else
			{
				[PipelineDesc.PipelineDescriptor.colorAttachments objectAtIndexedSubscript:RenderTargetIndex].pixelFormat = MTLPixelFormatInvalid;
			}
	
			// update the hash no matter what case (null, unused, used)
			PipelineDesc.SetHashValue(FMetalRenderPipelineDesc::RTBitOffsets[RenderTargetIndex], NumBits_RenderTargetFormat, FormatKey);
		}
		
		RenderTargetArraySize = 1;
		
		if(ArrayTargets)
		{
			if (!GetMetalDeviceContext().SupportsFeature(EMetalFeaturesLayeredRendering))
			{
				if (ArrayRenderLayers != 1)
				{
					UE_LOG(LogMetal, Fatal, TEXT("Layered rendering is unsupported on this device."));
				}
			}
#if PLATFORM_MAC
			else
			{
				if (ArrayTargets == BoundTargets)
				{
					RenderTargetArraySize = ArrayRenderLayers;
					RenderPass.renderTargetArrayLength = ArrayRenderLayers;
				}
				else
				{
					UE_LOG(LogMetal, Fatal, TEXT("All color render targets must be layered when performing multi-layered rendering under Metal."));
				}
			}
#endif
		}
	
		// default to invalid
		PipelineDesc.PipelineDescriptor.depthAttachmentPixelFormat = MTLPixelFormatInvalid;
		PipelineDesc.PipelineDescriptor.stencilAttachmentPixelFormat = MTLPixelFormatInvalid;
		
		uint8 DepthFormatKey = 0;
		uint8 StencilFormatKey = 0;
	
		// setup depth and/or stencil
		if (RenderTargetsInfo.DepthStencilRenderTarget.Texture != nullptr)
		{
			FMetalSurface& Surface = *GetMetalSurfaceFromRHITexture(RenderTargetsInfo.DepthStencilRenderTarget.Texture);
			
			switch(Surface.Type)
			{
				case RRT_Texture2DArray:
				case RRT_Texture3D:
				case RRT_TextureCube:
					ArrayRenderLayers = Surface.GetNumFaces();
					break;
				default:
					ArrayRenderLayers = 1;
					break;
			}
			if(!ArrayTargets && ArrayRenderLayers > 1)
			{
				if (!GetMetalDeviceContext().SupportsFeature(EMetalFeaturesLayeredRendering))
				{
					UE_LOG(LogMetal, Fatal, TEXT("Layered rendering is unsupported on this device."));
				}
#if PLATFORM_MAC
				else
				{
					RenderTargetArraySize = ArrayRenderLayers;
					RenderPass.renderTargetArrayLength = ArrayRenderLayers;
				}
#endif
			}
			
			if(!bFramebufferSizeSet)
			{
				bFramebufferSizeSet = true;
				FrameBufferSize.width = Surface.SizeX;
				FrameBufferSize.height = Surface.SizeY;
			}
			else
			{
				FrameBufferSize.width = FMath::Min(FrameBufferSize.width, (CGFloat)Surface.SizeX);
				FrameBufferSize.height = FMath::Min(FrameBufferSize.height, (CGFloat)Surface.SizeY);
			}
			
			EPixelFormat DepthStencilPixelFormat = RenderTargetsInfo.DepthStencilRenderTarget.Texture->GetFormat();
			
			id<MTLTexture> DepthTexture = nil;
			id<MTLTexture> StencilTexture = nil;
			
			switch (DepthStencilPixelFormat)
			{
				case PF_X24_G8:
				case PF_DepthStencil:
				case PF_D24:
				{
					MTLPixelFormat DepthStencilFormat = Surface.Texture ? Surface.Texture.pixelFormat : MTLPixelFormatInvalid;
					
					switch(DepthStencilFormat)
					{
						case MTLPixelFormatDepth32Float:
							DepthTexture = Surface.Texture;
#if !PLATFORM_MAC
							StencilTexture = (DepthStencilPixelFormat == PF_DepthStencil) ? Surface.StencilTexture : nil;
#endif
							break;
						case MTLPixelFormatStencil8:
							StencilTexture = Surface.Texture;
							break;
						case MTLPixelFormatDepth32Float_Stencil8:
							DepthTexture = Surface.Texture;
							StencilTexture = Surface.Texture;
							break;
#if PLATFORM_MAC
						case MTLPixelFormatDepth24Unorm_Stencil8:
							DepthTexture = Surface.Texture;
							StencilTexture = Surface.Texture;
							break;
#endif
						default:
							break;
					}
					
					break;
				}
				case PF_ShadowDepth:
				{
					DepthTexture = Surface.Texture;
					break;
				}
				default:
					break;
			}
			
			float DepthClearValue = 0.0f;
			uint32 StencilClearValue = 0;
			const FClearValueBinding& ClearValue = RenderTargetsInfo.DepthStencilRenderTarget.Texture->GetClearBinding();
			if (ClearValue.ColorBinding == EClearBinding::EDepthStencilBound)
			{
				ClearValue.GetDepthStencil(DepthClearValue, StencilClearValue);
			}
			else if(!ArrayTargets && ArrayRenderLayers > 1)
			{
				DepthClearValue = 1.0f;
			}

			if (DepthTexture)
			{
				MTLRenderPassDepthAttachmentDescriptor* DepthAttachment = [[MTLRenderPassDepthAttachmentDescriptor alloc] init];
				TRACK_OBJECT(STAT_MetalRenderPassDepthAttachmentDescriptorCount, DepthAttachment);
				
				DepthFormatKey = Surface.FormatKey;
	
				// set up the depth attachment
				DepthAttachment.texture = DepthTexture;
				DepthAttachment.loadAction = GetMetalRTLoadAction(RenderTargetsInfo.DepthStencilRenderTarget.DepthLoadAction);
				
				bNeedsClear |= (DepthAttachment.loadAction == MTLLoadActionClear);
				
				NewDepthStore = GetMetalRTStoreAction(RenderTargetsInfo.DepthStencilRenderTarget.DepthStoreAction);
				DepthAttachment.storeAction = bSupportsDeferredStore ? MTLStoreActionUnknown : NewDepthStore;
				DepthAttachment.clearDepth = DepthClearValue;

				PipelineDesc.PipelineDescriptor.depthAttachmentPixelFormat = DepthAttachment.texture.pixelFormat;
				if (PipelineDesc.SampleCount == 0)
				{
					PipelineDesc.SampleCount = DepthAttachment.texture.sampleCount;
				}
				
				bHasValidRenderTarget = true;

				bCanRestartRenderPass &= (PipelineDesc.SampleCount <= 1) && ((RenderTargetsInfo.DepthStencilRenderTarget.Texture == FallbackDepthStencilSurface) || ((DepthAttachment.loadAction == MTLLoadActionLoad) && (!RenderTargetsInfo.DepthStencilRenderTarget.GetDepthStencilAccess().IsDepthWrite() || (RenderTargetsInfo.DepthStencilRenderTarget.DepthStoreAction == ERenderTargetStoreAction::ENoAction) || (RenderTargetsInfo.DepthStencilRenderTarget.DepthStoreAction == ERenderTargetStoreAction::EStore))));
				
				// and assign it
				RenderPass.depthAttachment = DepthAttachment;
				
				UNTRACK_OBJECT(STAT_MetalRenderPassDepthAttachmentDescriptorCount, DepthAttachment);
				[DepthAttachment release];
			}
	
			if (StencilTexture)
			{
				MTLRenderPassStencilAttachmentDescriptor* StencilAttachment = [[MTLRenderPassStencilAttachmentDescriptor alloc] init];
				TRACK_OBJECT(STAT_MetalRenderPassStencilAttachmentDescriptorCount, StencilAttachment);
				
				StencilFormatKey = Surface.FormatKey;
	
				// set up the stencil attachment
				StencilAttachment.texture = StencilTexture;
				StencilAttachment.loadAction = GetMetalRTLoadAction(RenderTargetsInfo.DepthStencilRenderTarget.StencilLoadAction);
				
				bNeedsClear |= (StencilAttachment.loadAction == MTLLoadActionClear);
				
				NewStencilStore = GetMetalRTStoreAction(RenderTargetsInfo.DepthStencilRenderTarget.GetStencilStoreAction());
				StencilAttachment.storeAction = bSupportsDeferredStore ? MTLStoreActionUnknown : NewStencilStore;
				StencilAttachment.clearStencil = StencilClearValue;

				PipelineDesc.PipelineDescriptor.stencilAttachmentPixelFormat = StencilAttachment.texture.pixelFormat;
				if (PipelineDesc.SampleCount == 0)
				{
					PipelineDesc.SampleCount = StencilAttachment.texture.sampleCount;
				}
				
				bHasValidRenderTarget = true;
				
				// @todo Stencil writes that need to persist must use ERenderTargetStoreAction::EStore on iOS.
				// We should probably be using deferred store actions so that we can safely lazily instantiate encoders.
				bCanRestartRenderPass &= (PipelineDesc.SampleCount <= 1) && ((RenderTargetsInfo.DepthStencilRenderTarget.Texture == FallbackDepthStencilSurface) || ((StencilAttachment.loadAction == MTLLoadActionLoad) && (1 || !RenderTargetsInfo.DepthStencilRenderTarget.GetDepthStencilAccess().IsStencilWrite() || (RenderTargetsInfo.DepthStencilRenderTarget.GetStencilStoreAction() == ERenderTargetStoreAction::EStore))));
				
				// and assign it
				RenderPass.stencilAttachment = StencilAttachment;
				
				UNTRACK_OBJECT(STAT_MetalRenderPassStencilAttachmentDescriptorCount, StencilAttachment);
				[StencilAttachment release];
			}
		}
		
		//Update deferred store states if required otherwise they're already set directly on the Metal Attachement Descriptors
		if (bSupportsDeferredStore)
		{
			for (uint32 i = 0; i < MaxMetalRenderTargets; ++i)
			{
				ColorStore[i] = NewColorStore[i];
			}
			DepthStore = NewDepthStore;
			StencilStore = NewStencilStore;
		}
		
		bHasValidRenderTarget |= (InRenderTargets.NumUAVs > 0);
		if (PipelineDesc.SampleCount == 0)
		{
			PipelineDesc.SampleCount = 1;
		}
		
		bIsRenderTargetActive = bHasValidRenderTarget;
		
		// Only start encoding if the render target state is valid
		if (bHasValidRenderTarget)
		{
			// Retain and/or release the depth-stencil surface in case it is a temporary surface for a draw call that writes to depth without a depth/stencil buffer bound.
			DepthStencilSurface = RenderTargetsInfo.DepthStencilRenderTarget.Texture;
			
			// update hash for the depth/stencil buffer & sample count
			PipelineDesc.SetHashValue(Offset_DepthFormat, NumBits_DepthFormat, DepthFormatKey);
			PipelineDesc.SetHashValue(Offset_StencilFormat, NumBits_StencilFormat, StencilFormatKey);
			PipelineDesc.SetHashValue(Offset_SampleCount, NumBits_SampleCount, PipelineDesc.SampleCount);
		}
		else
		{
			// update hash for the depth/stencil buffer & sample count
			PipelineDesc.SetHashValue(Offset_DepthFormat, NumBits_DepthFormat, 0);
			PipelineDesc.SetHashValue(Offset_StencilFormat, NumBits_StencilFormat, 0);
			PipelineDesc.SetHashValue(Offset_SampleCount, NumBits_SampleCount, 0);
			
			DepthStencilSurface.SafeRelease();
		}
		
//		if (bReset)
//		{
//			// Reset any existing state as that must be fully reinitialised by the caller.
//			StencilRef = 0;
//			BlendFactor = FLinearColor::Transparent;
//			DepthStencilState.SafeRelease();
//			RasterizerState.SafeRelease();
//			BlendState.SafeRelease();
//			BoundShaderState.SafeRelease();
//			PipelineState = nil;
//		}
		
		[RenderPassDesc release];
		RenderPassDesc = nil;
		RenderPassDesc = [RenderPass retain];
		
		bNeedsSet = true;
	}

	return bNeedsSet;
}

void FMetalStateCache::InvalidateRenderTargets(void)
{
	bHasValidRenderTarget = false;
	bHasValidColorTarget = false;
	bIsRenderTargetActive = false;
}

void FMetalStateCache::SetRenderTargetsActive(bool const bActive)
{
	bIsRenderTargetActive = bActive;
}

static bool MTLViewportEqual(MTLViewport const& Left, MTLViewport const& Right)
{
	return FMath::IsNearlyEqual(Left.originX, Right.originX) &&
			FMath::IsNearlyEqual(Left.originY, Right.originY) &&
			FMath::IsNearlyEqual(Left.width, Right.width) &&
			FMath::IsNearlyEqual(Left.height, Right.height) &&
			FMath::IsNearlyEqual(Left.znear, Right.znear) &&
			FMath::IsNearlyEqual(Left.zfar, Right.zfar);
}

void FMetalStateCache::SetViewport(const MTLViewport& InViewport)
{
	if (!MTLViewportEqual(Viewport, InViewport))
	{
		Viewport = InViewport;
	
		RasterBits |= EMetalRenderFlagViewport;
	}
	
	if (!bScissorRectEnabled)
	{
		MTLScissorRect Rect;
		Rect.x = InViewport.originX;
		Rect.y = InViewport.originY;
		Rect.width = InViewport.width;
		Rect.height = InViewport.height;
		SetScissorRect(false, Rect);
	}
}

void FMetalStateCache::SetVertexStream(uint32 const Index, id<MTLBuffer> Buffer, NSData* Bytes, uint32 const Stride, uint32 const Offset, uint32 const Length)
{
	check(Index < MaxVertexElementCount);
	check(UNREAL_TO_METAL_BUFFER_INDEX(Index) < MaxMetalStreams);

	VertexBuffers[Index].Buffer = Buffer;
	VertexBuffers[Index].Offset = (Buffer || Bytes) ? Stride : 0;
	VertexBuffers[Index].Bytes = Bytes;
	VertexBuffers[Index].Length = Length;
	
	SetShaderBuffer(SF_Vertex, Buffer, Bytes, Offset, Length, UNREAL_TO_METAL_BUFFER_INDEX(Index));
}

uint32 FMetalStateCache::GetVertexBufferSize(uint32 const Index)
{
	check(Index < MaxVertexElementCount);
	check(UNREAL_TO_METAL_BUFFER_INDEX(Index) < MaxMetalStreams);
	return VertexBuffers[Index].Length;
}

#if PLATFORM_MAC
void FMetalStateCache::SetPrimitiveTopology(MTLPrimitiveTopologyClass PrimitiveType)
{
	PipelineDesc.SetHashValue(Offset_PrimitiveTopology, NumBits_PrimitiveTopology, PrimitiveType);
	PipelineDesc.PipelineDescriptor.inputPrimitiveTopology = PrimitiveType;
}
#endif

void FMetalStateCache::SetPipelineState(FMetalShaderPipeline* State)
{
	if (PipelineState != State)
	{
		PipelineState = State;
		
		RasterBits |= EMetalRenderFlagPipelineState;
	}
}

void FMetalStateCache::SetIndexType(EMetalIndexType IndexType)
{
	PipelineDesc.SetHashValue(Offset_IndexType, NumBits_IndexType, IndexType);
	PipelineDesc.IndexType = IndexType;
}

void FMetalStateCache::BindUniformBuffer(EShaderFrequency const Freq, uint32 const BufferIndex, FUniformBufferRHIParamRef BufferRHI)
{
	if (BoundUniformBuffers[Freq].Num() <= BufferIndex)
	{
		BoundUniformBuffers[Freq].SetNumZeroed(BufferIndex+1);
	}
	BoundUniformBuffers[Freq][BufferIndex] = BufferRHI;
	DirtyUniformBuffers[Freq] |= 1 << BufferIndex;
}

void FMetalStateCache::SetDirtyUniformBuffers(EShaderFrequency const Freq, uint32 const Dirty)
{
	DirtyUniformBuffers[Freq] = Dirty;
}

void FMetalStateCache::SetVisibilityResultMode(MTLVisibilityResultMode const Mode, NSUInteger const Offset)
{
	if (VisibilityMode != Mode || VisibilityOffset != Offset)
	{
		VisibilityMode = Mode;
		VisibilityOffset = Offset;
		
		RasterBits |= EMetalRenderFlagVisibilityResultMode;
	}
}

void FMetalStateCache::ConditionalUpdateBackBuffer(FMetalSurface& Surface)
{
	// are we setting the back buffer? if so, make sure we have the drawable
	if ((Surface.Flags & TexCreate_Presentable))
	{
		// update the back buffer texture the first time used this frame
		if (Surface.Texture == nil)
		{
			// set the texture into the backbuffer
			Surface.GetDrawableTexture();
		}
#if PLATFORM_MAC
		check (Surface.Texture);
#endif
	}
}

bool FMetalStateCache::NeedsToSetRenderTarget(const FRHISetRenderTargetsInfo& InRenderTargetsInfo) const
{
	// see if our new Info matches our previous Info
	
	// basic checks
	bool bAllChecksPassed = GetHasValidRenderTarget() && bIsRenderTargetActive && InRenderTargetsInfo.NumColorRenderTargets == RenderTargetsInfo.NumColorRenderTargets && InRenderTargetsInfo.NumUAVs == RenderTargetsInfo.NumUAVs &&
		// handle the case where going from backbuffer + depth -> backbuffer + null, no need to reset RT and do a store/load
		(InRenderTargetsInfo.DepthStencilRenderTarget.Texture == RenderTargetsInfo.DepthStencilRenderTarget.Texture || InRenderTargetsInfo.DepthStencilRenderTarget.Texture == nullptr);

	// now check each color target if the basic tests passe
	if (bAllChecksPassed)
	{
		for (int32 RenderTargetIndex = 0; RenderTargetIndex < InRenderTargetsInfo.NumColorRenderTargets; RenderTargetIndex++)
		{
			const FRHIRenderTargetView& RenderTargetView = InRenderTargetsInfo.ColorRenderTarget[RenderTargetIndex];
			const FRHIRenderTargetView& PreviousRenderTargetView = RenderTargetsInfo.ColorRenderTarget[RenderTargetIndex];

			// handle simple case of switching textures or mip/slice
			if (RenderTargetView.Texture != PreviousRenderTargetView.Texture ||
				RenderTargetView.MipIndex != PreviousRenderTargetView.MipIndex ||
				RenderTargetView.ArraySliceIndex != PreviousRenderTargetView.ArraySliceIndex)
			{
				bAllChecksPassed = false;
				break;
			}
			
			// it's non-trivial when we need to switch based on load/store action:
			// LoadAction - it only matters what we are switching to in the new one
			//    If we switch to Load, no need to switch as we can re-use what we already have
			//    If we switch to Clear, we have to always switch to a new RT to force the clear
			//    If we switch to DontCare, there's definitely no need to switch
			//    If we switch *from* Clear then we must change target as we *don't* want to clear again.
            if (RenderTargetView.LoadAction == ERenderTargetLoadAction::EClear)
            {
                bAllChecksPassed = false;
                break;
            }
            // StoreAction - this matters what the previous one was **In Spirit**
            //    If we come from Store, we need to switch to a new RT to force the store
            //    If we come from DontCare, then there's no need to switch
            //    @todo metal: However, we basically only use Store now, and don't
            //        care about intermediate results, only final, so we don't currently check the value
            //			if (PreviousRenderTargetView.StoreAction == ERenderTTargetStoreAction::EStore)
            //			{
            //				bAllChecksPassed = false;
            //				break;
            //			}
        }
        
        if (InRenderTargetsInfo.DepthStencilRenderTarget.Texture && (InRenderTargetsInfo.DepthStencilRenderTarget.DepthLoadAction == ERenderTargetLoadAction::EClear || InRenderTargetsInfo.DepthStencilRenderTarget.StencilLoadAction == ERenderTargetLoadAction::EClear))
        {
            bAllChecksPassed = false;
		}
	}

	// if we are setting them to nothing, then this is probably end of frame, and we can't make a framebuffer
	// with nothng, so just abort this (only need to check on single MRT case)
	if (InRenderTargetsInfo.NumColorRenderTargets == 1 && InRenderTargetsInfo.ColorRenderTarget[0].Texture == nullptr &&
		InRenderTargetsInfo.DepthStencilRenderTarget.Texture == nullptr)
	{
		bAllChecksPassed = true;
	}

	return bAllChecksPassed == false;
}

void FMetalStateCache::SetShaderBuffer(EShaderFrequency const Frequency, id<MTLBuffer> const Buffer, NSData* const Bytes, NSUInteger const Offset, NSUInteger const Length, NSUInteger const Index)
{
	check(Frequency < SF_NumFrequencies);
	check(Index < ML_MaxBuffers);
	
	if (ShaderBuffers[Frequency].Buffers[Index].Buffer != Buffer ||
		ShaderBuffers[Frequency].Buffers[Index].Bytes != Bytes ||
		ShaderBuffers[Frequency].Buffers[Index].Offset != Offset ||
		ShaderBuffers[Frequency].Buffers[Index].Length != Length)
	{
		ShaderBuffers[Frequency].Buffers[Index].Buffer = Buffer;
		ShaderBuffers[Frequency].Buffers[Index].Bytes = Bytes;
		ShaderBuffers[Frequency].Buffers[Index].Offset = Offset;
		ShaderBuffers[Frequency].Buffers[Index].Length = Length;
		
		if (Buffer || Bytes)
		{
			ShaderBuffers[Frequency].Bound |= (1 << Index);
		}
		else
		{
			ShaderBuffers[Frequency].Bound &= ~(1 << Index);
		}
	}
}

void FMetalStateCache::SetShaderTexture(EShaderFrequency const Frequency, id<MTLTexture> Texture, NSUInteger const Index)
{
	check(Frequency < SF_NumFrequencies);
	check(Index < ML_MaxTextures);
	
	if (ShaderTextures[Frequency].Textures[Index] != Texture)
	{
		ShaderTextures[Frequency].Textures[Index] = Texture;
		
		if (Texture)
		{
			ShaderTextures[Frequency].Bound |= (1 << Index);
		}
		else
		{
			ShaderTextures[Frequency].Bound &= ~(1 << Index);
		}
	}
}

void FMetalStateCache::SetShaderSamplerState(EShaderFrequency const Frequency, FMetalSamplerState* const Sampler, NSUInteger const Index)
{
	check(Frequency < SF_NumFrequencies);
	check(Index < ML_MaxSamplers);
	
	if (ShaderSamplers[Frequency].Samplers[Index] != Sampler)
	{
		ShaderSamplers[Frequency].Samplers[Index] = Sampler;

		if (Sampler)
		{
			ShaderSamplers[Frequency].Bound |= (1 << Index);
		}
		else
		{
			ShaderSamplers[Frequency].Bound &= ~(1 << Index);
		}
	}
}

void FMetalStateCache::SetResource(uint32 ShaderStage, uint32 BindIndex, FRHITexture* RESTRICT TextureRHI, float CurrentTime)
{
	FMetalSurface* Surface = GetMetalSurfaceFromRHITexture(TextureRHI);
	id<MTLTexture> Texture = nil;
	if (Surface != nullptr)
	{
		TextureRHI->SetLastRenderTime(CurrentTime);
		Texture = Surface->Texture;
	}
	
	switch (ShaderStage)
	{
		case CrossCompiler::SHADER_STAGE_PIXEL:
			SetShaderTexture(SF_Pixel, Texture, BindIndex);
			break;
			
		case CrossCompiler::SHADER_STAGE_VERTEX:
			SetShaderTexture(SF_Vertex, Texture, BindIndex);
			break;
			
		case CrossCompiler::SHADER_STAGE_COMPUTE:
			SetShaderTexture(SF_Compute, Texture, BindIndex);
			break;
			
		case CrossCompiler::SHADER_STAGE_HULL:
			SetShaderTexture(SF_Hull, Texture, BindIndex);
			break;
			
		case CrossCompiler::SHADER_STAGE_DOMAIN:
			SetShaderTexture(SF_Domain, Texture, BindIndex);
			break;
			
		default:
			check(0);
			break;
	}
}

void FMetalStateCache::SetShaderResourceView(FMetalContext* Context, EShaderFrequency ShaderStage, uint32 BindIndex, FMetalShaderResourceView* RESTRICT SRV)
{
	if (SRV)
	{
		FRHITexture* Texture = SRV->SourceTexture.GetReference();
		FMetalVertexBuffer* VB = SRV->SourceVertexBuffer.GetReference();
		FMetalIndexBuffer* IB = SRV->SourceIndexBuffer.GetReference();
		FMetalStructuredBuffer* SB = SRV->SourceStructuredBuffer.GetReference();
		if (Texture)
		{
			FMetalSurface* Surface = SRV->TextureView;
			if (Surface != nullptr)
			{
				if (Context)
				{
					Surface->UpdateSRV(Context, SRV->SourceTexture);
				}
				SetShaderTexture(ShaderStage, Surface->Texture, BindIndex);
			}
			else
			{
				SetShaderTexture(ShaderStage, nil, BindIndex);
			}
		}
		else if (VB)
		{
			SetShaderBuffer(ShaderStage, VB->Buffer, VB->Data, 0, VB->GetSize(), BindIndex);
		}
		else if (IB)
		{
			SetShaderBuffer(ShaderStage, IB->Buffer, nil, 0, IB->GetSize(), BindIndex);
		}
		else if (SB)
		{
			SetShaderBuffer(ShaderStage, SB->Buffer, nil, 0, SB->GetSize(), BindIndex);
		}
	}
}

void FMetalStateCache::SetShaderUnorderedAccessView(EShaderFrequency ShaderStage, uint32 BindIndex, FMetalUnorderedAccessView* RESTRICT UAV)
{
	if (UAV)
	{
		// figure out which one of the resources we need to set
		FMetalStructuredBuffer* StructuredBuffer = UAV->SourceView->SourceStructuredBuffer.GetReference();
		FMetalVertexBuffer* VertexBuffer = UAV->SourceView->SourceVertexBuffer.GetReference();
		FRHITexture* Texture = UAV->SourceView->SourceTexture.GetReference();
		FMetalSurface* Surface = UAV->SourceView->TextureView;
		if (StructuredBuffer)
		{
			SetShaderBuffer(ShaderStage, StructuredBuffer->Buffer, nil, 0, StructuredBuffer->GetSize(), BindIndex);
		}
		else if (VertexBuffer)
		{
			check(!VertexBuffer->Data && VertexBuffer->Buffer);
			SetShaderBuffer(ShaderStage, VertexBuffer->Buffer, VertexBuffer->Data, 0, VertexBuffer->GetSize(), BindIndex);
		}
		else if (Texture)
		{
			if (!Surface)
			{
				Surface = GetMetalSurfaceFromRHITexture(Texture);
			}
			if (Surface != nullptr)
			{
				FMetalSurface* Source = GetMetalSurfaceFromRHITexture(Texture);
				
				FPlatformAtomics::InterlockedExchange(&Surface->Written, 1);
				FPlatformAtomics::InterlockedExchange(&Source->Written, 1);
				
				SetShaderTexture(ShaderStage, Surface->Texture, BindIndex);
			}
			else
			{
				SetShaderTexture(ShaderStage, nil, BindIndex);
			}
		}
	}
}

void FMetalStateCache::SetResource(uint32 ShaderStage, uint32 BindIndex, FMetalShaderResourceView* RESTRICT SRV, float CurrentTime)
{
	switch (ShaderStage)
	{
		case CrossCompiler::SHADER_STAGE_PIXEL:
			SetShaderResourceView(nullptr, SF_Pixel, BindIndex, SRV);
			break;
			
		case CrossCompiler::SHADER_STAGE_VERTEX:
			SetShaderResourceView(nullptr, SF_Vertex, BindIndex, SRV);
			break;
			
		case CrossCompiler::SHADER_STAGE_COMPUTE:
			SetShaderResourceView(nullptr, SF_Compute, BindIndex, SRV);
			break;
			
		case CrossCompiler::SHADER_STAGE_HULL:
			SetShaderResourceView(nullptr, SF_Hull, BindIndex, SRV);
			break;
			
		case CrossCompiler::SHADER_STAGE_DOMAIN:
			SetShaderResourceView(nullptr, SF_Domain, BindIndex, SRV);
			break;
			
		default:
			check(0);
			break;
	}
}

void FMetalStateCache::SetResource(uint32 ShaderStage, uint32 BindIndex, FMetalSamplerState* RESTRICT SamplerState, float CurrentTime)
{
	check(SamplerState->State != nil);
	switch (ShaderStage)
	{
		case CrossCompiler::SHADER_STAGE_PIXEL:
			SetShaderSamplerState(SF_Pixel, SamplerState, BindIndex);
			break;
			
		case CrossCompiler::SHADER_STAGE_VERTEX:
			SetShaderSamplerState(SF_Vertex, SamplerState, BindIndex);
			break;
			
		case CrossCompiler::SHADER_STAGE_COMPUTE:
			SetShaderSamplerState(SF_Compute, SamplerState, BindIndex);
			break;
			
		case CrossCompiler::SHADER_STAGE_HULL:
			SetShaderSamplerState(SF_Hull, SamplerState, BindIndex);
			break;
			
		case CrossCompiler::SHADER_STAGE_DOMAIN:
			SetShaderSamplerState(SF_Domain, SamplerState, BindIndex);
			break;
			
		default:
			check(0);
			break;
	}
}

void FMetalStateCache::SetResource(uint32 ShaderStage, uint32 BindIndex, FMetalUnorderedAccessView* RESTRICT UAV, float CurrentTime)
{
	switch (ShaderStage)
	{
		case CrossCompiler::SHADER_STAGE_PIXEL:
			SetShaderUnorderedAccessView(SF_Pixel, BindIndex, UAV);
			break;
			
		case CrossCompiler::SHADER_STAGE_VERTEX:
			SetShaderUnorderedAccessView(SF_Vertex, BindIndex, UAV);
			break;
			
		case CrossCompiler::SHADER_STAGE_COMPUTE:
			SetShaderUnorderedAccessView(SF_Compute, BindIndex, UAV);
			break;
			
		case CrossCompiler::SHADER_STAGE_HULL:
			SetShaderUnorderedAccessView(SF_Hull, BindIndex, UAV);
			break;
			
		case CrossCompiler::SHADER_STAGE_DOMAIN:
			SetShaderUnorderedAccessView(SF_Domain, BindIndex, UAV);
			break;
			
		default:
			check(0);
			break;
	}
}


template <typename MetalResourceType>
inline int32 FMetalStateCache::SetShaderResourcesFromBuffer(uint32 ShaderStage, FMetalUniformBuffer* RESTRICT Buffer, const uint32* RESTRICT ResourceMap, int32 BufferIndex)
{
	const TRefCountPtr<FRHIResource>* RESTRICT Resources = Buffer->ResourceTable.GetData();
	float CurrentTime = FPlatformTime::Seconds();
	int32 NumSetCalls = 0;
	uint32 BufferOffset = ResourceMap[BufferIndex];
	if (BufferOffset > 0)
	{
		const uint32* RESTRICT ResourceInfos = &ResourceMap[BufferOffset];
		uint32 ResourceInfo = *ResourceInfos++;
		do
		{
			checkSlow(FRHIResourceTableEntry::GetUniformBufferIndex(ResourceInfo) == BufferIndex);
			const uint16 ResourceIndex = FRHIResourceTableEntry::GetResourceIndex(ResourceInfo);
			const uint8 BindIndex = FRHIResourceTableEntry::GetBindIndex(ResourceInfo);
			
			MetalResourceType* ResourcePtr = (MetalResourceType*)Resources[ResourceIndex].GetReference();
			
			// todo: could coalesce adjacent bound resources.
			SetResource(ShaderStage, BindIndex, ResourcePtr, CurrentTime);
			
			NumSetCalls++;
			ResourceInfo = *ResourceInfos++;
		} while (FRHIResourceTableEntry::GetUniformBufferIndex(ResourceInfo) == BufferIndex);
	}
	return NumSetCalls;
}

template <class ShaderType>
void FMetalStateCache::SetResourcesFromTables(ShaderType Shader, uint32 ShaderStage)
{
	checkSlow(Shader);
	
	if (!FShaderCache::IsPredrawCall(ShaderCacheContextState))
	{
		EShaderFrequency Frequency;
		switch(ShaderStage)
		{
			case CrossCompiler::SHADER_STAGE_VERTEX:
				Frequency = SF_Vertex;
				break;
			case CrossCompiler::SHADER_STAGE_HULL:
				Frequency = SF_Hull;
				break;
			case CrossCompiler::SHADER_STAGE_DOMAIN:
				Frequency = SF_Domain;
				break;
			case CrossCompiler::SHADER_STAGE_PIXEL:
				Frequency = SF_Pixel;
				break;
			case CrossCompiler::SHADER_STAGE_COMPUTE:
				Frequency = SF_Compute;
				break;
			default:
				Frequency = SF_NumFrequencies; //Silence a compiler warning/error
				check(false);
				break;
		}

		// Mask the dirty bits by those buffers from which the shader has bound resources.
		uint32 DirtyBits = Shader->Bindings.ShaderResourceTable.ResourceTableBits & GetDirtyUniformBuffers(Frequency);
		while (DirtyBits)
		{
			// Scan for the lowest set bit, compute its index, clear it in the set of dirty bits.
			const uint32 LowestBitMask = (DirtyBits)& (-(int32)DirtyBits);
			const int32 BufferIndex = FMath::FloorLog2(LowestBitMask); // todo: This has a branch on zero, we know it could never be zero...
			DirtyBits ^= LowestBitMask;
			FMetalUniformBuffer* Buffer = (FMetalUniformBuffer*)GetBoundUniformBuffers(Frequency)[BufferIndex].GetReference();
			check(Buffer);
			check(BufferIndex < Shader->Bindings.ShaderResourceTable.ResourceTableLayoutHashes.Num());
			check(Buffer->GetLayout().GetHash() == Shader->Bindings.ShaderResourceTable.ResourceTableLayoutHashes[BufferIndex]);
			
			// todo: could make this two pass: gather then set
			SetShaderResourcesFromBuffer<FRHITexture>(ShaderStage, Buffer, Shader->Bindings.ShaderResourceTable.TextureMap.GetData(), BufferIndex);
			SetShaderResourcesFromBuffer<FMetalShaderResourceView>(ShaderStage, Buffer, Shader->Bindings.ShaderResourceTable.ShaderResourceViewMap.GetData(), BufferIndex);
			SetShaderResourcesFromBuffer<FMetalSamplerState>(ShaderStage, Buffer, Shader->Bindings.ShaderResourceTable.SamplerMap.GetData(), BufferIndex);
			SetShaderResourcesFromBuffer<FMetalUnorderedAccessView>(ShaderStage, Buffer, Shader->Bindings.ShaderResourceTable.UnorderedAccessViewMap.GetData(), BufferIndex);
		}
		SetDirtyUniformBuffers(Frequency, 0);
	}
}

void FMetalStateCache::CommitRenderResources(FMetalCommandEncoder* Raster)
{
	check(IsValidRef(BoundShaderState));
    
    SetResourcesFromTables(BoundShaderState->VertexShader, CrossCompiler::SHADER_STAGE_VERTEX);
    GetShaderParameters(CrossCompiler::SHADER_STAGE_VERTEX).CommitPackedUniformBuffers(this, BoundShaderState, nullptr, CrossCompiler::SHADER_STAGE_VERTEX, GetBoundUniformBuffers(SF_Vertex), BoundShaderState->VertexShader->UniformBuffersCopyInfo);
    GetShaderParameters(CrossCompiler::SHADER_STAGE_VERTEX).CommitPackedGlobals(this, Raster, SF_Vertex, BoundShaderState->VertexShader->Bindings);
	
    if (IsValidRef(BoundShaderState->PixelShader))
    {
    	SetResourcesFromTables(BoundShaderState->PixelShader, CrossCompiler::SHADER_STAGE_PIXEL);
        GetShaderParameters(CrossCompiler::SHADER_STAGE_PIXEL).CommitPackedUniformBuffers(this, BoundShaderState, nullptr, CrossCompiler::SHADER_STAGE_PIXEL, GetBoundUniformBuffers(SF_Pixel), BoundShaderState->PixelShader->UniformBuffersCopyInfo);
        GetShaderParameters(CrossCompiler::SHADER_STAGE_PIXEL).CommitPackedGlobals(this, Raster, SF_Pixel, BoundShaderState->PixelShader->Bindings);
    }
}

void FMetalStateCache::CommitTessellationResources(FMetalCommandEncoder* Raster, FMetalCommandEncoder* Compute)
{
	check(IsValidRef(BoundShaderState));
    check(IsValidRef(BoundShaderState->HullShader) && IsValidRef(BoundShaderState->DomainShader));
    
    SetResourcesFromTables(BoundShaderState->VertexShader, CrossCompiler::SHADER_STAGE_VERTEX);
    GetShaderParameters(CrossCompiler::SHADER_STAGE_VERTEX).CommitPackedUniformBuffers(this, BoundShaderState, nullptr, CrossCompiler::SHADER_STAGE_VERTEX, GetBoundUniformBuffers(SF_Vertex), BoundShaderState->VertexShader->UniformBuffersCopyInfo);
    GetShaderParameters(CrossCompiler::SHADER_STAGE_VERTEX).CommitPackedGlobals(this, Compute, SF_Vertex, BoundShaderState->VertexShader->Bindings);
	
    if (IsValidRef(BoundShaderState->PixelShader))
    {
    	SetResourcesFromTables(BoundShaderState->PixelShader, CrossCompiler::SHADER_STAGE_PIXEL);
        GetShaderParameters(CrossCompiler::SHADER_STAGE_PIXEL).CommitPackedUniformBuffers(this, BoundShaderState, nullptr, CrossCompiler::SHADER_STAGE_PIXEL, GetBoundUniformBuffers(SF_Pixel), BoundShaderState->PixelShader->UniformBuffersCopyInfo);
        GetShaderParameters(CrossCompiler::SHADER_STAGE_PIXEL).CommitPackedGlobals(this, Raster, SF_Pixel, BoundShaderState->PixelShader->Bindings);
    }
    
    SetResourcesFromTables(BoundShaderState->HullShader, CrossCompiler::SHADER_STAGE_HULL);
    GetShaderParameters(CrossCompiler::SHADER_STAGE_HULL).CommitPackedUniformBuffers(this, BoundShaderState, nullptr, CrossCompiler::SHADER_STAGE_HULL, GetBoundUniformBuffers(SF_Hull), BoundShaderState->HullShader->UniformBuffersCopyInfo);
    GetShaderParameters(CrossCompiler::SHADER_STAGE_HULL).CommitPackedGlobals(this, Compute, SF_Hull, BoundShaderState->HullShader->Bindings);
	
	SetResourcesFromTables(BoundShaderState->DomainShader, CrossCompiler::SHADER_STAGE_DOMAIN);
    GetShaderParameters(CrossCompiler::SHADER_STAGE_DOMAIN).CommitPackedUniformBuffers(this, BoundShaderState, nullptr, CrossCompiler::SHADER_STAGE_DOMAIN, GetBoundUniformBuffers(SF_Domain), BoundShaderState->DomainShader->UniformBuffersCopyInfo);
    GetShaderParameters(CrossCompiler::SHADER_STAGE_DOMAIN).CommitPackedGlobals(this, Raster, SF_Domain, BoundShaderState->DomainShader->Bindings);
}

void FMetalStateCache::CommitComputeResources(FMetalCommandEncoder* Compute)
{
	check(IsValidRef(ComputeShader));
	SetResourcesFromTables(ComputeShader, CrossCompiler::SHADER_STAGE_COMPUTE);
	
	GetShaderParameters(CrossCompiler::SHADER_STAGE_COMPUTE).CommitPackedUniformBuffers(this, BoundShaderState, ComputeShader, CrossCompiler::SHADER_STAGE_COMPUTE, GetBoundUniformBuffers(SF_Compute), ComputeShader->UniformBuffersCopyInfo);
	GetShaderParameters(CrossCompiler::SHADER_STAGE_COMPUTE).CommitPackedGlobals(this, Compute, SF_Compute, ComputeShader->Bindings);
}

bool FMetalStateCache::PrepareToRestart(void)
{
	if(CanRestartRenderPass())
	{
		return true;
	}
	else
	{
		if (GetRenderPipelineDesc().SampleCount <= 1)
		{
			// Deferred store actions make life a bit easier...
			static bool bSupportsDeferredStore = GetMetalDeviceContext().GetCommandQueue().SupportsFeature(EMetalFeaturesDeferredStoreActions);
			
			FRHISetRenderTargetsInfo Info = GetRenderTargetsInfo();
			for (int32 RenderTargetIndex = 0; RenderTargetIndex < Info.NumColorRenderTargets; RenderTargetIndex++)
			{
				FRHIRenderTargetView& RenderTargetView = Info.ColorRenderTarget[RenderTargetIndex];
				RenderTargetView.LoadAction = ERenderTargetLoadAction::ELoad;
				check(RenderTargetView.Texture == nil || RenderTargetView.StoreAction == ERenderTargetStoreAction::EStore);
			}
			Info.bClearColor = false;
			
			if (Info.DepthStencilRenderTarget.Texture)
			{
				Info.DepthStencilRenderTarget.DepthLoadAction = ERenderTargetLoadAction::ELoad;
				check(bSupportsDeferredStore || !Info.DepthStencilRenderTarget.GetDepthStencilAccess().IsDepthWrite() || Info.DepthStencilRenderTarget.DepthStoreAction == ERenderTargetStoreAction::EStore);
				Info.bClearDepth = false;
				
				Info.DepthStencilRenderTarget.StencilLoadAction = ERenderTargetLoadAction::ELoad;
				// @todo Stencil writes that need to persist must use ERenderTargetStoreAction::EStore on iOS.
				// We should probably be using deferred store actions so that we can safely lazily instantiate encoders.
				check(bSupportsDeferredStore || !Info.DepthStencilRenderTarget.GetDepthStencilAccess().IsStencilWrite() || Info.DepthStencilRenderTarget.GetStencilStoreAction() == ERenderTargetStoreAction::EStore);
				Info.bClearStencil = false;
			}
			
			InvalidateRenderTargets();
			return SetRenderTargetsInfo(Info, GetVisibilityResultsBuffer(), false) && CanRestartRenderPass();
		}
		else
		{
			return false;
		}
	}
}

void FMetalStateCache::SetStateDirty(void)
{
	RasterBits = UINT32_MAX;
	for (uint32 i = 0; i < SF_NumFrequencies; i++)
	{
		ShaderBuffers[i].Bound = UINT32_MAX;
#if PLATFORM_MAC
#ifndef UINT128_MAX
#define UINT128_MAX (((__uint128_t)1 << 127) - (__uint128_t)1 + ((__uint128_t)1 << 127))
#endif
		ShaderTextures[i].Bound = UINT128_MAX;
#else
		ShaderTextures[i].Bound = UINT32_MAX;
#endif
		ShaderSamplers[i].Bound = UINT32_MAX;
	}
}

void FMetalStateCache::SetRenderStoreActions(FMetalCommandEncoder& CommandEncoder, bool const bConditionalSwitch)
{
	check(CommandEncoder.IsRenderCommandEncoderActive())
	{
		// Deferred store actions make life a bit easier...
		static bool bSupportsDeferredStore = GetMetalDeviceContext().GetCommandQueue().SupportsFeature(EMetalFeaturesDeferredStoreActions);
		if (bConditionalSwitch && bSupportsDeferredStore)
		{
			for (int32 RenderTargetIndex = 0; RenderTargetIndex < RenderTargetsInfo.NumColorRenderTargets; RenderTargetIndex++)
			{
				FRHIRenderTargetView& RenderTargetView = RenderTargetsInfo.ColorRenderTarget[RenderTargetIndex];
				if(RenderTargetView.Texture != nil && RenderTargetView.StoreAction != ERenderTargetStoreAction::EStore)
				{
					ColorStore[RenderTargetIndex] = GetConditionalMetalRTStoreAction(RenderTargetView.StoreAction);
				}
			}
			
			if (RenderTargetsInfo.DepthStencilRenderTarget.Texture)
			{
				if(RenderTargetsInfo.DepthStencilRenderTarget.GetDepthStencilAccess().IsDepthWrite() && RenderTargetsInfo.DepthStencilRenderTarget.DepthStoreAction != ERenderTargetStoreAction::EStore)
				{
					DepthStore = GetConditionalMetalRTStoreAction(RenderTargetsInfo.DepthStencilRenderTarget.DepthStoreAction);
				}
				
				if(RenderTargetsInfo.DepthStencilRenderTarget.GetDepthStencilAccess().IsStencilWrite() && RenderTargetsInfo.DepthStencilRenderTarget.GetStencilStoreAction() != ERenderTargetStoreAction::EStore)
				{
					StencilStore = GetConditionalMetalRTStoreAction(RenderTargetsInfo.DepthStencilRenderTarget.GetStencilStoreAction());
				}
			}
		}
		CommandEncoder.SetRenderPassStoreActions(ColorStore, DepthStore, StencilStore);
	}
}

void FMetalStateCache::SetRenderState(FMetalCommandEncoder& CommandEncoder, FMetalCommandEncoder* PrologueEncoder)
{
    if (RasterBits & EMetalRenderFlagViewport)
    {
        CommandEncoder.SetViewport(Viewport);
    }
    if (RasterBits & EMetalRenderFlagFrontFacingWinding)
    {
        CommandEncoder.SetFrontFacingWinding(MTLWindingCounterClockwise);
    }
    if (RasterBits & EMetalRenderFlagCullMode)
    {
		check(IsValidRef(RasterizerState));
        CommandEncoder.SetCullMode(TranslateCullMode(RasterizerState->State.CullMode));
    }
    if (RasterBits & EMetalRenderFlagDepthBias)
	{
		check(IsValidRef(RasterizerState));
        CommandEncoder.SetDepthBias(RasterizerState->State.DepthBias, RasterizerState->State.SlopeScaleDepthBias, FLT_MAX);
    }
    if ((RasterBits & EMetalRenderFlagScissorRect) && !FShaderCache::IsPredrawCall(ShaderCacheContextState))
    {
        CommandEncoder.SetScissorRect(Scissor);
    }
    if (RasterBits & EMetalRenderFlagTriangleFillMode)
	{
		check(IsValidRef(RasterizerState));
        CommandEncoder.SetTriangleFillMode(TranslateFillMode(RasterizerState->State.FillMode));
    }
    if (RasterBits & EMetalRenderFlagBlendColor)
    {
        CommandEncoder.SetBlendColor(BlendFactor.R, BlendFactor.G, BlendFactor.B, BlendFactor.A);
    }
    if (RasterBits & EMetalRenderFlagDepthStencilState)
    {
		check(IsValidRef(DepthStencilState));
        CommandEncoder.SetDepthStencilState(DepthStencilState ? DepthStencilState->State : nil);
    }
    if (RasterBits & EMetalRenderFlagStencilReferenceValue)
    {
        CommandEncoder.SetStencilReferenceValue(StencilRef);
    }
    if (RasterBits & EMetalRenderFlagVisibilityResultMode)
    {
        CommandEncoder.SetVisibilityResultMode(VisibilityMode, VisibilityOffset);
    }
	// Some Intel drivers need RenderPipeline state to be set after DepthStencil state to work properly
	if (RasterBits & EMetalRenderFlagPipelineState)
	{
		check(PipelineState);
		CommandEncoder.SetRenderPipelineState(PipelineState);
		if (PipelineState.ComputePipelineState)
		{
			check(PrologueEncoder);
			PrologueEncoder->SetComputePipelineState(PipelineState);
		}
	}
   	RasterBits = 0;
}

void FMetalStateCache::CommitResourceTable(EShaderFrequency const Frequency, MTLFunctionType const Type, FMetalCommandEncoder& CommandEncoder)
{
	FMetalBufferBindings& BufferBindings = ShaderBuffers[Frequency];
	for (uint Index = 0; Index < ML_MaxBuffers; Index++)
	{
		if (BufferBindings.Bound & (1 << Index))
		{
			BufferBindings.Bound &= ~(1 << Index);
			
			FMetalBufferBinding& Binding = BufferBindings.Buffers[Index];
			if (Binding.Buffer)
			{
				CommandEncoder.SetShaderBuffer(Type, Binding.Buffer, Binding.Offset, Binding.Length, Index);
			}
			else if (Binding.Bytes)
			{
				CommandEncoder.SetShaderData(Type, Binding.Bytes, Binding.Offset, Index);
			}
		}
	}
    
    FMetalTextureBindings& TextureBindings = ShaderTextures[Frequency];
	for (uint Index = 0; Index < ML_MaxTextures; Index++)
	{
		if (TextureBindings.Bound & FMetalTextureMask(FMetalTextureMask(1) << FMetalTextureMask(Index)))
		{
			TextureBindings.Bound &= ~FMetalTextureMask(FMetalTextureMask(1) << FMetalTextureMask(Index));
			
			if (TextureBindings.Textures[Index])
			{
				CommandEncoder.SetShaderTexture(Type, TextureBindings.Textures[Index], Index);
			}
		}
	}
	
    FMetalSamplerBindings& SamplerBindings = ShaderSamplers[Frequency];
	for (uint Index = 0; Index < ML_MaxSamplers; Index++)
	{
		if (SamplerBindings.Bound & (1 << Index))
		{
			SamplerBindings.Bound &= ~(1 << Index);
			
			if (IsValidRef(SamplerBindings.Samplers[Index]))
			{
				CommandEncoder.SetShaderSamplerState(Type, SamplerBindings.Samplers[Index]->State, Index);
			}
		}
	}
}

FTexture2DRHIRef FMetalStateCache::CreateFallbackDepthStencilSurface(uint32 Width, uint32 Height)
{
	if (!IsValidRef(FallbackDepthStencilSurface) || FallbackDepthStencilSurface->GetSizeX() != Width || FallbackDepthStencilSurface->GetSizeY() != Height)
	{
		FRHIResourceCreateInfo TexInfo;
		FallbackDepthStencilSurface = RHICreateTexture2D(Width, Height, PF_DepthStencil, 1, 1, TexCreate_DepthStencilTargetable, TexInfo);
	}
	check(IsValidRef(FallbackDepthStencilSurface));
	return FallbackDepthStencilSurface;
}

