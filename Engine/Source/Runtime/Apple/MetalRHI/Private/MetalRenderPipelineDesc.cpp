// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "MetalRHIPrivate.h"
#include "MetalRenderPipelineDesc.h"
#include "MetalProfiler.h"
#include "MetalCommandQueue.h"
#include "MetalCommandBuffer.h"
#include "RenderUtils.h"
#include <objc/runtime.h>

static int32 GMetalTessellationForcePartitionMode = 0;
static FAutoConsoleVariableRef CVarMetalTessellationForcePartitionMode(
	TEXT("rhi.Metal.TessellationForcePartitionMode"),
	GMetalTessellationForcePartitionMode,
	TEXT("The partition mode (+1) to force Metal to use for debugging or off (0). (Default: 0)"));

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
			return FMath::RoundUpToPowerOfTwo(TessFactor);
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
TMap<FMetalRenderPipelineDesc::FMetalRenderPipelineKey, FMetalShaderPipeline*> FMetalRenderPipelineDesc::MetalPipelineCache;
FMetalRenderPipelineDesc::FMetalRWMutex FMetalRenderPipelineDesc::MetalPipelineMutex;

@implementation FMetalShaderPipeline

APPLE_PLATFORM_OBJECT_ALLOC_OVERRIDES(FMetalShaderPipeline)
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
@end

static_assert(Offset_RasterEnd < 64 && Offset_End < 128, "Offset_RasterEnd must be < 64 && Offset_End < 128");

uint32 FMetalRenderPipelineDesc::BlendBitOffsets[] = { Offset_BlendState0, Offset_BlendState1, Offset_BlendState2, Offset_BlendState3, Offset_BlendState4, Offset_BlendState5, Offset_BlendState6, Offset_BlendState7 };
uint32 FMetalRenderPipelineDesc::RTBitOffsets[] = { Offset_RenderTargetFormat0, Offset_RenderTargetFormat1, Offset_RenderTargetFormat2, Offset_RenderTargetFormat3, Offset_RenderTargetFormat4, Offset_RenderTargetFormat5, Offset_RenderTargetFormat6, Offset_RenderTargetFormat7 };

FMetalRenderPipelineDesc::FMetalRenderPipelineDesc()
: PipelineDescriptor([[MTLRenderPipelineDescriptor alloc] init])
, SampleCount(0)
{
	Hash.RasterBits = 0;
	Hash.TargetBits = 0;
	for (int Index = 0; Index < MaxSimultaneousRenderTargets; Index++)
	{
		[PipelineDescriptor.colorAttachments setObject:[[MTLRenderPipelineColorAttachmentDescriptor new] autorelease] atIndexedSubscript:Index];
	}
}

FMetalRenderPipelineDesc::~FMetalRenderPipelineDesc()
{
	
}

FMetalShaderPipeline* FMetalRenderPipelineDesc::CreatePipelineStateForBoundShaderState(FMetalBoundShaderState* BSS, FMetalHashedVertexDescriptor const& VertexDesc) const
{
	SCOPE_CYCLE_COUNTER(STAT_MetalPipelineStateTime);
	
	MTLPixelFormat DepthFormat = PipelineDescriptor.depthAttachmentPixelFormat;
	MTLPixelFormat StencilFormat = PipelineDescriptor.stencilAttachmentPixelFormat;
	if(BSS->PixelShader && (BSS->PixelShader->Bindings.InOutMask & 0x8000) && PipelineDescriptor.depthAttachmentPixelFormat == MTLPixelFormatInvalid)
	{
		PipelineDescriptor.depthAttachmentPixelFormat = (MTLPixelFormat)GPixelFormats[PF_DepthStencil].PlatformFormat;
		PipelineDescriptor.stencilAttachmentPixelFormat = (MTLPixelFormat)GPixelFormats[PF_DepthStencil].PlatformFormat;
	}
	
	// Disable blending and writing on unbound targets or Metal will assert/crash/abort depending on build.
	// At least with this API all the state must match all of the time for it to work.
	for (int Index = 0; Index < MaxSimultaneousRenderTargets; Index++)
	{
		MTLRenderPipelineColorAttachmentDescriptor* Desc = [PipelineDescriptor.colorAttachments objectAtIndexedSubscript:Index];
		if(Desc.pixelFormat == MTLPixelFormatInvalid)
		{
			Desc.blendingEnabled = NO;
			Desc.writeMask = 0;
		}
	}
	
	// set the bound shader state settings
	id<MTLFunction> computeFunction = BSS->DomainShader ? BSS->VertexShader->Function : nil;
	PipelineDescriptor.vertexDescriptor = BSS->DomainShader ? nil : VertexDesc.VertexDesc;
	PipelineDescriptor.vertexFunction = BSS->DomainShader ? BSS->DomainShader->Function : BSS->VertexShader->Function;
	PipelineDescriptor.fragmentFunction = BSS->PixelShader ? BSS->PixelShader->Function : nil;
	PipelineDescriptor.sampleCount = SampleCount;

	TArray<uint32> FunctionConstants;
	
	if(BSS->HullShader)
	{
		check(BSS->DomainShader);
		PipelineDescriptor.tessellationPartitionMode = GMetalTessellationForcePartitionMode == 0 ? BSS->DomainShader->TessellationPartitioning : (MTLTessellationPartitionMode)(GMetalTessellationForcePartitionMode - 1);
		PipelineDescriptor.tessellationFactorStepFunction = MTLTessellationFactorStepFunctionPerPatch;
		PipelineDescriptor.tessellationOutputWindingOrder = BSS->DomainShader->TessellationOutputWinding;
		int FixedMaxTessFactor = (int)RoundTessLevel(BSS->VertexShader->TessellationMaxTessFactor, PipelineDescriptor.tessellationPartitionMode);
		PipelineDescriptor.maxTessellationFactor = FixedMaxTessFactor;
		PipelineDescriptor.tessellationFactorScaleEnabled = NO;
		PipelineDescriptor.tessellationFactorFormat = MTLTessellationFactorFormatHalf;
		PipelineDescriptor.tessellationControlPointIndexType = MTLTessellationControlPointIndexTypeNone;
		
		FunctionConstants.Push((uint32)IndexType);
	}

	check(SampleCount > 0);
	
	NSError* Error = nil;
	
	if (IsRunningRHIInSeparateThread())
	{
		SCOPE_CYCLE_COUNTER(STAT_MetalPipelineLockTime);
		int Err = pthread_rwlock_rdlock(&MetalPipelineMutex.Mutex);
		checkf(Err == 0, TEXT("pthread_rwlock_rdlock failed with error: %d"), Err);
	}
	
	FMetalRenderPipelineKey ComparableDesc;
	ComparableDesc.RenderPipelineHash = GetHash();
	ComparableDesc.VertexDescriptorHash = VertexDesc;
	ComparableDesc.VertexFunction = TPair<id<MTLFunction>, id<MTLLibrary>>(BSS->VertexShader->Function, BSS->VertexShader->Library);
	ComparableDesc.PixelFunction = TPair<id<MTLFunction>, id<MTLLibrary>>(BSS->PixelShader ? BSS->PixelShader->Function : nil, BSS->PixelShader ? BSS->PixelShader->Library : nil);
	ComparableDesc.DomainFunction = TPair<id<MTLFunction>, id<MTLLibrary>>(BSS->DomainShader ? BSS->DomainShader->Function : nil, BSS->DomainShader ? BSS->DomainShader->Library : nil);
	ComparableDesc.FunctionConstants = FunctionConstants;
	
	FMetalShaderPipeline* statePack = MetalPipelineCache.FindRef(ComparableDesc);
	if(statePack == nil)
	{
		if(IsRunningRHIInSeparateThread())
		{
			SCOPE_CYCLE_COUNTER(STAT_MetalPipelineLockTime);
			int Err = pthread_rwlock_unlock(&MetalPipelineMutex.Mutex);
			checkf(Err == 0, TEXT("pthread_rwlock_unlock failed with error: %d"), Err);
		}
		
		id<MTLDevice> Device = GetMetalDeviceContext().GetDevice();
		statePack = [FMetalShaderPipeline new];
		
		METAL_DEBUG_OPTION(FMemory::Memzero(statePack->ResourceMask, sizeof(statePack->ResourceMask)));
		
		if(BSS->HullShader)
		{
			PipelineDescriptor.vertexDescriptor = [MTLVertexDescriptor new];
			
			FMetalTessellationPipelineDesc* tessellationDesc = [[FMetalTessellationPipelineDesc new] autorelease];
			statePack.TessellationPipelineDesc = tessellationDesc;
			tessellationDesc.TessellationInputControlPointBufferIndex = UINT_MAX;
			tessellationDesc.TessellationOutputControlPointBufferIndex = UINT_MAX;
			tessellationDesc.TessellationPatchConstBufferIndex = UINT_MAX;
			tessellationDesc.TessellationInputPatchConstBufferIndex = UINT_MAX;
			tessellationDesc.TessellationFactorBufferIndex = UINT_MAX;
			tessellationDesc.TessellationPatchCountBufferIndex = UINT_MAX;
			tessellationDesc.TessellationControlPointIndexBufferIndex = UINT_MAX;
			tessellationDesc.DomainVertexDescriptor = PipelineDescriptor.vertexDescriptor;
			
			// Disambiguated function name.
			NSString* Name = [NSString stringWithFormat:@"Main_%0.8x_%0.8x", BSS->VertexShader->SourceLen, BSS->VertexShader->SourceCRC];
			if (FunctionConstants.Num())
			{
				MTLFunctionConstantValues* constantValues = [[MTLFunctionConstantValues new] autorelease];
				[constantValues setConstantValues:FunctionConstants.GetData() type:MTLDataTypeUInt withRange:NSMakeRange(0, FunctionConstants.Num())];
				computeFunction = [[BSS->VertexShader->Library newFunctionWithName:Name constantValues:constantValues error:&Error] autorelease];
			}
			else
			{
				computeFunction = [[BSS->VertexShader->Library newFunctionWithName:Name] autorelease];
			}
			
			if (computeFunction == nil)
			{
				NSLog(@"Failed to create kernel: %@", Error);
				NSLog(@"*********** Error\n%@", BSS->VertexShader->GlslCodeNSString);
				check(0);
			}
			
			tessellationDesc.DSNumUniformBuffers = BSS->DomainShader->Bindings.NumUniformBuffers;
			
			MTLComputePipelineDescriptor *computePipelineDescriptor = [MTLComputePipelineDescriptor new];
			computePipelineDescriptor.computeFunction = computeFunction;
			bool indexed = (IndexType != EMetalIndexType_None);
			MTLIndexType indexType = MTLIndexTypeUInt16;
			if (indexed)
			{
				computePipelineDescriptor.stageInputDescriptor.indexType = GetMetalIndexType(IndexType);
			}
			tessellationDesc.TessellationControlPointIndexBufferIndex = computePipelineDescriptor.stageInputDescriptor.indexBufferIndex = BSS->VertexShader->TessellationControlPointIndexBuffer;
			for(int onIndex = 0; onIndex < MaxMetalStreams; onIndex++)
			{
				// NOTE: accessing the VertexDesc like this will end up allocating layouts/attributes
				auto stride = VertexDesc.VertexDesc.layouts[onIndex].stride;
				if(stride)
				{
					computePipelineDescriptor.stageInputDescriptor.layouts[onIndex].stride = stride;
					auto stepFunction = VertexDesc.VertexDesc.layouts[onIndex].stepFunction;
					switch(stepFunction)
					{
						case MTLVertexStepFunctionConstant:
							computePipelineDescriptor.stageInputDescriptor.layouts[onIndex].stepFunction = MTLStepFunctionConstant;
							break;
						case MTLVertexStepFunctionPerVertex:
							computePipelineDescriptor.stageInputDescriptor.layouts[onIndex].stepFunction = indexed ? MTLStepFunctionThreadPositionInGridXIndexed : MTLStepFunctionThreadPositionInGridX;
							break;
						case MTLVertexStepFunctionPerInstance:
							computePipelineDescriptor.stageInputDescriptor.layouts[onIndex].stepFunction = MTLStepFunctionThreadPositionInGridY;
							break;
						default:
							check(0);
					}
					computePipelineDescriptor.stageInputDescriptor.layouts[onIndex].stepRate = VertexDesc.VertexDesc.layouts[onIndex].stepRate;
				}
				auto format = VertexDesc.VertexDesc.attributes[onIndex].format;
				if(format == MTLVertexFormatInvalid) continue;
				{
					computePipelineDescriptor.stageInputDescriptor.attributes[onIndex].format = (MTLAttributeFormat)format; // TODO FIXME currently these align perfectly (at least assert that is the case)
					computePipelineDescriptor.stageInputDescriptor.attributes[onIndex].offset = VertexDesc.VertexDesc.attributes[onIndex].offset;
					computePipelineDescriptor.stageInputDescriptor.attributes[onIndex].bufferIndex = VertexDesc.VertexDesc.attributes[onIndex].bufferIndex;
				}
			}
			
#if METAL_DEBUG_OPTIONS
			MTLAutoreleasedComputePipelineReflection reflection = nil;
			if (GetMetalDeviceContext().GetCommandQueue().GetRuntimeDebuggingLevel() >= EMetalDebugLevelFastValidation)
			{
				statePack.ComputePipelineState = [Device newComputePipelineStateWithDescriptor:computePipelineDescriptor options:MTLPipelineOptionArgumentInfo|MTLPipelineOptionBufferTypeInfo reflection:&reflection error:&Error];
				statePack.ComputePipelineReflection = reflection;
			}
			else
#endif
			{
				statePack.ComputePipelineState = [Device newComputePipelineStateWithDescriptor:computePipelineDescriptor options:MTLPipelineOptionNone reflection:nil error:&Error];
			}
			if(statePack.ComputePipelineState == nil)
			{
				NSLog(@"Failed to generate a pipeline state object: %@", Error);
				NSLog(@"Vertex shader: %@", BSS->VertexShader->GlslCodeNSString);
				NSLog(@"Pixel shader: %@", BSS->PixelShader->GlslCodeNSString);
				NSLog(@"Hull shader: %@", BSS->HullShader->GlslCodeNSString);
				NSLog(@"Domain shader: %@", BSS->DomainShader->GlslCodeNSString);
				NSLog(@"Descriptor: %@", PipelineDescriptor);
				
				UE_LOG(LogMetal, Fatal, TEXT("Failed to generate a hull pipeline state object:\n\n %s\n\n"), *FString([Error localizedDescription]));
			}
			else
			{
				TRACK_OBJECT(STAT_MetalRenderPipelineStateCount, statePack.ComputePipelineState);
				
				tessellationDesc.TessellationInputControlPointBufferIndex = BSS->DomainShader->TessellationControlPointOutBuffer;
				tessellationDesc.TessellationOutputControlPointBufferIndex = BSS->VertexShader->TessellationControlPointOutBuffer;

				tessellationDesc.TessellationInputPatchConstBufferIndex = BSS->DomainShader->TessellationHSOutBuffer;
				tessellationDesc.TessellationPatchConstBufferIndex = BSS->VertexShader->TessellationHSOutBuffer;
				
				tessellationDesc.TessellationFactorBufferIndex = BSS->VertexShader->TessellationHSTFOutBuffer;
				
				tessellationDesc.TessellationPatchCountBufferIndex = BSS->VertexShader->TessellationPatchCountBuffer;
				
				tessellationDesc.TessellationIndexBufferIndex = BSS->VertexShader->TessellationIndexBuffer;

				tessellationDesc.TessellationPatchConstOutSize = BSS->VertexShader->TessellationOutputAttribs.HSOutSize;
				MTLVertexStepFunction stepFunction = MTLVertexStepFunctionPerPatch;
                
                static MTLVertexFormat Formats[(uint8)EMetalComponentType::Max][4] = {
                    {MTLVertexFormatUInt, MTLVertexFormatUInt2, MTLVertexFormatUInt3, MTLVertexFormatUInt4},
                    {MTLVertexFormatInt, MTLVertexFormatInt2, MTLVertexFormatInt3, MTLVertexFormatInt4},
                    {MTLVertexFormatInvalid, MTLVertexFormatHalf2, MTLVertexFormatHalf3, MTLVertexFormatHalf4},
                    {MTLVertexFormatFloat, MTLVertexFormatFloat2, MTLVertexFormatFloat3, MTLVertexFormatFloat4},
                    {MTLVertexFormatInvalid, MTLVertexFormatUChar2, MTLVertexFormatUChar3, MTLVertexFormatUChar4},
                };
                
                if (BSS->DomainShader->TessellationHSOutBuffer != UINT_MAX)
                {
                    check(BSS->DomainShader->TessellationHSOutBuffer < ML_MaxBuffers);
                    uint32 bufferIndex = BSS->DomainShader->TessellationHSOutBuffer;
                    uint32 bufferSize = BSS->VertexShader->TessellationOutputAttribs.HSOutSize;
                    
                    PipelineDescriptor.vertexDescriptor.layouts[bufferIndex].stride = bufferSize;
                    PipelineDescriptor.vertexDescriptor.layouts[bufferIndex].stepFunction = stepFunction;
                    PipelineDescriptor.vertexDescriptor.layouts[bufferIndex].stepRate = 1;
				
                    for (FMetalAttribute const& Attrib : BSS->VertexShader->TessellationOutputAttribs.HSOut)
                    {
                        int attributeIndex = Attrib.Index;
                        check(attributeIndex >= 0 && attributeIndex <= 31);
                        check(Attrib.Components > 0 && Attrib.Components <= 4);
                        MTLVertexFormat format = Formats[(uint8)Attrib.Type][Attrib.Components-1];
                        check(format != MTLVertexFormatInvalid); // TODO support more cases
                        PipelineDescriptor.vertexDescriptor.attributes[attributeIndex].format = format;
                        PipelineDescriptor.vertexDescriptor.attributes[attributeIndex].offset = Attrib.Offset;
                        PipelineDescriptor.vertexDescriptor.attributes[attributeIndex].bufferIndex = bufferIndex;
                    }
                }
				
				tessellationDesc.TessellationPatchControlPointOutSize = BSS->VertexShader->TessellationOutputAttribs.PatchControlPointOutSize;
				stepFunction = MTLVertexStepFunctionPerPatchControlPoint;
				uint32 bufferIndex = BSS->DomainShader->TessellationControlPointOutBuffer;
				uint32 bufferSize = BSS->VertexShader->TessellationOutputAttribs.PatchControlPointOutSize;
				
				PipelineDescriptor.vertexDescriptor.layouts[bufferIndex].stride = bufferSize;
				PipelineDescriptor.vertexDescriptor.layouts[bufferIndex].stepFunction = stepFunction;
				PipelineDescriptor.vertexDescriptor.layouts[bufferIndex].stepRate = 1;
				
				for (FMetalAttribute const& Attrib : BSS->VertexShader->TessellationOutputAttribs.PatchControlPointOut)
				{
					int attributeIndex = Attrib.Index;
					check(attributeIndex >= 0 && attributeIndex <= 31);
					check(Attrib.Components > 0 && Attrib.Components <= 4);
					MTLVertexFormat format = Formats[(uint8)Attrib.Type][Attrib.Components-1];
					check(format != MTLVertexFormatInvalid); // TODO support more cases
					PipelineDescriptor.vertexDescriptor.attributes[attributeIndex].format = format;
					PipelineDescriptor.vertexDescriptor.attributes[attributeIndex].offset = Attrib.Offset;
					PipelineDescriptor.vertexDescriptor.attributes[attributeIndex].bufferIndex = bufferIndex;
				}
				
				tessellationDesc.TessellationTessFactorOutSize = BSS->VertexShader->TessellationOutputAttribs.HSTFOutSize;
				
#if METAL_DEBUG_OPTIONS
				if (reflection)
				{
					bool found__HSTFOut = false;
					for(MTLArgument* arg in reflection.arguments)
					{
						bool addAttributes = false;
						stepFunction = (MTLVertexStepFunction)-1; // invalid
						
						bufferIndex = UINT_MAX;
						
						if([arg.name isEqualToString: @"PatchControlPointOutBuffer"])
						{
							check((arg.bufferAlignment & (arg.bufferAlignment - 1)) == 0); // must be pow2
							check((arg.bufferDataSize & (arg.bufferAlignment - 1)) == 0); // must be aligned
							
							check(arg.bufferDataSize == BSS->VertexShader->TessellationOutputAttribs.PatchControlPointOutSize);
							
							addAttributes = true;
							bufferIndex = BSS->DomainShader->TessellationControlPointOutBuffer;
							stepFunction = MTLVertexStepFunctionPerPatchControlPoint;
							check(arg.index == BSS->VertexShader->TessellationControlPointOutBuffer);
						}
						else if([arg.name isEqualToString: @"__HSOut"])
						{
							check((arg.bufferAlignment & (arg.bufferAlignment - 1)) == 0); // must be pow2
							check((arg.bufferDataSize & (arg.bufferAlignment - 1)) == 0); // must be aligned
							
							check(arg.bufferDataSize == BSS->VertexShader->TessellationOutputAttribs.HSOutSize);
							
							addAttributes = true;
							bufferIndex = BSS->DomainShader->TessellationHSOutBuffer;
							stepFunction = MTLVertexStepFunctionPerPatch;
							check(arg.index == BSS->VertexShader->TessellationHSOutBuffer);
						}
						else if([arg.name isEqualToString: @"__HSTFOut"])
						{
							found__HSTFOut = true;
							check((arg.bufferAlignment & (arg.bufferAlignment - 1)) == 0); // must be pow2
							check((arg.bufferDataSize & (arg.bufferAlignment - 1)) == 0); // must be aligned
							
							check(arg.bufferDataSize == BSS->VertexShader->TessellationOutputAttribs.HSTFOutSize);
							
							check(arg.index == BSS->VertexShader->TessellationHSTFOutBuffer);
						}
						else if([arg.name isEqualToString:@"patchCount"])
						{
							check(arg.index == BSS->VertexShader->TessellationPatchCountBuffer);
						}
						else if([arg.name isEqualToString:@"indexBuffer"])
						{
							check(arg.index == BSS->VertexShader->TessellationIndexBuffer);
						}
						
						// build the vertexDescriptor
						if(addAttributes)
						{
							check(PipelineDescriptor.vertexDescriptor.layouts[bufferIndex].stride == arg.bufferDataSize);
							check(PipelineDescriptor.vertexDescriptor.layouts[bufferIndex].stepFunction == stepFunction);
							check(PipelineDescriptor.vertexDescriptor.layouts[bufferIndex].stepRate == 1);
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
								check(PipelineDescriptor.vertexDescriptor.attributes[attributeIndex].format == format);
								check(PipelineDescriptor.vertexDescriptor.attributes[attributeIndex].offset == attribute.offset);
								check(PipelineDescriptor.vertexDescriptor.attributes[attributeIndex].bufferIndex == bufferIndex);
							}
						}
					}
					check(found__HSTFOut);
				}
#endif
				check(tessellationDesc.TessellationOutputControlPointBufferIndex < ML_MaxBuffers);
				check(tessellationDesc.TessellationFactorBufferIndex < ML_MaxBuffers);
				check(tessellationDesc.TessellationPatchCountBufferIndex < ML_MaxBuffers);
			}
			
			check(statePack.TessellationPipelineDesc);
			check(computePipelineDescriptor);
			
			check(statePack.TessellationPipelineDesc.TessellationTessFactorOutSize == 2*4 || statePack.TessellationPipelineDesc.TessellationTessFactorOutSize == 2*6);
			[computePipelineDescriptor release];
		}
		
#if METAL_DEBUG_OPTIONS
		if (GetMetalDeviceContext().GetCommandQueue().GetRuntimeDebuggingLevel() >= EMetalDebugLevelFastValidation)
		{
			MTLAutoreleasedRenderPipelineReflection Reflection;
			statePack.RenderPipelineState = [Device newRenderPipelineStateWithDescriptor:PipelineDescriptor options:MTLPipelineOptionArgumentInfo reflection:&Reflection error:&Error];
			statePack.RenderPipelineReflection = Reflection;
		}
		else
#endif
		{
			statePack.RenderPipelineState = [Device newRenderPipelineStateWithDescriptor:PipelineDescriptor error : &Error];
		}
		
		if(statePack.RenderPipelineState)
		{
			TRACK_OBJECT(STAT_MetalRenderPipelineStateCount, statePack.RenderPipelineState);
			
#if METAL_DEBUG_OPTIONS
			statePack.ComputeSource = BSS->DomainShader ? BSS->VertexShader->GlslCodeNSString : nil;
			statePack.VertexSource = BSS->DomainShader ? BSS->DomainShader->GlslCodeNSString : BSS->VertexShader->GlslCodeNSString;
			statePack.FragmentSource = BSS->PixelShader ? BSS->PixelShader->GlslCodeNSString : nil;
#endif
			
			if(IsRunningRHIInSeparateThread())
			{
				SCOPE_CYCLE_COUNTER(STAT_MetalPipelineLockTime);
				int Err = pthread_rwlock_wrlock(&MetalPipelineMutex.Mutex);
				checkf(Err == 0, TEXT("pthread_rwlock_wrlock failed with error: %d"), Err);
			}
			
			FMetalShaderPipeline* ExistingPipeline = MetalPipelineCache.FindRef(ComparableDesc);
			if (!ExistingPipeline)
			{
				MetalPipelineCache.Add(ComparableDesc, statePack);
			}
			else // Someone beat us to it - release our references and hand back the existing version.
			{
				[statePack release];
				statePack = ExistingPipeline;
			}
			
			if(IsRunningRHIInSeparateThread())
			{
				SCOPE_CYCLE_COUNTER(STAT_MetalPipelineLockTime);
				int Err = pthread_rwlock_unlock(&MetalPipelineMutex.Mutex);
				checkf(Err == 0, TEXT("pthread_rwlock_unlock failed with error: %d"), Err);
			}
		}
		else
		{
			NSLog(@"Failed to generate a pipeline state object: %@", Error);
			NSLog(@"Vertex shader: %@", BSS->VertexShader->GlslCodeNSString);
			NSLog(@"Pixel shader: %@", BSS->PixelShader->GlslCodeNSString);
			
			if (IsValidRef(BSS->HullShader))
			{
				NSLog(@"Hull shader: %@", BSS->HullShader->GlslCodeNSString);
			}
			if (IsValidRef(BSS->DomainShader))
			{
				NSLog(@"Domain shader: %@", BSS->DomainShader->GlslCodeNSString);
			}
			NSLog(@"Descriptor: %@", PipelineDescriptor);
			
			UE_LOG(LogMetal, Fatal, TEXT("Failed to generate a render pipeline state object:\n\n %s\n\n"), *FString([Error localizedDescription]));
			return nil;
		}
	}
	else if(IsRunningRHIInSeparateThread())
	{
		SCOPE_CYCLE_COUNTER(STAT_MetalPipelineLockTime);
		int Err = pthread_rwlock_unlock(&MetalPipelineMutex.Mutex);
		checkf(Err == 0, TEXT("pthread_rwlock_unlock failed with error: %d"), Err);
	}
	
	PipelineDescriptor.depthAttachmentPixelFormat = DepthFormat;
	PipelineDescriptor.stencilAttachmentPixelFormat = StencilFormat;
	
	return statePack;
}

bool FMetalRenderPipelineDesc::FMetalRenderPipelineKey::operator==(FMetalRenderPipelineKey const& Other) const
{
	if (this != &Other)
	{
		return (RenderPipelineHash.RasterBits == Other.RenderPipelineHash.RasterBits) && (RenderPipelineHash.TargetBits == Other.RenderPipelineHash.TargetBits) && (VertexDescriptorHash == Other.VertexDescriptorHash) && (VertexFunction == Other.VertexFunction) && (PixelFunction == Other.PixelFunction) && (DomainFunction == Other.DomainFunction) && (FunctionConstants == Other.FunctionConstants);
	}
	return true;
}
