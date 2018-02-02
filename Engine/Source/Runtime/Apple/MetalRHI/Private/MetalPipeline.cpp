// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MetalPipeline.cpp: Metal shader pipeline RHI implementation.
=============================================================================*/

#include "MetalRHIPrivate.h"
#include "MetalPipeline.h"
#include "MetalShaderResources.h"
#include "MetalResources.h"
#include "MetalProfiler.h"
#include "MetalCommandQueue.h"
#include "MetalCommandBuffer.h"
#include "RenderUtils.h"
#include "ShaderCache.h"
#include "ScopeRWLock.h"
#include <objc/runtime.h>

static int32 GMetalTessellationForcePartitionMode = 0;
static FAutoConsoleVariableRef CVarMetalTessellationForcePartitionMode(
	TEXT("rhi.Metal.TessellationForcePartitionMode"),
	GMetalTessellationForcePartitionMode,
	TEXT("The partition mode (+1) to force Metal to use for debugging or off (0). (Default: 0)"));

static uint32 BlendBitOffsets[] = { Offset_BlendState0, Offset_BlendState1, Offset_BlendState2, Offset_BlendState3, Offset_BlendState4, Offset_BlendState5, Offset_BlendState6, Offset_BlendState7 };
static uint32 RTBitOffsets[] = { Offset_RenderTargetFormat0, Offset_RenderTargetFormat1, Offset_RenderTargetFormat2, Offset_RenderTargetFormat3, Offset_RenderTargetFormat4, Offset_RenderTargetFormat5, Offset_RenderTargetFormat6, Offset_RenderTargetFormat7 };
static_assert(Offset_RasterEnd < 64 && Offset_End < 128, "Offset_RasterEnd must be < 64 && Offset_End < 128");

static float RoundUpNearestEven(const float f)
{
	const float ret = FMath::CeilToFloat(f);
	const float isOdd = (float)(((int)ret) & 1);
	return ret + isOdd;
}

static float RoundTessLevel(float TessFactor, MTLTessellationPartitionMode PartitionMode)
{
	switch(PartitionMode)
	{
		case MTLTessellationPartitionModePow2:
			return FMath::RoundUpToPowerOfTwo((uint32)TessFactor);
		case MTLTessellationPartitionModeInteger:
			return FMath::CeilToFloat(TessFactor);
		case MTLTessellationPartitionModeFractionalEven:
		case MTLTessellationPartitionModeFractionalOdd: // these are handled the same way
			return RoundUpNearestEven(TessFactor);
		default:
			check(false);
			return 0.0f;
	}
}

@implementation FMetalShaderPipeline

APPLE_PLATFORM_OBJECT_ALLOC_OVERRIDES(FMetalShaderPipeline)

- (void)dealloc
{
	[_RenderPipelineState release];
	[_ComputePipelineState release];
	[_TessellationPipelineDesc release];
#if METAL_DEBUG_OPTIONS
	[_RenderPipelineReflection release];
	[_ComputePipelineReflection release];
	[_VertexSource release];
	[_FragmentSource release];
	[_ComputeSource release];
#endif
	[super dealloc];
}

#if METAL_DEBUG_OPTIONS
- (void)initResourceMask
{
	if (self.RenderPipelineReflection)
	{
		[self initResourceMask:EMetalShaderVertex];
		[self initResourceMask:EMetalShaderFragment];
	}
	if (self.ComputePipelineReflection)
	{
		[self initResourceMask:EMetalShaderCompute];
	}
}
- (void)initResourceMask:(EMetalShaderFrequency)Frequency
{
	NSArray<MTLArgument*>* Arguments = nil;
	switch(Frequency)
	{
		case EMetalShaderVertex:
		{
			MTLRenderPipelineReflection* Reflection = self.RenderPipelineReflection;
			check(Reflection);
			
			Arguments = Reflection.vertexArguments;
			break;
		}
		case EMetalShaderFragment:
		{
			MTLRenderPipelineReflection* Reflection = self.RenderPipelineReflection;
			check(Reflection);
			
			Arguments = Reflection.fragmentArguments;
			break;
		}
		case EMetalShaderCompute:
		{
			MTLComputePipelineReflection* Reflection = self.ComputePipelineReflection;
			check(Reflection);
			
			Arguments = Reflection.arguments;
			break;
		}
		default:
			check(false);
			break;
	}
	
	for (uint32 i = 0; i < Arguments.count; i++)
	{
		MTLArgument* Arg = [Arguments objectAtIndex:i];
		check(Arg);
		switch(Arg.type)
		{
			case MTLArgumentTypeBuffer:
			{
				checkf(Arg.index < ML_MaxBuffers, TEXT("Metal buffer index exceeded!"));
				ResourceMask[Frequency].BufferMask |= (1 << Arg.index);
				break;
			}
			case MTLArgumentTypeThreadgroupMemory:
			{
				break;
			}
			case MTLArgumentTypeTexture:
			{
				checkf(Arg.index < ML_MaxTextures, TEXT("Metal texture index exceeded!"));
				ResourceMask[Frequency].TextureMask |= (1 << Arg.index);
				break;
			}
			case MTLArgumentTypeSampler:
			{
				checkf(Arg.index < ML_MaxSamplers, TEXT("Metal sampler index exceeded!"));
				ResourceMask[Frequency].SamplerMask |= (1 << Arg.index);
				break;
			}
			default:
				check(false);
				break;
		}
	}
}
#endif
@end

@implementation FMetalTessellationPipelineDesc

APPLE_PLATFORM_OBJECT_ALLOC_OVERRIDES(FMetalTessellationPipelineDesc)

- (void)dealloc
{
	[_DomainVertexDescriptor release];
	[super dealloc];
}
@end

struct FMetalGraphicsPipelineKey
{
	FMetalRenderPipelineHash RenderPipelineHash;
	FMetalHashedVertexDescriptor VertexDescriptorHash;
	FSHAHash VertexFunction;
	FSHAHash DomainFunction;
	FSHAHash PixelFunction;
	uint32 VertexBufferHash;
	uint32 DomainBufferHash;
	uint32 PixelBufferHash;

	template<typename Type>
	inline void SetHashValue(uint32 Offset, uint32 NumBits, Type Value)
	{
		if (Offset < Offset_RasterEnd)
		{
			uint64 BitMask = ((((uint64)1ULL) << NumBits) - 1) << Offset;
			RenderPipelineHash.RasterBits = (RenderPipelineHash.RasterBits & ~BitMask) | (((uint64)Value << Offset) & BitMask);
		}
		else
		{
			Offset -= Offset_RenderTargetFormat0;
			uint64 BitMask = ((((uint64)1ULL) << NumBits) - 1) << Offset;
			RenderPipelineHash.TargetBits = (RenderPipelineHash.TargetBits & ~BitMask) | (((uint64)Value << Offset) & BitMask);
		}
	}

	bool operator==(FMetalGraphicsPipelineKey const& Other) const
	{
		return (RenderPipelineHash == Other.RenderPipelineHash
		&& VertexDescriptorHash == Other.VertexDescriptorHash
		&& VertexFunction == Other.VertexFunction
		&& DomainFunction == Other.DomainFunction
		&& PixelFunction == Other.PixelFunction
		&& VertexBufferHash == Other.VertexBufferHash
		&& DomainBufferHash == Other.DomainBufferHash
		&& PixelBufferHash == Other.PixelBufferHash);
	}
	
	friend uint32 GetTypeHash(FMetalGraphicsPipelineKey const& Key)
	{
		uint32 H = FCrc::MemCrc32(&Key.RenderPipelineHash, sizeof(Key.RenderPipelineHash), GetTypeHash(Key.VertexDescriptorHash));
		H = FCrc::MemCrc32(Key.VertexFunction.Hash, sizeof(Key.VertexFunction.Hash), H);
		H = FCrc::MemCrc32(Key.DomainFunction.Hash, sizeof(Key.DomainFunction.Hash), H);
		H = FCrc::MemCrc32(Key.PixelFunction.Hash, sizeof(Key.PixelFunction.Hash), H);
		H = HashCombine(H, GetTypeHash(Key.VertexBufferHash));
		H = HashCombine(H, GetTypeHash(Key.DomainBufferHash));
		H = HashCombine(H, GetTypeHash(Key.PixelBufferHash));
		return H;
	}
	
	friend void InitMetalGraphicsPipelineKey(FMetalGraphicsPipelineKey& Key, const FGraphicsPipelineStateInitializer& Init, EMetalIndexType const IndexType, EPixelFormat const* const VertexBufferTypes, EPixelFormat const* const PixelBufferTypes, EPixelFormat const* const DomainBufferTypes)
	{
		uint32 const NumActiveTargets = Init.ComputeNumValidRenderTargets();
		check(NumActiveTargets <= MaxSimultaneousRenderTargets);
	
		FMetalBlendState* BlendState = (FMetalBlendState*)Init.BlendState;
		
		FMemory::Memzero(Key.RenderPipelineHash);
		
		bool bHasActiveTargets = false;
		for (uint32 i = 0; i < NumActiveTargets; i++)
		{
			EPixelFormat TargetFormat = Init.RenderTargetFormats[i];
			if (TargetFormat == PF_Unknown) { continue; }

			MTLPixelFormat MetalFormat = (MTLPixelFormat)GPixelFormats[TargetFormat].PlatformFormat;
			uint32 Flags = Init.RenderTargetFlags[i];
			if (Flags & TexCreate_SRGB)
			{
#if PLATFORM_MAC // Expand as R8_sRGB is iOS only.
				if (MetalFormat == MTLPixelFormatR8Unorm)
				{
					MetalFormat = MTLPixelFormatRGBA8Unorm;
				}
#endif
				MetalFormat = ToSRGBFormat(MetalFormat);
			}
			
			uint8 FormatKey = GetMetalPixelFormatKey(MetalFormat);;
			Key.SetHashValue(RTBitOffsets[i], NumBits_RenderTargetFormat, FormatKey);
			Key.SetHashValue(BlendBitOffsets[i], NumBits_BlendState, BlendState->RenderTargetStates[i].BlendStateKey);
			
			bHasActiveTargets |= true;
		}
		
		uint8 DepthFormatKey = 0;
		uint8 StencilFormatKey = 0;
		switch(Init.DepthStencilTargetFormat)
		{
			case PF_DepthStencil:
			{
				MTLPixelFormat MetalFormat = (MTLPixelFormat)GPixelFormats[PF_DepthStencil].PlatformFormat;
				if (Init.DepthTargetLoadAction != ERenderTargetLoadAction::ENoAction || Init.DepthTargetStoreAction != ERenderTargetStoreAction::ENoAction)
				{
					DepthFormatKey = GetMetalPixelFormatKey(MetalFormat);
				}
				if (Init.StencilTargetLoadAction != ERenderTargetLoadAction::ENoAction || Init.StencilTargetStoreAction != ERenderTargetStoreAction::ENoAction)
				{
					StencilFormatKey = GetMetalPixelFormatKey(MTLPixelFormatStencil8);
				}
				bHasActiveTargets |= true;
				break;
			}
			case PF_ShadowDepth:
			{
				DepthFormatKey = GetMetalPixelFormatKey((MTLPixelFormat)GPixelFormats[PF_ShadowDepth].PlatformFormat);
				bHasActiveTargets |= true;
				break;
			}
			default:
			{
				break;
			}
		}
		
		// If the pixel shader writes depth then we must compile with depth access, so we may bind the dummy depth.
		// If the pixel shader writes to UAVs but not target is bound we must also bind the dummy depth.
		FMetalPixelShader* PixelShader = (FMetalPixelShader*)Init.BoundShaderState.PixelShaderRHI;
		if (PixelShader && (((PixelShader->Bindings.InOutMask & 0x8000) && (DepthFormatKey == 0)) || (bHasActiveTargets == false && PixelShader->Bindings.NumUAVs > 0)))
		{
			MTLPixelFormat MetalFormat = (MTLPixelFormat)GPixelFormats[PF_DepthStencil].PlatformFormat;
			DepthFormatKey = GetMetalPixelFormatKey(MetalFormat);
		}
			
		Key.SetHashValue(Offset_DepthFormat, NumBits_DepthFormat, DepthFormatKey);
		Key.SetHashValue(Offset_StencilFormat, NumBits_StencilFormat, StencilFormatKey);

		Key.SetHashValue(Offset_SampleCount, NumBits_SampleCount, Init.NumSamples);
		
#if PLATFORM_MAC		
		Key.SetHashValue(Offset_PrimitiveTopology, NumBits_PrimitiveTopology, TranslatePrimitiveTopology(Init.PrimitiveType));
#endif

		FMetalVertexDeclaration* VertexDecl = (FMetalVertexDeclaration*)Init.BoundShaderState.VertexDeclarationRHI;
		Key.VertexDescriptorHash = VertexDecl->Layout;
		
		FMetalVertexShader* VertexShader = (FMetalVertexShader*)Init.BoundShaderState.VertexShaderRHI;
		FMetalDomainShader* DomainShader = (FMetalDomainShader*)Init.BoundShaderState.DomainShaderRHI;
		
        Key.VertexFunction = VertexShader->GetHash();
		Key.VertexBufferHash = VertexShader->GetBindingHash(VertexBufferTypes);
		if (DomainShader)
		{
			Key.DomainFunction = DomainShader->GetHash();
			Key.SetHashValue(Offset_IndexType, NumBits_IndexType, IndexType);
			Key.DomainBufferHash = DomainShader->GetBindingHash(DomainBufferTypes);
		}
		else
		{
			Key.SetHashValue(Offset_IndexType, NumBits_IndexType, EMetalIndexType_None);
		}
		if (PixelShader)
		{
			Key.PixelFunction = PixelShader->GetHash();
			Key.PixelBufferHash = PixelShader->GetBindingHash(PixelBufferTypes);
		}
	}
};

static MTLVertexDescriptor* GetMaskedVertexDescriptor(MTLVertexDescriptor* InputDesc, uint32 InOutMask)
{
	for (uint32 Attr = 0; Attr < MaxMetalStreams; Attr++)
	{
		if (!(InOutMask & (1 << Attr)) && [InputDesc.attributes objectAtIndexedSubscript:Attr] != nil)
		{
			MTLVertexDescriptor* Desc = [[InputDesc copy] autorelease];
			uint32 BuffersUsed = 0;
			for (uint32 i = 0; i < MaxMetalStreams; i++)
			{
				if (!(InOutMask & (1 << i)))
				{
					[Desc.attributes setObject:nil atIndexedSubscript:i];
				}
				else
				{
					BuffersUsed |= (1 << [Desc.attributes objectAtIndexedSubscript:i].bufferIndex);
				}
			}
			for (uint32 i = 0; i < ML_MaxBuffers; i++)
			{
				if (!(BuffersUsed & (1 << i)))
				{
					[Desc.layouts setObject:nil atIndexedSubscript:i];
				}
			}
			return Desc;
		}
	}
	
	return InputDesc;
}

static FMetalShaderPipeline* CreateMTLRenderPipeline(bool const bSync, FMetalGraphicsPipelineKey const& Key, const FGraphicsPipelineStateInitializer& Init, EMetalIndexType const IndexType, EPixelFormat const* const VertexBufferTypes, EPixelFormat const* const PixelBufferTypes, EPixelFormat const* const DomainBufferTypes)
{
    FMetalVertexShader* VertexShader = (FMetalVertexShader*)Init.BoundShaderState.VertexShaderRHI;
    FMetalDomainShader* DomainShader = (FMetalDomainShader*)Init.BoundShaderState.DomainShaderRHI;
    FMetalPixelShader* PixelShader = (FMetalPixelShader*)Init.BoundShaderState.PixelShaderRHI;
    
    id<MTLFunction> vertexFunction = VertexShader->GetFunction(IndexType, VertexBufferTypes, Key.VertexBufferHash);
    id<MTLFunction> fragmentFunction = PixelShader ? PixelShader->GetFunction(EMetalIndexType_None, PixelBufferTypes, Key.PixelBufferHash) : nil;
    id<MTLFunction> domainFunction = DomainShader ? DomainShader->GetFunction(EMetalIndexType_None, DomainBufferTypes, Key.DomainBufferHash) : nil;
    
    FMetalShaderPipeline* Pipeline = nil;
    if (vertexFunction && ((PixelShader != nullptr) == (fragmentFunction != nil)) && ((DomainShader != nullptr) == (domainFunction != nil)))
    {
        NSError* Error = nil;
        id<MTLDevice> Device = GetMetalDeviceContext().GetDevice();

		uint32 const NumActiveTargets = Init.ComputeNumValidRenderTargets();
        check(NumActiveTargets <= MaxSimultaneousRenderTargets);
        if (PixelShader)
        {
			if ((PixelShader->Bindings.InOutMask & 0x8000) == 0 && (PixelShader->Bindings.InOutMask & 0x7fff) == 0 && PixelShader->Bindings.NumUAVs == 0 && PixelShader->Bindings.bDiscards == false)
			{
				UE_LOG(LogMetal, Error, TEXT("Pixel shader has no outputs which is not permitted. No Discards, In-Out Mask: %x\nNumber UAVs: %d\nSource Code:\n%s"), PixelShader->Bindings.InOutMask, PixelShader->Bindings.NumUAVs, *FString(PixelShader->GetSourceCode()));
				return nil;
			}
            
            UE_CLOG((NumActiveTargets < __builtin_popcount(PixelShader->Bindings.InOutMask & 0x7fff)), LogMetal, Verbose, TEXT("NumActiveTargets doesn't match pipeline's pixel shader output mask: %u, %hx"), NumActiveTargets, PixelShader->Bindings.InOutMask);
        }
        
		Pipeline = [FMetalShaderPipeline new];
		METAL_DEBUG_OPTION(FMemory::Memzero(Pipeline->ResourceMask, sizeof(Pipeline->ResourceMask)));

		MTLRenderPipelineDescriptor* RenderPipelineDesc = [MTLRenderPipelineDescriptor new];
		MTLComputePipelineDescriptor* ComputePipelineDesc = nil;
		
        FMetalBlendState* BlendState = (FMetalBlendState*)Init.BlendState;
        
        for (uint32 i = 0; i < NumActiveTargets; i++)
        {
            EPixelFormat TargetFormat = Init.RenderTargetFormats[i];
            if (TargetFormat == PF_Unknown)
            {
                UE_CLOG(PixelShader && (((PixelShader->Bindings.InOutMask & 0x7fff) & (1 << i))), LogMetal, Warning, TEXT("Pipeline pixel shader expects target %u to be bound but it isn't."), i);
                continue;
            }
            
            MTLPixelFormat MetalFormat = (MTLPixelFormat)GPixelFormats[TargetFormat].PlatformFormat;
            uint32 Flags = Init.RenderTargetFlags[i];
            if (Flags & TexCreate_SRGB)
            {
        #if PLATFORM_MAC // Expand as R8_sRGB is iOS only.
                if (MetalFormat == MTLPixelFormatR8Unorm)
                {
                    MetalFormat = MTLPixelFormatRGBA8Unorm;
                }
        #endif
                MetalFormat = ToSRGBFormat(MetalFormat);
            }
            
            MTLRenderPipelineColorAttachmentDescriptor* Attachment = [RenderPipelineDesc.colorAttachments objectAtIndexedSubscript:i];
            Attachment.pixelFormat = MetalFormat;
            
            MTLRenderPipelineColorAttachmentDescriptor* Blend = BlendState->RenderTargetStates[i].BlendState;
            if(Attachment)
            {
                // assign each property manually, would be nice if this was faster
                Attachment.blendingEnabled = Blend.blendingEnabled;
                Attachment.sourceRGBBlendFactor = Blend.sourceRGBBlendFactor;
                Attachment.destinationRGBBlendFactor = Blend.destinationRGBBlendFactor;
                Attachment.rgbBlendOperation = Blend.rgbBlendOperation;
                Attachment.sourceAlphaBlendFactor = Blend.sourceAlphaBlendFactor;
                Attachment.destinationAlphaBlendFactor = Blend.destinationAlphaBlendFactor;
                Attachment.alphaBlendOperation = Blend.alphaBlendOperation;
                Attachment.writeMask = Blend.writeMask;
            }
            else
            {
                Attachment.blendingEnabled = NO;
                Attachment.writeMask = 0;
            }
        }
        
        switch(Init.DepthStencilTargetFormat)
        {
            case PF_DepthStencil:
            {
                MTLPixelFormat MetalFormat = (MTLPixelFormat)GPixelFormats[PF_DepthStencil].PlatformFormat;
                if(MetalFormat == MTLPixelFormatDepth32Float)
                {
                    if (Init.DepthTargetLoadAction != ERenderTargetLoadAction::ENoAction || Init.DepthTargetStoreAction != ERenderTargetStoreAction::ENoAction)
                    {
                        RenderPipelineDesc.depthAttachmentPixelFormat = MetalFormat;
                    }
                    if (Init.StencilTargetLoadAction != ERenderTargetLoadAction::ENoAction || Init.StencilTargetStoreAction != ERenderTargetStoreAction::ENoAction)
                    {
                        RenderPipelineDesc.stencilAttachmentPixelFormat = MTLPixelFormatStencil8;
                    }
                }
                else
                {
                    RenderPipelineDesc.depthAttachmentPixelFormat = MetalFormat;
                    RenderPipelineDesc.stencilAttachmentPixelFormat = MetalFormat;
                }
                break;
            }
            case PF_ShadowDepth:
            {
                RenderPipelineDesc.depthAttachmentPixelFormat = (MTLPixelFormat)GPixelFormats[PF_ShadowDepth].PlatformFormat;
                break;
            }
            default:
            {
                break;
            }
        }
        
        check(Init.BoundShaderState.VertexShaderRHI != nullptr);
        check(Init.BoundShaderState.GeometryShaderRHI == nullptr);

        FMetalHullShader* HullShader = (FMetalHullShader*)Init.BoundShaderState.HullShaderRHI;
        
        if(RenderPipelineDesc.depthAttachmentPixelFormat == MTLPixelFormatInvalid && PixelShader && ((PixelShader->Bindings.InOutMask & 0x8000) || (NumActiveTargets == 0 && (PixelShader->Bindings.NumUAVs > 0))))
        {
            RenderPipelineDesc.depthAttachmentPixelFormat = (MTLPixelFormat)GPixelFormats[PF_DepthStencil].PlatformFormat;
            RenderPipelineDesc.stencilAttachmentPixelFormat = (MTLPixelFormat)GPixelFormats[PF_DepthStencil].PlatformFormat;
        }
        
        RenderPipelineDesc.sampleCount = FMath::Max(Init.NumSamples, 1u);
    #if PLATFORM_MAC
        RenderPipelineDesc.inputPrimitiveTopology = TranslatePrimitiveTopology(Init.PrimitiveType);
    #endif
        
        FMetalVertexDeclaration* VertexDecl = (FMetalVertexDeclaration*)Init.BoundShaderState.VertexDeclarationRHI;
        
        if (Init.BoundShaderState.HullShaderRHI == nullptr)
        {
            check(Init.BoundShaderState.DomainShaderRHI == nullptr);
            RenderPipelineDesc.vertexDescriptor = GetMaskedVertexDescriptor(VertexDecl->Layout.VertexDesc, VertexShader->Bindings.InOutMask);
            RenderPipelineDesc.vertexFunction = vertexFunction;
            RenderPipelineDesc.fragmentFunction = fragmentFunction;
        }
        else
        {
            check(Init.BoundShaderState.DomainShaderRHI != nullptr);
            
            RenderPipelineDesc.tessellationPartitionMode = GMetalTessellationForcePartitionMode == 0 ? DomainShader->TessellationPartitioning : (MTLTessellationPartitionMode)(GMetalTessellationForcePartitionMode - 1);
            RenderPipelineDesc.tessellationFactorStepFunction = MTLTessellationFactorStepFunctionPerPatch;
            RenderPipelineDesc.tessellationOutputWindingOrder = DomainShader->TessellationOutputWinding;
            int FixedMaxTessFactor = (int)RoundTessLevel(VertexShader->TessellationMaxTessFactor, RenderPipelineDesc.tessellationPartitionMode);
            RenderPipelineDesc.maxTessellationFactor = FixedMaxTessFactor;
            RenderPipelineDesc.tessellationFactorScaleEnabled = NO;
            RenderPipelineDesc.tessellationFactorFormat = MTLTessellationFactorFormatHalf;
            RenderPipelineDesc.tessellationControlPointIndexType = MTLTessellationControlPointIndexTypeNone;
            
            RenderPipelineDesc.vertexFunction = domainFunction;
            RenderPipelineDesc.fragmentFunction = fragmentFunction;
            
            ComputePipelineDesc = [MTLComputePipelineDescriptor new];
            check(ComputePipelineDesc);
            
            RenderPipelineDesc.vertexDescriptor = [[MTLVertexDescriptor new] autorelease];
            
            FMetalTessellationPipelineDesc* TessellationDesc = [[FMetalTessellationPipelineDesc new] autorelease];
            Pipeline.TessellationPipelineDesc = TessellationDesc;
            check(Pipeline.TessellationPipelineDesc);
            
            TessellationDesc.TessellationInputControlPointBufferIndex = DomainShader->TessellationControlPointOutBuffer;
            TessellationDesc.TessellationOutputControlPointBufferIndex = VertexShader->TessellationControlPointOutBuffer;
            TessellationDesc.TessellationInputPatchConstBufferIndex = DomainShader->TessellationHSOutBuffer;
            TessellationDesc.TessellationPatchConstBufferIndex = VertexShader->TessellationHSOutBuffer;
            TessellationDesc.TessellationFactorBufferIndex = VertexShader->TessellationHSTFOutBuffer;
            TessellationDesc.TessellationPatchCountBufferIndex = VertexShader->TessellationPatchCountBuffer;
            TessellationDesc.TessellationIndexBufferIndex = VertexShader->TessellationIndexBuffer;
            TessellationDesc.TessellationPatchConstOutSize = VertexShader->TessellationOutputAttribs.HSOutSize;
            TessellationDesc.TessellationControlPointIndexBufferIndex = ComputePipelineDesc.stageInputDescriptor.indexBufferIndex = VertexShader->TessellationControlPointIndexBuffer;
            TessellationDesc.DomainVertexDescriptor = RenderPipelineDesc.vertexDescriptor;
            TessellationDesc.DSNumUniformBuffers = DomainShader->Bindings.NumUniformBuffers;
            TessellationDesc.TessellationPatchControlPointOutSize = VertexShader->TessellationOutputAttribs.PatchControlPointOutSize;
            TessellationDesc.TessellationTessFactorOutSize = VertexShader->TessellationOutputAttribs.HSTFOutSize;
            
            check(TessellationDesc.TessellationOutputControlPointBufferIndex < ML_MaxBuffers);
            check(TessellationDesc.TessellationFactorBufferIndex < ML_MaxBuffers);
            check(TessellationDesc.TessellationPatchCountBufferIndex < ML_MaxBuffers);
            check(TessellationDesc.TessellationTessFactorOutSize == 2*4 || TessellationDesc.TessellationTessFactorOutSize == 2*6);
            
            MTLVertexStepFunction stepFunction = MTLVertexStepFunctionPerPatch;
            
            static MTLVertexFormat Formats[(uint8)EMetalComponentType::Max][4] = {
                {MTLVertexFormatUInt, MTLVertexFormatUInt2, MTLVertexFormatUInt3, MTLVertexFormatUInt4},
                {MTLVertexFormatInt, MTLVertexFormatInt2, MTLVertexFormatInt3, MTLVertexFormatInt4},
                {MTLVertexFormatInvalid, MTLVertexFormatHalf2, MTLVertexFormatHalf3, MTLVertexFormatHalf4},
                {MTLVertexFormatFloat, MTLVertexFormatFloat2, MTLVertexFormatFloat3, MTLVertexFormatFloat4},
                {MTLVertexFormatInvalid, MTLVertexFormatUChar2, MTLVertexFormatUChar3, MTLVertexFormatUChar4},
            };
            
            if (DomainShader->TessellationHSOutBuffer != UINT_MAX)
            {
                check(DomainShader->TessellationHSOutBuffer < ML_MaxBuffers);
                uint32 bufferIndex = DomainShader->TessellationHSOutBuffer;
                uint32 bufferSize = VertexShader->TessellationOutputAttribs.HSOutSize;
               
                RenderPipelineDesc.vertexDescriptor.layouts[bufferIndex].stride = bufferSize;
                RenderPipelineDesc.vertexDescriptor.layouts[bufferIndex].stepFunction = stepFunction;
                RenderPipelineDesc.vertexDescriptor.layouts[bufferIndex].stepRate = 1;
            
                for (FMetalAttribute const& Attrib : VertexShader->TessellationOutputAttribs.HSOut)
                {
                    int attributeIndex = Attrib.Index;
                    check(attributeIndex >= 0 && attributeIndex <= 31);
                    check(Attrib.Components > 0 && Attrib.Components <= 4);
                    MTLVertexFormat format = Formats[(uint8)Attrib.Type][Attrib.Components-1];
                    check(format != MTLVertexFormatInvalid); // TODO support more cases
                    RenderPipelineDesc.vertexDescriptor.attributes[attributeIndex].format = format;
                    RenderPipelineDesc.vertexDescriptor.attributes[attributeIndex].offset = Attrib.Offset;
                    RenderPipelineDesc.vertexDescriptor.attributes[attributeIndex].bufferIndex = bufferIndex;
                }
            }
            
            stepFunction = MTLVertexStepFunctionPerPatchControlPoint;
            uint32 bufferIndex = DomainShader->TessellationControlPointOutBuffer;
            uint32 bufferSize = VertexShader->TessellationOutputAttribs.PatchControlPointOutSize;
            
            RenderPipelineDesc.vertexDescriptor.layouts[bufferIndex].stride = bufferSize;
            RenderPipelineDesc.vertexDescriptor.layouts[bufferIndex].stepFunction = stepFunction;
            RenderPipelineDesc.vertexDescriptor.layouts[bufferIndex].stepRate = 1;
            
            for (FMetalAttribute const& Attrib : VertexShader->TessellationOutputAttribs.PatchControlPointOut)
            {
                int attributeIndex = Attrib.Index;
                check(attributeIndex >= 0 && attributeIndex <= 31);
                check(Attrib.Components > 0 && Attrib.Components <= 4);
                MTLVertexFormat format = Formats[(uint8)Attrib.Type][Attrib.Components-1];
                check(format != MTLVertexFormatInvalid); // TODO support more cases
                RenderPipelineDesc.vertexDescriptor.attributes[attributeIndex].format = format;
                RenderPipelineDesc.vertexDescriptor.attributes[attributeIndex].offset = Attrib.Offset;
                RenderPipelineDesc.vertexDescriptor.attributes[attributeIndex].bufferIndex = bufferIndex;
            }
            
            bool const bIsIndexed = (IndexType == EMetalIndexType_UInt16 || IndexType == EMetalIndexType_UInt32);
            
            MTLVertexDescriptor* VertexDesc = GetMaskedVertexDescriptor(VertexDecl->Layout.VertexDesc, VertexShader->Bindings.InOutMask);
            for(int onIndex = 0; onIndex < MaxMetalStreams; onIndex++)
            {
                // NOTE: accessing the VertexDesc like this will end up allocating layouts/attributes
                auto stride = VertexDesc.layouts[onIndex].stride;
                if(stride)
                {
                    ComputePipelineDesc.stageInputDescriptor.layouts[onIndex].stride = stride;
                    auto InnerStepFunction = VertexDesc.layouts[onIndex].stepFunction;
                    switch(InnerStepFunction)
                    {
                        case MTLVertexStepFunctionConstant:
                            ComputePipelineDesc.stageInputDescriptor.layouts[onIndex].stepFunction = MTLStepFunctionConstant;
                            break;
                        case MTLVertexStepFunctionPerVertex:
                            ComputePipelineDesc.stageInputDescriptor.layouts[onIndex].stepFunction = bIsIndexed ? MTLStepFunctionThreadPositionInGridXIndexed : MTLStepFunctionThreadPositionInGridX;
                            break;
                        case MTLVertexStepFunctionPerInstance:
                            ComputePipelineDesc.stageInputDescriptor.layouts[onIndex].stepFunction = MTLStepFunctionThreadPositionInGridY;
                            break;
                        default:
                            check(0);
                    }
                    ComputePipelineDesc.stageInputDescriptor.layouts[onIndex].stepRate = VertexDesc.layouts[onIndex].stepRate;
                }
                auto format = VertexDesc.attributes[onIndex].format;
                if(format == MTLVertexFormatInvalid) continue;
                {
                    ComputePipelineDesc.stageInputDescriptor.attributes[onIndex].format = (MTLAttributeFormat)format; // TODO FIXME currently these align perfectly (at least assert that is the case)
                    ComputePipelineDesc.stageInputDescriptor.attributes[onIndex].offset = VertexDesc.attributes[onIndex].offset;
                    ComputePipelineDesc.stageInputDescriptor.attributes[onIndex].bufferIndex = VertexDesc.attributes[onIndex].bufferIndex;
                }
            }
            
            // Disambiguated function name.
            ComputePipelineDesc.computeFunction = vertexFunction;
            check(ComputePipelineDesc.computeFunction);

            // Don't set the index type if there isn't an index buffer.
            if (IndexType != EMetalIndexType_None)
            {
                ComputePipelineDesc.stageInputDescriptor.indexType = GetMetalIndexType(IndexType);
            }
            
            {
                Pipeline.ComputePipelineState = [Device newComputePipelineStateWithDescriptor:ComputePipelineDesc options:MTLPipelineOptionNone reflection:nil error:&Error];
                
                MTLPipelineOption ComputeOption = MTLPipelineOptionNone;
 #if METAL_DEBUG_OPTIONS
	            if (GetMetalDeviceContext().GetCommandQueue().GetRuntimeDebuggingLevel() >= EMetalDebugLevelFastValidation)
	            {
	            	ComputeOption = MTLPipelineOptionArgumentInfo|MTLPipelineOptionBufferTypeInfo;
	            }
#endif                
               	MTLAutoreleasedComputePipelineReflection Reflection = nil;
				Pipeline.ComputePipelineState = [Device newComputePipelineStateWithDescriptor:ComputePipelineDesc options:ComputeOption reflection:&Reflection error:&Error];
#if METAL_DEBUG_OPTIONS
				Pipeline.ComputePipelineReflection = Reflection;
#endif
				UE_CLOG((Pipeline.ComputePipelineState == nil), LogMetal, Error, TEXT("Failed to generate a pipeline state object: %s"), *FString(Error.description));
				UE_CLOG((Pipeline.ComputePipelineState == nil), LogMetal, Error, TEXT("Vertex shader: %s"), *FString(VertexShader->GetSourceCode()));
				UE_CLOG((Pipeline.ComputePipelineState == nil), LogMetal, Error, TEXT("Pixel shader: %s"), PixelShader ? *FString(PixelShader->GetSourceCode()) : TEXT("NULL"));
				UE_CLOG((Pipeline.ComputePipelineState == nil), LogMetal, Error, TEXT("Hull shader: %s"), *FString(HullShader->GetSourceCode()));
				UE_CLOG((Pipeline.ComputePipelineState == nil), LogMetal, Error, TEXT("Domain shader: %s"), *FString(DomainShader->GetSourceCode()));
				UE_CLOG((Pipeline.ComputePipelineState == nil), LogMetal, Error, TEXT("Descriptor: %s"), *FString(ComputePipelineDesc.description));
				UE_CLOG((Pipeline.ComputePipelineState == nil), LogMetal, Fatal, TEXT("Failed to generate a hull pipeline state object:\n\n %s\n\n"), *FString([Error localizedDescription]));
				
				TRACK_OBJECT(STAT_MetalRenderPipelineStateCount, Pipeline.ComputePipelineState);
#if METAL_DEBUG_OPTIONS
				if (Reflection)
				{
					bool found__HSTFOut = false;
					for(MTLArgument* arg in Reflection.arguments)
					{
						bool addAttributes = false;
						MTLVertexStepFunction StepFunction = (MTLVertexStepFunction)-1; // invalid
						
						uint32 BufferIndex = UINT_MAX;
						
						if([arg.name isEqualToString: @"PatchControlPointOutBuffer"])
						{
							check((arg.bufferAlignment & (arg.bufferAlignment - 1)) == 0); // must be pow2
							check((arg.bufferDataSize & (arg.bufferAlignment - 1)) == 0); // must be aligned
							
							check(arg.bufferDataSize == VertexShader->TessellationOutputAttribs.PatchControlPointOutSize);
							
							addAttributes = true;
							BufferIndex = DomainShader->TessellationControlPointOutBuffer;
							StepFunction = MTLVertexStepFunctionPerPatchControlPoint;
							check(arg.index == VertexShader->TessellationControlPointOutBuffer);
						}
						else if([arg.name isEqualToString: @"__HSOut"])
						{
							check((arg.bufferAlignment & (arg.bufferAlignment - 1)) == 0); // must be pow2
							check((arg.bufferDataSize & (arg.bufferAlignment - 1)) == 0); // must be aligned
							
							check(arg.bufferDataSize == VertexShader->TessellationOutputAttribs.HSOutSize);
							
							addAttributes = true;
							BufferIndex = DomainShader->TessellationHSOutBuffer;
							StepFunction = MTLVertexStepFunctionPerPatch;
							check(arg.index == VertexShader->TessellationHSOutBuffer);
						}
						else if([arg.name isEqualToString: @"__HSTFOut"])
						{
							found__HSTFOut = true;
							check((arg.bufferAlignment & (arg.bufferAlignment - 1)) == 0); // must be pow2
							check((arg.bufferDataSize & (arg.bufferAlignment - 1)) == 0); // must be aligned
							
							check(arg.bufferDataSize == VertexShader->TessellationOutputAttribs.HSTFOutSize);
							
							check(arg.index == VertexShader->TessellationHSTFOutBuffer);
						}
						else if([arg.name isEqualToString:@"patchCount"])
						{
							check(arg.index == VertexShader->TessellationPatchCountBuffer);
						}
						else if([arg.name isEqualToString:@"indexBuffer"])
						{
							check(arg.index == VertexShader->TessellationIndexBuffer);
						}
						
						// build the vertexDescriptor
						if(addAttributes)
						{
							check(RenderPipelineDesc.vertexDescriptor.layouts[BufferIndex].stride == arg.bufferDataSize);
							check(RenderPipelineDesc.vertexDescriptor.layouts[BufferIndex].stepFunction == StepFunction);
							check(RenderPipelineDesc.vertexDescriptor.layouts[BufferIndex].stepRate == 1);
							for(MTLStructMember *attribute in arg.bufferStructType.members)
							{
								int attributeIndex = -1;
								sscanf([attribute.name UTF8String], "OUT_ATTRIBUTE%d_", &attributeIndex);
								check(attributeIndex >= 0 && attributeIndex <= 31);
								MTLVertexFormat format = MTLVertexFormatInvalid;
								switch(attribute.dataType)
								{
									case MTLDataTypeFloat:  format = MTLVertexFormatFloat; break;
									case MTLDataTypeFloat2: format = MTLVertexFormatFloat2; break;
									case MTLDataTypeFloat3: format = MTLVertexFormatFloat3; break;
									case MTLDataTypeFloat4: format = MTLVertexFormatFloat4; break;
										
									case MTLDataTypeInt:  format = MTLVertexFormatInt; break;
									case MTLDataTypeInt2: format = MTLVertexFormatInt2; break;
									case MTLDataTypeInt3: format = MTLVertexFormatInt3; break;
									case MTLDataTypeInt4: format = MTLVertexFormatInt4; break;
										
									case MTLDataTypeUInt:  format = MTLVertexFormatUInt; break;
									case MTLDataTypeUInt2: format = MTLVertexFormatUInt2; break;
									case MTLDataTypeUInt3: format = MTLVertexFormatUInt3; break;
									case MTLDataTypeUInt4: format = MTLVertexFormatUInt4; break;
										
									default: check(0); // TODO support more cases
								}
								check(RenderPipelineDesc.vertexDescriptor.attributes[attributeIndex].format == format);
								check(RenderPipelineDesc.vertexDescriptor.attributes[attributeIndex].offset == attribute.offset);
								check(RenderPipelineDesc.vertexDescriptor.attributes[attributeIndex].bufferIndex == BufferIndex);
							}
						}
					}
					check(found__HSTFOut);
				}
#endif
				[ComputePipelineDesc release];
            }
        }
        
        MTLPipelineOption RenderOption = MTLPipelineOptionNone;
#if METAL_DEBUG_OPTIONS
        if (GetMetalDeviceContext().GetCommandQueue().GetRuntimeDebuggingLevel() >= EMetalDebugLevelFastValidation)
        {
        	RenderOption = MTLPipelineOptionArgumentInfo|MTLPipelineOptionBufferTypeInfo;
        }
#endif

		MTLAutoreleasedRenderPipelineReflection Reflection = nil;
		Pipeline.RenderPipelineState = [Device newRenderPipelineStateWithDescriptor:RenderPipelineDesc options:RenderOption reflection:&Reflection error:&Error];
#if METAL_DEBUG_OPTIONS
		Pipeline.RenderPipelineReflection = Reflection;
#endif
		
		UE_CLOG((Pipeline.RenderPipelineState == nil), LogMetal, Error, TEXT("Failed to generate a pipeline state object: %s"), *FString(Error.description));
		UE_CLOG((Pipeline.RenderPipelineState == nil), LogMetal, Error, TEXT("Vertex shader: %s"), *FString(VertexShader->GetSourceCode()));
		UE_CLOG((Pipeline.RenderPipelineState == nil), LogMetal, Error, TEXT("Pixel shader: %s"), PixelShader ? *FString(PixelShader->GetSourceCode()) : TEXT("NULL"));
		UE_CLOG((Pipeline.RenderPipelineState == nil), LogMetal, Error, TEXT("Hull shader: %s"), HullShader ? *FString(HullShader->GetSourceCode()) : TEXT("NULL"));
		UE_CLOG((Pipeline.RenderPipelineState == nil), LogMetal, Error, TEXT("Domain shader: %s"), DomainShader ? *FString(DomainShader->GetSourceCode()) : TEXT("NULL"));
		UE_CLOG((Pipeline.RenderPipelineState == nil), LogMetal, Error, TEXT("Descriptor: %s"), *FString(RenderPipelineDesc.description));
		UE_CLOG((Pipeline.RenderPipelineState == nil), LogMetal, Fatal, TEXT("Failed to generate a render pipeline state object:\n\n %s\n\n"), *FString([Error localizedDescription]));
		
		TRACK_OBJECT(STAT_MetalRenderPipelineStateCount, Pipeline.RenderPipelineState);
		[RenderPipelineDesc release];
        
    #if METAL_DEBUG_OPTIONS
        Pipeline.ComputeSource = DomainShader ? VertexShader->GetSourceCode() : nil;
        Pipeline.VertexSource = DomainShader ? DomainShader->GetSourceCode() : VertexShader->GetSourceCode();
        Pipeline.FragmentSource = PixelShader ? PixelShader->GetSourceCode() : nil;
    #endif
        
    #if 0
        if (GFrameCounter > 3)
        {
            NSLog(@"===============================================================");
            NSLog(@"Creating a BSS at runtime frame %lld... this may hitch! [this = %p]", GFrameCounter, Pipeline);
            NSLog(@"Vertex declaration:");
            FVertexDeclarationElementList& Elements = VertexDecl->Elements;
            for (int32 i = 0; i < Elements.Num(); i++)
            {
                FVertexElement& Elem = Elements[i];
                NSLog(@"   Elem %d: attr: %d, stream: %d, type: %d, stride: %d, offset: %d", i, Elem.AttributeIndex, Elem.StreamIndex, (uint32)Elem.Type, Elem.Stride, Elem.Offset);
            }
            
            NSLog(@"\nVertexShader:");
            NSLog(@"%@", VertexShader ? VertexShader->GetSourceCode() : @"NONE");
            NSLog(@"\nPixelShader:");
            NSLog(@"%@", PixelShader ? PixelShader->GetSourceCode() : @"NONE");
            NSLog(@"\nDomainShader:");
            NSLog(@"%@", DomainShader ? DomainShader->GetSourceCode() : @"NONE");
            NSLog(@"===============================================================");
        }
    #endif
        
#if METAL_DEBUG_OPTIONS
        if (GFrameCounter > 3)
        {
            UE_LOG(LogMetal, Verbose, TEXT("Created a hitchy pipeline state for hash %llx %llx %llx"), (uint64)Key.RenderPipelineHash.RasterBits, (uint64)(Key.RenderPipelineHash.TargetBits), (uint64)Key.VertexDescriptorHash.VertexDescHash);
        }
#endif

    }
    return !bSync ? nil : Pipeline;
}

static FMetalShaderPipeline* GetMTLRenderPipeline(bool const bSync, FMetalGraphicsPipelineState const* State, const FGraphicsPipelineStateInitializer& Init, EMetalIndexType const IndexType, EPixelFormat const* const VertexBufferTypes, EPixelFormat const* const PixelBufferTypes, EPixelFormat const* const DomainBufferTypes)
{
	static FRWLock PipelineMutex;
	static TMap<FMetalGraphicsPipelineKey, FMetalShaderPipeline*> Pipelines;

	FMetalGraphicsPipelineKey Key;
	InitMetalGraphicsPipelineKey(Key, Init, IndexType, VertexBufferTypes, PixelBufferTypes, DomainBufferTypes);

	// By default there'll be more threads trying to read this than to write it.
	FRWScopeLock Lock(PipelineMutex, SLT_ReadOnly);

	// Try to find the entry in the cache.
	FMetalShaderPipeline* Desc = Pipelines.FindRef(Key);
	if (Desc == nil)
	{
		// Now we are a writer as we want to create & add the new pipeline
		Lock.ReleaseReadOnlyLockAndAcquireWriteLock_USE_WITH_CAUTION();
		
		// Retest to ensure no-one beat us here!
		Desc = Pipelines.FindRef(Key);
		if (Desc == nil)
		{
			Desc = CreateMTLRenderPipeline(bSync, Key, Init, IndexType, VertexBufferTypes, PixelBufferTypes, DomainBufferTypes);
			Pipelines.Add(Key, Desc);
		}
	}
	check(Desc);
	
	return Desc;
}

FMetalGraphicsPipelineState::FMetalGraphicsPipelineState(const FGraphicsPipelineStateInitializer& Init)
: Initializer(Init)
{
	FMemory::Memzero(PipelineStates);
	for (uint32 i = 0; i < EMetalIndexType_Num; i++)
	{
		PipelineStates[i][0][0][0] = [GetMTLRenderPipeline(true, this, Init, (EMetalIndexType)i, nullptr, nullptr, nullptr) retain];
        check(PipelineStates[i][0][0][0]);
	}
}

FMetalGraphicsPipelineState::~FMetalGraphicsPipelineState()
{
	for (uint32 i = 0; i < EMetalIndexType_Num; i++)
	{
        for (uint32 v = 0; v < EMetalBufferType_Num; v++)
        {
            for (uint32 f = 0; f < EMetalBufferType_Num; f++)
            {
                for (uint32 c = 0; c < EMetalBufferType_Num; c++)
                {
                	if (PipelineStates[i][v][f][c])
                	{
	                    [PipelineStates[i][v][f][c] release];
	                    PipelineStates[i][v][f][c] = nil;
                    }
                }
            }
        }
	}
}

FMetalShaderPipeline* FMetalGraphicsPipelineState::GetPipeline(EMetalIndexType IndexType, uint32 VertexBufferHash, uint32 PixelBufferHash, uint32 DomainBufferHash, EPixelFormat const* const VertexBufferTypes, EPixelFormat const* const PixelBufferTypes, EPixelFormat const* const DomainBufferTypes)
{
	check(IndexType < EMetalIndexType_Num);

	EMetalBufferType Vertex = VertexShader && (VertexShader->BufferTypeHash && VertexShader->BufferTypeHash == VertexBufferHash) ? EMetalBufferType_Static : EMetalBufferType_Dynamic;
	EMetalBufferType Fragment = PixelShader && (PixelShader->BufferTypeHash && PixelShader->BufferTypeHash == PixelBufferHash) ? EMetalBufferType_Static : EMetalBufferType_Dynamic;
	EMetalBufferType Compute = DomainShader && (DomainShader->BufferTypeHash && DomainShader->BufferTypeHash == DomainBufferHash) ? EMetalBufferType_Static : EMetalBufferType_Dynamic;

    FMetalShaderPipeline* Pipe = PipelineStates[IndexType][Vertex][Fragment][Compute];
	if (!Pipe)
	{
		Pipe = PipelineStates[IndexType][Vertex][Fragment][Compute] = [GetMTLRenderPipeline(true, this, Initializer, IndexType, VertexBufferTypes, PixelBufferTypes, DomainBufferTypes) retain];
	}
    if (!Pipe)
    {
    	if(!PipelineStates[IndexType][0][0][0])
    	{
    		PipelineStates[IndexType][0][0][0] = [GetMTLRenderPipeline(true, this, Initializer, IndexType, nullptr, nullptr, nullptr) retain];
    	}
        Pipe = PipelineStates[IndexType][0][0][0];
    }
	check(Pipe);
    return Pipe;
}


FGraphicsPipelineStateRHIRef FMetalDynamicRHI::RHICreateGraphicsPipelineState(const FGraphicsPipelineStateInitializer& Initializer)
{
	@autoreleasepool {
	FMetalGraphicsPipelineState* State = new FMetalGraphicsPipelineState(Initializer);
	check(State);
	State->VertexDeclaration = ResourceCast(Initializer.BoundShaderState.VertexDeclarationRHI);
	State->VertexShader = ResourceCast(Initializer.BoundShaderState.VertexShaderRHI);
	State->PixelShader = ResourceCast(Initializer.BoundShaderState.PixelShaderRHI);
	State->HullShader = ResourceCast(Initializer.BoundShaderState.HullShaderRHI);
	State->DomainShader = ResourceCast(Initializer.BoundShaderState.DomainShaderRHI);
	State->GeometryShader = ResourceCast(Initializer.BoundShaderState.GeometryShaderRHI);
	State->DepthStencilState = ResourceCast(Initializer.DepthStencilState);
	State->RasterizerState = ResourceCast(Initializer.RasterizerState);
	FShaderCache::LogGraphicsPipelineState(ImmediateContext.GetInternalContext().GetCurrentState().GetShaderCacheStateObject(), GMaxRHIShaderPlatform, Initializer, State);
	return State;
	}
}

TRefCountPtr<FRHIComputePipelineState> FMetalDynamicRHI::RHICreateComputePipelineState(FRHIComputeShader* ComputeShader)
{
	@autoreleasepool {
	return new FMetalComputePipelineState(ResourceCast(ComputeShader));
	}
}
